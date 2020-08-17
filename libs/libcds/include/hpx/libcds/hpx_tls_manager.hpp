// Copyright (c) 2020 Weile Wei
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LIBCDS_HPX_TLS_MANAGER
#define LIBCDS_HPX_TLS_MANAGER

#include <hpx/config.hpp>
//
#include <cds/gc/details/hp_common.h>
#include <cds/gc/hp.h>

//@cond
namespace cds { namespace gc { namespace hp { namespace details {

    class HPXTLSManager
    {
    public:
        static CDS_EXPORT_API thread_data* getTLS();
        static CDS_EXPORT_API void setTLS(thread_data*);
    };

}}}}    // namespace cds::gc::hp::details

namespace hpx { namespace cds {
    ///////////////////////////////////////////////////////////////////////////////
    struct thread_manager_wrapper
    {
        // the boolean uselibcs option is provided to make comparison
        // of certain tests with/without libcds easier
        // @TODO : we should remove it one day
        explicit thread_manager_wrapper(bool uselibcds = true)
          : uselibcds_(uselibcds)
        {
            if (uselibcds_)
                ::cds::gc::hp::custom_smr<
                    ::cds::gc::hp::details::HPXTLSManager>::attach_thread();
        }
        ~thread_manager_wrapper()
        {
            if (uselibcds_)
                ::cds::gc::hp::custom_smr<
                    ::cds::gc::hp::details::HPXTLSManager>::detach_thread();
        }

        bool uselibcds_;
    };

}}    // namespace hpx::cds

#endif    // #ifndef LIBCDS_HPX_TLS_MANAGER
