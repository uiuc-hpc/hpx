//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)

#include <hpx/assert.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <hpx/modules/lci_base.hpp>
#include <hpx/parcelport_lci/sender_connection.hpp>
#include <hpx/parcelport_lci/tag_provider.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace hpx { namespace parcelset { namespace policies { namespace lci {
    struct sender
    {
        using connection_type = sender_connection;
        using connection_ptr = std::shared_ptr<connection_type>;
        using connection_list = std::deque<connection_ptr>;

        using mutex_type = hpx::lcos::local::spinlock;

        sender() {}

        void run() {}

        connection_ptr create_connection(int dest, parcelset::parcelport* pp)
        {
            return std::make_shared<connection_type>(this, dest, pp);
        }

        void add(connection_ptr const& ptr)
        {
            std::unique_lock<mutex_type> l(connections_mtx_);
            connections_.push_back(ptr);
        }

        int acquire_tag()
        {
            return tag_provider_.acquire();
        }

        void send_messages(connection_ptr connection)
        {
            // Check if sending has been completed....
            if (connection->send())
            {
                error_code ec;
                util::unique_function_nonser<void(error_code const&,
                    parcelset::locality const&, connection_ptr)>
                    postprocess_handler;
                std::swap(
                    postprocess_handler, connection->postprocess_handler_);
                postprocess_handler(ec, connection->destination(), connection);
            }
            else
            {
                std::unique_lock<mutex_type> l(connections_mtx_);
                connections_.push_back(HPX_MOVE(connection));
            }
        }

        bool background_work()
        {
            connection_ptr connection;
            {
                std::unique_lock<mutex_type> l(
                    connections_mtx_, std::try_to_lock);
                if (l && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }
            bool has_work = false;
            if (connection)
            {
                send_messages(HPX_MOVE(connection));
                has_work = true;
            }
            next_free_tag();
            return has_work;
        }

    private:
        tag_provider tag_provider_;

        void next_free_tag()
        {
            int next_free = -1;
            {
                std::unique_lock<mutex_type> l(
                    next_free_tag_mtx_, std::try_to_lock);
                if (l)
                    next_free = next_free_tag_locked();
            }

            if (next_free != -1)
            {
                HPX_ASSERT(next_free > 1);
                tag_provider_.release(next_free);
            }
        }

        int next_free_tag_locked()
        {
            util::lci_environment::scoped_try_lock l;
            if (l.locked)
            {
                LCI_error_t ret = LCI_queue_pop(
                    util::lci_environment::rt_queue(), &next_free_tag_request_);
                if (ret == LCI_OK)
                {
                    return *(int*) &next_free_tag_request_.data.immediate;
                }
                else
                {
                    LCI_progress(LCI_UR_DEVICE);
                }
            }
            return -1;
        }

        mutex_type connections_mtx_;
        connection_list connections_;

        mutex_type next_free_tag_mtx_;

        LCI_request_t next_free_tag_request_;
    };

}}}}    // namespace hpx::parcelset::policies::lci

#endif
