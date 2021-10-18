//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/modules/lci_base.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/util.hpp>

#include <boost/tokenizer.hpp>

#include <cstddef>
#include <cstdlib>
#include <string>

#define DEBUG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
// #define DEBUG(...)

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util {

    namespace detail {

        bool detect_lci_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
#if defined(__bgq__)
            // If running on BG/Q, we can safely assume to always run in an
            // LCI environment
            return true;
#else
            std::string lci_environment_strings =
                cfg.get_entry("hpx.parcel.lci.env", default_env);

            typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
            boost::char_separator<char> sep(";,: ");
            tokenizer tokens(lci_environment_strings, sep);
            for (tokenizer::iterator it = tokens.begin(); it != tokens.end();
                 ++it)
            {
                char* env = std::getenv(it->c_str());
                if (env)
                    return true;
            }
            return false;
#endif
        }
    }    // namespace detail

    bool lci_environment::check_lci_environment(
        util::runtime_configuration const& cfg)
    { // TODO: this should be returning false when we're using the MPI environment
#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)
        // We disable the LCI parcelport if any of these hold:
        //
        // - The parcelport is explicitly disabled
        // - The application is not run in an LCI environment
        // - The TCP parcelport is enabled and has higher priority
        // - The MPI parcelport is enabled and has higher priority
        if (get_entry_as(cfg, "hpx.parcel.lci.enable", 1) == 0 ||
            !detail::detect_lci_environment(cfg, HPX_HAVE_PARCELPORT_LCI_ENV) ||
            (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.lci.priority", 0))) ||
            (get_entry_as(cfg, "hpx.parcel.mpi.enable", 1) &&
             (get_entry_as(cfg, "hpx.parcel.mpi.priority", 1) >
              get_entry_as(cfg, "hpx.parcel.lci.priority", 0))))
        {
            return false;
        }

        return true;
#elif defined(HPX_HAVE_MODULE_LCI_BASE)
        // if LCI futures are enabled while networking is off we need to
        // check whether we were run using mpirun
        return detail::detect_lci_environment(cfg, HPX_HAVE_PARCELPORT_LCI_ENV);
#else
        return false;
#endif
    }
}}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)) ||      \
    defined(HPX_HAVE_MODULE_LCI_BASE)

namespace hpx { namespace util {

    lci_environment::mutex_type lci_environment::mtx_;
    bool lci_environment::enabled_ = false;
    bool lci_environment::has_called_init_ = false;
    int lci_environment::provided_threading_flag_ = MPI_THREAD_SINGLE;
    MPI_Comm lci_environment::communicator_ = MPI_COMM_NULL;

    int lci_environment::is_initialized_ = -1;

    LCI_endpoint_t lci_environment::ep_;
    LCI_endpoint_t lci_environment::rt_ep_;
    LCI_comp_t lci_environment::rt_cq_r_;
    LCI_endpoint_t lci_environment::h_ep_;
    LCI_comp_t lci_environment::h_cq_r_;

    ///////////////////////////////////////////////////////////////////////////
    int lci_environment::init(
        int*, char***, const int required, const int minimal, int& provided)
    {


        has_called_init_ = false;

        // Check if MPI_Init has been called previously
        int is_initialized = 0;
        int retval = MPI_Initialized(&is_initialized);
        if (MPI_SUCCESS != retval)
        {
            return retval;
        }
        if (!is_initialized)
        {
            retval = MPI_Init_thread(nullptr, nullptr, required, &provided);
            if (MPI_SUCCESS != retval)
            {
                return retval;
            }

            if (provided < minimal)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::util::lci_environment::init",
                    "MPI doesn't provide minimal requested thread level");
            }
            has_called_init_ = true;
        }

        int lci_initialized = 0;
        LCI_initialized(&lci_initialized);
        if(!lci_initialized) {
                LCI_error_t lci_retval = LCI_initialize();
                if(lci_retval != LCI_OK) return lci_retval;
        }

        // create main endpoint for pt2pt msgs
        LCI_plist_t plist_;
        LCI_plist_create(&plist_);
        LCI_plist_set_comp_type(plist_, LCI_PORT_COMMAND, LCI_COMPLETION_SYNC);
        LCI_plist_set_comp_type(plist_, LCI_PORT_MESSAGE, LCI_COMPLETION_SYNC);
        LCI_endpoint_init(&ep_, LCI_UR_DEVICE, plist_);
        LCI_plist_free(&plist_);

        // set endpoint for release tag msgs
        rt_ep_ = LCI_UR_ENDPOINT;
        rt_cq_r_ = LCI_UR_CQ;

        // create endpoint for header msgs
        LCI_plist_t h_plist_;
        LCI_plist_create(&h_plist_);
        LCI_queue_create(LCI_UR_DEVICE, &h_cq_r_);
        LCI_plist_set_comp_type(h_plist_, LCI_PORT_MESSAGE, LCI_COMPLETION_QUEUE);
        LCI_plist_set_comp_type(h_plist_, LCI_PORT_COMMAND, LCI_COMPLETION_QUEUE);
        LCI_plist_set_default_comp(h_plist_, h_cq_r_);
        LCI_endpoint_init(&h_ep_, LCI_UR_DEVICE, h_plist_);
        LCI_plist_free(&h_plist_);
        // DEBUG("Rank %d: Init lci env", LCI_RANK);

