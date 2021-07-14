//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/plugin/traits/plugin_config_data.hpp>

#include <hpx/modules/lci_base.hpp>
#include <hpx/plugins/parcelport_factory.hpp>
#include <hpx/command_line_handling/command_line_handling.hpp>

// parcelport
#include <hpx/runtime_distributed.hpp>
#include <hpx/runtime/parcelset/locality.hpp>
#include <hpx/runtime/parcelset/parcelport_impl.hpp>

#include <hpx/synchronization/spinlock.hpp>
#include <hpx/synchronization/condition_variable.hpp>

#include <hpx/plugins/parcelport/lci/locality.hpp>
#include <hpx/plugins/parcelport/lci/header.hpp>
#include <hpx/plugins/parcelport/lci/sender.hpp>
#include <hpx/plugins/parcelport/lci/receiver.hpp>

#include <hpx/execution_base/this_thread.hpp>
#include <hpx/runtime_configuration/runtime_configuration.hpp>
#include <hpx/util/get_entry_as.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx
{
    bool is_starting();
}

namespace hpx { namespace parcelset
{
    namespace policies { namespace lci
    {
        class HPX_EXPORT parcelport;
    }}

    template <>
    struct connection_handler_traits<policies::lci::parcelport>
    {
        typedef policies::lci::sender_connection connection_type;
        typedef std::true_type  send_early_parcel;
        typedef std::true_type  do_background_work;
        typedef std::false_type send_immediate_parcels;

        static const char * type()
        {
            return "lci";
        }

        static const char * pool_name()
        {
            return "parcel-pool-lci";
        }

        static const char * pool_name_postfix()
        {
            return "-lci";
        }
    };

    namespace policies { namespace lci
    {
        int acquire_tag(sender * s)
        {
            return s->acquire_tag();
        }

        void add_connection(sender * s, std::shared_ptr<sender_connection> const &ptr)
        {
            s->add(ptr);
        }

        class HPX_EXPORT parcelport
          : public parcelport_impl<parcelport>
        {
            typedef parcelport_impl<parcelport> base_type;

            static parcelset::locality here()
            {
                return
                    parcelset::locality(
                        locality(
                            util::lci_environment::enabled() ?
                            util::lci_environment::rank() : -1
                        )
                    );
            }

            static std::size_t max_connections(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.lci.max_connections", HPX_PARCEL_MAX_CONNECTIONS);
            }
        public:
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(), notifier)
              , stopped_(false)
              , receiver_(*this)
            {}

            ~parcelport()
            {
                util::lci_environment::finalize();
            }

            /// Start the handling of connections.
            bool do_run()
            {
                receiver_.run();
                sender_.run();
                for(std::size_t i = 0; i != io_service_pool_.size(); ++i)
                {
                    io_service_pool_.get_io_service(int(i)).post(
                        hpx::util::bind(
                            &parcelport::io_service_work, this
                        )
                    );
                }
                return true;
            }

            /// Stop the handling of connections.
            void do_stop()
            {
                while(do_background_work(0, parcelport_background_mode_all))
                {
                    if(threads::get_self_ptr())
                        hpx::this_thread::suspend(
                            hpx::threads::thread_schedule_state::pending,
                            "lci::parcelport::do_stop");
                }
                stopped_ = true;
                LCI_PM_barrier();
                //LCI_barrier();
            }

            /// Return the name of this locality
            std::string get_locality_name() const override
            {
                return util::lci_environment::get_processor_name();
            }

            std::shared_ptr<sender_connection> create_connection(
                parcelset::locality const& l, error_code&)
            {
                int dest_rank = l.get<locality>().rank();
                return sender_.create_connection(dest_rank, this);
            }

            parcelset::locality agas_locality(
                util::runtime_configuration const&) const override
            {
                return parcelset::locality(
                    locality(util::lci_environment::enabled() ? 0 : -1));
            }

            parcelset::locality create_locality() const override
            {
                return parcelset::locality(locality());
            }

            bool background_work(
                std::size_t /* num_thread */, parcelport_background_mode mode)
            {
                if (stopped_)
                    return false;

                bool has_work = false;
                if (mode & parcelport_background_mode_send)
                {
                    has_work = sender_.background_work();
                }
                if (mode & parcelport_background_mode_receive)
                {
                    has_work = receiver_.background_work() || has_work;
                }
                return has_work;
            }

        private:
            typedef lcos::local::spinlock mutex_type;

            std::atomic<bool> stopped_;

            sender sender_;
            receiver<parcelport> receiver_;

            void io_service_work()
            {
                std::size_t k = 0;
                // We only execute work on the IO service while HPX is starting
                while(hpx::is_starting())
                {
                    bool has_work = sender_.background_work();
                    has_work = receiver_.background_work() || has_work;
                    if(has_work)
                    {
                        k = 0;
                    }
                    else
                    {
                        ++k;
                        util::detail::yield_k(k,
                            "hpx::parcelset::policies::lci::parcelport::"
                                "io_service_work");
                    }
                }
            }

            void early_write_handler(
                std::error_code const& ec, parcel const & p)
            {
                if (ec) {
                    // all errors during early parcel handling are fatal
                    std::exception_ptr exception =
                        hpx::detail::get_exception(hpx::exception(ec),
                            "lci::early_write_handler", __FILE__, __LINE__,
                            "error while handling early parcel: " +
                                ec.message() + "(" +
                                std::to_string(ec.value()) +
                                ")" + parcelset::dump_parcel(p));

                    hpx::report_error(exception);
                }
            }

        };
    }}
}}

#include <hpx/config/warnings_suffix.hpp>

namespace hpx { namespace traits
{
    // Inject additional configuration data into the factory registry for this
    // type. This information ends up in the system wide configuration database
    // under the plugin specific section:
    //
    //      [hpx.parcel.lci]
    //      ...
    //      priority = 200
    //
    template <>
    struct plugin_config_data<hpx::parcelset::policies::lci::parcelport>
    {
        static char const* priority()
        {
            return "200";
        }
        static void init(int *argc, char ***argv, util::command_line_handling &cfg)
        {
            util::lci_environment::init(argc, argv, cfg.rtcfg_);
            cfg.num_localities_ =
                static_cast<std::size_t>(util::lci_environment::size());
            cfg.node_ = static_cast<std::size_t>(util::lci_environment::rank());
        }

        static void destroy()
        {
            util::lci_environment::finalize();
        }

        static char const* call()
        {
            return
                // TODO: change these for LCI
#if defined(HPX_HAVE_PARCELPORT_LCI_ENV)
                "env = ${HPX_HAVE_PARCELPORT_LCI_ENV:" HPX_HAVE_PARCELPORT_LCI_ENV "}\n"
#else
                "env = ${HPX_HAVE_PARCELPORT_LCI_ENV:"
                        "MV2_COMM_WORLD_RANK,PMIX_RANK,PMI_RANK,LCI_COMM_WORLD_SIZE,"
                        "ALPS_APP_PE"
                    "}\n"
#endif
                "multithreaded = ${HPX_HAVE_PARCELPORT_LCI_MULTITHREADED:0}\n"
                "max_connections = ${HPX_HAVE_PARCELPORT_LCI_MAX_CONNECTIONS:8192}\n"
                ;
        }
    };
}}

HPX_REGISTER_PARCELPORT(
    hpx::parcelset::policies::lci::parcelport,
    lci);

#endif