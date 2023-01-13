//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace hpx::parcelset::policies::lci {
    struct sender_connection;
    struct sender
    {
        using connection_type = sender_connection;
        using connection_ptr = std::shared_ptr<connection_type>;

        sender() noexcept {}

        void run() noexcept {}

        connection_ptr create_connection(int dest, parcelset::parcelport* pp);

        bool background_work(size_t num_thread) noexcept;

        // connectionless interface
        using buffer_type = std::vector<char>;
        using chunk_type = serialization::serialization_chunk;
        using parcel_buffer_type = parcel_buffer<buffer_type, chunk_type>;
        using callback_fn_type =
            hpx::move_only_function<void(error_code const&)>;

        static bool send(parcelset::parcelport* pp,
            parcelset::locality const& dest, parcel_buffer_type buffer,
            callback_fn_type&& callbackFn);
    };

}    // namespace hpx::parcelset::policies::lci

#endif
