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
                if (++thread_counter_ > max_concurrent_attach_thread)
                {
                    HPX_THROW_EXCEPTION(invalid_status,
                        "hpx::cds::thread_manager_wrapper ",
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
                        "hpx::cds::thread_manager_wrapper",
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
        static std::atomic<std::size_t> max_concurrent_attach_thread;

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
                hpxthread_manager_wrapper::max_concurrent_attach_thread)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::cds::thread_manager_wrapper ",
                    "attaching more threads than number of maximum allowed "
                    "detached threads, consider update "
                    "hpx::cds::thread_manager_wrapper::max_concurrent_"
                    "attach_thread_");
            }
            ::cds::threading::Manager::attachThread();
        }

        ~stdthread_manager_wrapper()
        {
            if (hpxthread_manager_wrapper::thread_counter_-- == 0)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::cds::thread_manager_wrapper",
                    "detaching more threads than number of attached "
                    "threads");
            }
            ::cds::threading::Manager::detachThread();
        }
    };

}}    // namespace hpx::cds

#endif    // #ifndef LIBCDS_HPX_TLS_MANAGER
