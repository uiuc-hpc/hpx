//  Copyright (c) 2020 Weile Wei
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/libcds/hpx_tls_manager.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/threading_base/thread_data.hpp>

#include <cds/gc/details/hp_common.h>

#include <atomic>
#include <cstddef>

namespace cds { namespace gc { namespace hp { namespace details {

    /*static*/ CDS_EXPORT_API thread_data* HPXTLSManager::getTLS()
    {
        auto thread_id = hpx::threads::get_self_id();
        std::size_t hpx_tls_data =
            hpx::threads::get_libcds_hazard_pointer_data(thread_id);
        return reinterpret_cast<thread_data*>(hpx_tls_data);
    }

    /*static*/ CDS_EXPORT_API void HPXTLSManager::setTLS(thread_data* new_tls)
    {
        auto thread_id = hpx::threads::get_self_id();
        size_t hp_tls_data = reinterpret_cast<std::size_t>(new_tls);
        hpx::threads::set_libcds_hazard_pointer_data(thread_id, hp_tls_data);
    }

}}}}    // namespace cds::gc::hp::details

namespace hpx { namespace cds {
    std::atomic<std::size_t>
        libcds_wrapper::max_concurrent_attach_thread_hazard_pointer_{100};

    std::atomic<std::size_t> hpxthread_manager_wrapper::thread_counter_{0};

}}    // namespace hpx::cds
