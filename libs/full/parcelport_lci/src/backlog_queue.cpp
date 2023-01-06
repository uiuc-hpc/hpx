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
    __thread backlog_queue_t* tls_backlog_queue = nullptr;

    void push(message_ptr message)
    {
        if (tls_backlog_queue == nullptr)
        {
            // this object will never be deallocated.
            tls_backlog_queue = new backlog_queue_t;
        }
        tls_backlog_queue->lock.lock();
        if (tls_backlog_queue->messages.size() <= (size_t) message->dst_rank)
        {
            tls_backlog_queue->messages.resize(message->dst_rank + 1);
        }
        tls_backlog_queue->messages[message->dst_rank].push_back(
            HPX_MOVE(message));
        tls_backlog_queue->lock.unlock();
    }

    bool empty(int dst_rank)
    {
        if (tls_backlog_queue == nullptr)
        {
            return false;
        }
        tls_backlog_queue->lock.lock();
        if (tls_backlog_queue->messages.size() <= (size_t) dst_rank)
        {
            tls_backlog_queue->messages.resize(dst_rank + 1);
        }
        bool ret = tls_backlog_queue->messages[dst_rank].empty();
        tls_backlog_queue->lock.unlock();
        return ret;
    }

    bool background_work(size_t num_thread) noexcept
    {
        if (tls_backlog_queue == nullptr)
        {
            return false;
        }
        tls_backlog_queue->lock.lock();
        bool did_some_work = false;
        for (size_t i = 0; i < tls_backlog_queue->messages.size(); ++i)
        {
            size_t idx = (num_thread + i) % tls_backlog_queue->messages.size();
            while (!tls_backlog_queue->messages[idx].empty())
            {
                message_ptr message = tls_backlog_queue->messages[idx].front();
                bool isSent = message->send();
                if (isSent)
                {
                    tls_backlog_queue->messages[idx].pop_front();
                    did_some_work = true;
                }
                else
                {
                    break;
                }
            }
        }
        tls_backlog_queue->lock.unlock();
        return did_some_work;
    }

    void free()
    {
        if (tls_backlog_queue != nullptr)
        {
            delete tls_backlog_queue;
            tls_backlog_queue = nullptr;
        }
    }
}    // namespace hpx::parcelset::policies::lci::backlog_queue

#endif
