// Copyright (c) 2020 Weile Wei
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LIBCDS_HPX_TLS_MANAGER
#define LIBCDS_HPX_TLS_MANAGER

#include <hpx/config.hpp>
#include <cds/gc/details/hp_common.h>

//@cond
namespace cds { namespace gc { namespace hp { namespace details {

    class HPXTLSManager
    {
    public:
        static CDS_EXPORT_API thread_data* getTLS();
        static CDS_EXPORT_API void setTLS(thread_data*);
    };

}}}}    // namespace cds::gc::hp::details

#endif    // #ifndef LIBCDS_HPX_TLS_MANAGER