        return retval;
    }

    ///////////////////////////////////////////////////////////////////////////
    void lci_environment::init(
        int* argc, char*** argv, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        int this_rank = -1;
        has_called_init_ = false;

        // We assume to use the LCI parcelport if it is not explicitly disabled
        enabled_ = check_lci_environment(rtcfg);
        if (!enabled_)
        {
            rtcfg.add_entry("hpx.parcel.lci.enable", "0");
            return;
        }

        rtcfg.add_entry("hpx.parcel.bootstrap", "lci");

        int required = MPI_THREAD_SINGLE;
        int minimal = MPI_THREAD_SINGLE;
#if defined(HPX_HAVE_PARCELPORT_LCI_MULTITHREADED)
        required =
            (get_entry_as(rtcfg, "hpx.parcel.lci.multithreaded", 1) != 0) ?
            MPI_THREAD_MULTIPLE :
            MPI_THREAD_SINGLE;

#if defined(MVAPICH2_VERSION) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MVAPICH2 if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MV2_ENABLE_AFFINITY", "0", 1);
#endif
#endif

        int retval =
            init(argc, argv, required, minimal, provided_threading_flag_);
        if (MPI_SUCCESS != retval && MPI_ERR_OTHER != retval)
        {
            // explicitly disable lci if not run by mpirun
            rtcfg.add_entry("hpx.parcel.lci.enable", "0");

            enabled_ = false;

            int msglen = 0;
            char message[MPI_MAX_ERROR_STRING + 1];
            MPI_Error_string(retval, message, &msglen);
            message[msglen] = '\0';

            std::string msg("lci_environment::init: MPI_Init_thread failed: ");
            msg = msg + message + ".";
            throw std::runtime_error(msg.c_str());
        }

        MPI_Comm_dup(MPI_COMM_WORLD, &communicator_);

        if (provided_threading_flag_ < MPI_THREAD_SERIALIZED)
        {
            // explicitly disable lci if not run by mpirun
            rtcfg.add_entry("hpx.parcel.lci.multithreaded", "0");
        }

        if (provided_threading_flag_ == MPI_THREAD_FUNNELED)
        {
            enabled_ = false;
            has_called_init_ = false;
            throw std::runtime_error(
                "lci_environment::init: MPI_Init_thread: "
                "The underlying MPI implementation only supports "
                "MPI_THREAD_FUNNELED. This mode is not supported by HPX. "
                "Please pass -Ihpx.parcel.lci.multithreaded=0 to explicitly "
                "disable MPI multi-threading.");
        }

        this_rank = rank();

#if defined(HPX_HAVE_NETWORKING)
        if (this_rank == 0)
        {
            rtcfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = hpx::runtime_mode::worker;
        }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        rtcfg.mode_ = hpx::runtime_mode::console;
#else
        rtcfg.mode_ = hpx::runtime_mode::local;
#endif

        rtcfg.add_entry("hpx.parcel.lci.rank", std::to_string(this_rank));
        rtcfg.add_entry("hpx.parcel.lci.processorname", get_processor_name());
    }

    std::string lci_environment::get_processor_name()
    { // TODO: find out how to do with with LCI and if it's necessary for LCI implementation
        char name[MPI_MAX_PROCESSOR_NAME + 1] = {'\0'};
        int len = 0;
        MPI_Get_processor_name(name, &len);

        return name;
    }

    void lci_environment::finalize()
    {
        if (enabled() && has_called_init())
        {
            int lci_init = 0;
            LCI_initialized(&lci_init);
            if(lci_init) LCI_finalize();
            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (!is_finalized)
            {
                MPI_Finalize();
            }
        }
    }

    bool lci_environment::enabled()
    {
        return enabled_;
    }

    bool lci_environment::multi_threaded()
    { // TODO: find out how to do with with LCI
        return provided_threading_flag_ >= MPI_THREAD_SERIALIZED;
    }

    bool lci_environment::has_called_init()
    {
        return has_called_init_;
    }

    int lci_environment::size()
    {
        int res(-1);
        if (enabled())
            res = LCI_NUM_PROCESSES;
        return res;
    }

    int lci_environment::rank()
    {
        int res(-1);
        if (enabled())
            res = LCI_RANK;
        return res;
    }

    MPI_Comm& lci_environment::communicator()
    {
        return communicator_;
    }

    LCI_endpoint_t& lci_environment::lci_endpoint() {
        return ep_;
    }

    LCI_endpoint_t& lci_environment::rt_endpoint() {
        return rt_ep_;
    }

    LCI_comp_t& lci_environment::rt_queue() {
        return rt_cq_r_;
    }

    LCI_endpoint_t& lci_environment::h_endpoint() {
        return h_ep_;
    }

    LCI_comp_t& lci_environment::h_queue() {
        return h_cq_r_;
    }

    lci_environment::scoped_lock::scoped_lock()
    {
        if (!multi_threaded())
            mtx_.lock();
    }

    lci_environment::scoped_lock::~scoped_lock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    void lci_environment::scoped_lock::unlock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    lci_environment::scoped_try_lock::scoped_try_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            locked = mtx_.try_lock();
        }
    }

    lci_environment::scoped_try_lock::~scoped_try_lock()
    {
        if (!multi_threaded() && locked)
            mtx_.unlock();
    }

    void lci_environment::scoped_try_lock::unlock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }
}}    // namespace hpx::util

#endif
