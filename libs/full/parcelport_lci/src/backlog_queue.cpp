//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)
#include <hpx/parcelport_lci/parcelport_lci_include.hpp>

namespace hpx::parcelset::policies::lci::backlog_queue {
    thread_local backlog_queue_t tls_backlog_queue;

    void push(message_ptr message)
    {
        if (tls_backlog_queue.messages.size() <= (size_t) message->dst_rank)
        {
            tls_backlog_queue.messages.resize(message->dst_rank + 1);
        }
        tls_backlog_queue.messages[message->dst_rank].push_back(
            HPX_MOVE(message));
    }

    bool empty(int dst_rank)
    {
        if (tls_backlog_queue.messages.size() <= (size_t) dst_rank)
        {
            tls_backlog_queue.messages.resize(dst_rank + 1);
        }
        bool ret = tls_backlog_queue.messages[dst_rank].empty();
        return ret;
    }

    bool background_work(size_t num_thread) noexcept
    {
        bool did_some_work = false;
        for (size_t i = 0; i < tls_backlog_queue.messages.size(); ++i)
        {
            size_t idx = (num_thread + i) % tls_backlog_queue.messages.size();
            while (idx < tls_backlog_queue.messages.size() &&
                !tls_backlog_queue.messages[idx].empty())
            {
                message_ptr message = tls_backlog_queue.messages[idx].front();
                bool needCallDone = message->isEager();
                bool isSent = message->send(false);
                if (isSent)
                {
                    tls_backlog_queue.messages[idx].pop_front();
                    did_some_work = true;
                    if (needCallDone) {
                        message->done();
                        // it can context switch to another thread at this point
                    }
                }
                else
                {
                    break;
                }
            }
        }
        return did_some_work;
    }
}    // namespace hpx::parcelset::policies::lci::backlog_queue

#endif
