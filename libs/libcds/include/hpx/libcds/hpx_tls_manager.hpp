// Copyright (c) 2020 Weile Wei
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LIBCDS_HPX_TLS_MANAGER
#define LIBCDS_HPX_TLS_MANAGER

#include <hpx/config.hpp>
#include <hpx/errors/exception.hpp>
//
#include <cds/gc/details/hp_common.h>
#include <cds/gc/hp.h>
#include <cds/init.h>
#include <cds/threading/details/cxx11_manager.h>

#include <atomic>

/// \cond NODETAIL
namespace cds { namespace gc { namespace hp { namespace details {

    class HPXTLSManager
    {
    public:
        static CDS_EXPORT_API thread_data* getTLS();
        static CDS_EXPORT_API void setTLS(thread_data*);
    };

}}}}    // namespace cds::gc::hp::details

namespace hpx { namespace cds {
    // this wrapper will initialize libCDS
    struct libcds_wrapper
    {
        libcds_wrapper()
        {
            // Initialize libcds
            ::cds::Initialize();
        }

        ~libcds_wrapper()
        {
            // Terminate libcds
            ::cds::Terminate();
        }
    };

    template <typename TLS>
    struct hazard_pointer_wrapper
    {
        // this wrapper will create Hazard Pointer SMR singleton
        hazard_pointer_wrapper(std::size_t hazard_pointer_count = 1,
            std::size_t max_thread_count = max_concurrent_attach_thread_,
            std::size_t max_retired_pointer_count = 16)
        {
            // hazard_pointer_count is corresponding var nHazardPtrCount
            // in libcds that defines Hazard pointer count per thread;

            // max_concurrent_attach_thread_ is corresponding var nMaxThreadCount
            // in libcds that defines Max count of simultaneous working thread in your application

            // max_retired_pointer_count is corresponding var nMaxRetiredPtrCount
            // in libcds that defines Capacity of the array of retired objects for the thread
            ::cds::gc::hp::custom_smr<TLS>::construct(hazard_pointer_count,
                max_thread_count, max_retired_pointer_count);
        }

        static std::size_t get_max_concurrent_attach_thread()
        {
            return max_concurrent_attach_thread_;
        }

        // default 100 concurrent threads attached to Hazard Pointer SMR
        static std::atomic<std::size_t> max_concurrent_attach_thread_;
    };

    // this wrapper will initialize an HPX thread/task for use with libCDS
    // algorithms
    struct hpxthread_manager_wrapper
    {
        // allow the std thread wrapper to use the same counter because
        // the libCDS backend does not distinguish between them yet.
        friend struct stdthread_manager_wrapper;

        // the boolean uselibcs option is provided to make comparison
        // of certain tests with/without libcds easier
        // @TODO : we should remove it one day
        explicit hpxthread_manager_wrapper(bool uselibcds = true)
          : uselibcds_(uselibcds)
        {
            if (uselibcds_)
            {
                if (++thread_counter_ >
                    hazard_pointer_wrapper<::cds::gc::hp::details::
                            HPXTLSManager>::max_concurrent_attach_thread_)
                {
                    HPX_THROW_EXCEPTION(invalid_status,
                        "hpx::cds::hpxthread_manager_wrapper ",
                        "attaching more threads than number of maximum allowed "
                        "detached threads, consider update "
                        "hpx::cds::thread_manager_wrapper::max_concurrent_"
                        "attach_thread_");
                }
                ::cds::gc::hp::custom_smr<
                    ::cds::gc::hp::details::HPXTLSManager>::attach_thread();
            }
        }

        ~hpxthread_manager_wrapper()
        {
            if (uselibcds_)
            {
                if (thread_counter_-- == 0)
                {
                    HPX_THROW_EXCEPTION(invalid_status,
                        "hpx::cds::hpxthread_manager_wrapper",
                        "detaching more threads than number of attached "
                        "threads");
                }
                ::cds::gc::hp::custom_smr<
                    ::cds::gc::hp::details::HPXTLSManager>::detach_thread();
            }
        }

        // max_concurrent_attach_thread is corresponding variable to
        // the variable nMaxThreadCount in Hazard Pointer class. It defines
        // max count of simultaneous working thread in the application, default 100
        // and it is public to user for use

    private:
        static std::atomic<std::size_t> thread_counter_;
        bool uselibcds_;
    };

    // this wrapper will initialize a std::thread for use with libCDS
    // algorithms
    struct stdthread_manager_wrapper
    {
        explicit stdthread_manager_wrapper()
        {
            if (++hpxthread_manager_wrapper::thread_counter_ >
                hazard_pointer_wrapper<::cds::gc::hp::details::
                        DefaultTLSManager>::max_concurrent_attach_thread_)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::cds::stdthread_manager_wrapper ",
                    "attaching more threads than number of maximum allowed "
                    "detached threads, consider update "
                    "hpx::cds::thread_manager_wrapper::max_concurrent_"
                    "attach_thread");
            }
            ::cds::gc::hp::custom_smr<
                ::cds::gc::hp::details::DefaultTLSManager>::attach_thread();
        }

        ~stdthread_manager_wrapper()
        {
            if (hpxthread_manager_wrapper::thread_counter_-- == 0)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::cds::stdthread_manager_wrapper",
                    "detaching more threads than number of attached "
                    "threads");
            }
            ::cds::gc::hp::custom_smr<
                ::cds::gc::hp::details::DefaultTLSManager>::detach_thread();
        }
    };

}}    // namespace hpx::cds

#endif    // #ifndef LIBCDS_HPX_TLS_MANAGER
