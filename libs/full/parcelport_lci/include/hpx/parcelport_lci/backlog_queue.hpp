//  Copyright (c) 2014-2023 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)

#include <deque>
#include <vector>

namespace hpx::parcelset::policies::lci {
    struct sender_connection;
    namespace backlog_queue {
        using message_type = sender_connection;
        using message_ptr = std::shared_ptr<message_type>;
        struct backlog_queue_t
        {
            // pending messages per destination
            std::vector<std::deque<message_ptr>> messages;
            hpx::spinlock lock;
        };

        extern __thread backlog_queue_t* tls_backlog_queue;

        void push(message_ptr message);
        bool empty(int dst_rank);
        bool background_work(size_t num_thread) noexcept;
        void free();
    }    // namespace backlog_queue
}    // namespace hpx::parcelset::policies::lci

#endif
