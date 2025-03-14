//  Copyright (c) 2024 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/parcelport_lci/completion_manager/completion_manager_sync_single.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>

namespace hpx::parcelset::policies::lci {
    LCI_request_t completion_manager_sync_single::poll()
    {
        LCI_request_t request;
        request.flag = LCI_ERR_RETRY;

        bool succeed = lock.try_lock();
        if (succeed)
        {
            LCI_sync_test(sync, &request);
            if (config_t::progress_type ==
                    config_t::progress_type_t::always_poll ||
                (request.flag == LCI_ERR_RETRY &&
                    config_t::progress_type == config_t::progress_type_t::poll))
                pp_->do_progress_local();
            if (request.flag == LCI_ERR_RETRY)
                lock.unlock();
                // Otherwise we need to lock to be held until the new receive is postec
        }
        return request;
    }
}    // namespace hpx::parcelset::policies::lci
