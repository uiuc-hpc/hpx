
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
#include <hpx/plugins/parcelport/lci/header.hpp>
#include <hpx/plugins/parcelport/lci/receiver_connection.hpp>

#include <algorithm>
#include <deque>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

namespace hpx { namespace parcelset { namespace policies { namespace lci {
    template <typename Parcelport>
    struct receiver
    {
        typedef hpx::lcos::local::spinlock mutex_type;
        typedef std::list<std::pair<int, header>> header_list;
        typedef std::set<std::pair<int, int>> handles_header_type;
        typedef receiver_connection<Parcelport> connection_type;
        typedef std::shared_ptr<connection_type> connection_ptr;
        typedef std::deque<connection_ptr> connection_list;

        receiver(Parcelport& pp)
          : pp_(pp)
        {
        }

        void run() {}

        bool background_work()
        {
            // We first try to accept a new connection
            connection_ptr connection = accept();

            // If we don't have a new connection, try to handle one of the
            // already accepted ones.
            if (!connection)
            {
                std::unique_lock<mutex_type> l(
                    connections_mtx_, std::try_to_lock);
                if (l && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }

            if (connection)
            {
                receive_messages(HPX_MOVE(connection));
                return true;
            }

            return false;
        }

        void receive_messages(connection_ptr connection)
        {
            if (!connection->receive())
            {
                std::unique_lock<mutex_type> l(connections_mtx_);
                connections_.push_back(HPX_MOVE(connection));
            }
        }

        connection_ptr accept()
        {
            std::unique_lock<mutex_type> l(headers_mtx_, std::try_to_lock);
            if (l)
                return accept_locked(l);
            return connection_ptr();
        }

        connection_ptr accept_locked(std::unique_lock<mutex_type>& header_lock)
        {
            connection_ptr res;
            util::lci_environment::scoped_try_lock l;

            if (l.locked)
            {
                LCI_request_t request;
                LCI_error_t ret =
                    LCI_queue_pop(util::lci_environment::h_queue(), &request);
                if (ret == LCI_OK)
                {
                    header h = *(header*) (request.data.mbuffer.address);
                    h.assert_valid();
                    l.unlock();
                    header_lock.unlock();
                    res.reset(new connection_type(request.rank, h, pp_));
                    LCI_mbuffer_free(request.data.mbuffer);
                    return res;
                }
                else
                {
                    LCI_progress(LCI_UR_DEVICE);
                }
            }
            return res;
        }

        Parcelport& pp_;

        mutex_type headers_mtx_;
        header rcv_header_;

        mutex_type handles_header_mtx_;
        handles_header_type handles_header_;

        mutex_type connections_mtx_;
        connection_list connections_;
    };

}}}}    // namespace hpx::parcelset::policies::lci

#endif
