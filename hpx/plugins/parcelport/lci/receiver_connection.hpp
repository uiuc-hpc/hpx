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
#include <hpx/runtime/parcelset/decode_parcels.hpp>
#include <hpx/runtime/parcelset/parcel_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace hpx { namespace parcelset { namespace policies { namespace lci
{
    template <typename Parcelport>
    struct receiver_connection
    {
    private:
        enum connection_state
        {
            initialized
          , rcvd_transmission_chunks
          , rcvd_data
          , rcvd_chunks
          , sent_release_tag
        };

        typedef hpx::lcos::local::spinlock mutex_type;

        typedef std::vector<char>
            data_type;
        typedef parcel_buffer<data_type, data_type> buffer_type;

    public:
        receiver_connection(
            int src
          , header h
          , Parcelport & pp
        )
          : state_(initialized)
          , src_(src)
          , tag_(h.tag())
          , header_(h)
          , request_ptr_(nullptr)
          , chunks_idx_(0)
          , pp_(pp)
        {
            header_.assert_valid();

            performance_counters::parcels::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds();
            data.bytes_ = static_cast<std::size_t>(header_.numbytes());

            buffer_.data_.resize(static_cast<std::size_t>(header_.size()));
            buffer_.num_chunks_ = header_.num_chunks();

            LCI_sync_create(LCI_UR_DEVICE, 1, &sync_);
        }

        bool receive(std::size_t num_thread = -1)
        {
            switch (state_)
            {
                case initialized:
                    return receive_transmission_chunks(num_thread);
                case rcvd_transmission_chunks:
                    return receive_data(num_thread);
                case rcvd_data:
                    return receive_chunks(num_thread);
                case rcvd_chunks:
                    return send_release_tag(num_thread);
                case sent_release_tag:
                    return done();
                default:
                    HPX_ASSERT(false);
            }
            return false;
        }
        
        int get_src_rank() {
            return src_;
        }

        bool receive_transmission_chunks(std::size_t num_thread = -1)
        {
            // determine the size of the chunk buffer
            std::size_t num_zero_copy_chunks =
                static_cast<std::size_t>(
                    static_cast<std::uint32_t>(buffer_.num_chunks_.first));
            std::size_t num_non_zero_copy_chunks =
                static_cast<std::size_t>(
                    static_cast<std::uint32_t>(buffer_.num_chunks_.second));
            buffer_.transmission_chunks_.resize(
                num_zero_copy_chunks + num_non_zero_copy_chunks
            );
            if(num_zero_copy_chunks != 0)
            {
                buffer_.chunks_.resize(num_zero_copy_chunks);
                {
                    util::lci_environment::scoped_lock l;

                    lbuf_.address = buffer_.transmission_chunks_.data();
                    LCI_memory_register(
                        LCI_UR_DEVICE, 
                        lbuf_.address, 
                        static_cast<int>(buffer_.transmission_chunks_.size() * sizeof(buffer_type::transmission_chunk_type)), 
                        &lbuf_.segment);
                    lbuf_.length = static_cast<int>(buffer_.transmission_chunks_.size() * sizeof(buffer_type::transmission_chunk_type));
                    while(LCI_recvl(
                        util::lci_environment::lci_endpoint(),
                        lbuf_,
                        get_src_rank(),
                        tag_,
                        sync_,
                        NULL
                    ) != LCI_OK) { LCI_progress(LCI_UR_DEVICE); }

                    request_ptr_ = &sync_;
                }
            }

            state_ = rcvd_transmission_chunks;

            return receive_data(num_thread);
        }

        bool receive_data(std::size_t num_thread = -1) {
            if(!request_done()) return false;

            char *piggy_back = header_.piggy_back();
            if(piggy_back) {
                std::memcpy(&buffer_.data_[0], piggy_back, buffer_.data_.size());
            } else {
                util::lci_environment::scoped_lock l;

                lbuf_.address = buffer_.data_.data();
                LCI_memory_register(LCI_UR_DEVICE, lbuf_.address, static_cast<int>(buffer_.data_.size()), &lbuf_.segment);
                lbuf_.length = static_cast<int>(buffer_.data_.size());
                while(LCI_recvl(
                    util::lci_environment::lci_endpoint(),
                    lbuf_,
                    get_src_rank(),
                    tag_,
                    sync_,
                    NULL
                ) != LCI_OK) { LCI_progress(LCI_UR_DEVICE); }

                request_ptr_ = &sync_;
            }
            state_ = rcvd_data;

            return receive_chunks(num_thread);
        }

        bool receive_chunks(std::size_t num_thread = -1)
        {
            while(chunks_idx_ < buffer_.chunks_.size())
            {
                if(!request_done()) return false;

                std::size_t idx = chunks_idx_++;
                std::size_t chunk_size = buffer_.transmission_chunks_[idx].second;

                data_type & c = buffer_.chunks_[idx];
                c.resize(chunk_size);
                {
                    util::lci_environment::scoped_lock l;

                    lbuf_.address = c.data();
                    LCI_memory_register(
                        LCI_UR_DEVICE,
                        lbuf_.address,
                        static_cast<int>(c.size()),
                        &lbuf_.segment);
                    lbuf_.length = static_cast<int>(c.size());
                    while(LCI_recvl(
                        util::lci_environment::lci_endpoint(),
                        lbuf_,
                        get_src_rank(),
                        tag_,
                        sync_,
                        NULL
                    ) != LCI_OK) { LCI_progress(LCI_UR_DEVICE); }

                    request_ptr_ = &sync_;
                }
            }

            state_ = rcvd_chunks;

            return send_release_tag(num_thread);
        }

        bool request_done() {
            if(request_ptr_ == nullptr) return true;

            util::lci_environment::scoped_try_lock l;
            if(!l.locked) return false;

            int completed = 0;
            if(request_ptr_ == &sync_) { // TODO: can I just do this unconditionally, or is it possible that the ptr is still null?
                LCI_progress(LCI_UR_DEVICE);
                completed = (LCI_sync_test(sync_, NULL) == LCI_OK);
                if(completed) {
                    LCI_memory_deregister(&lbuf_.segment);
                }
            }
            if(completed) {
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }

        bool send_release_tag(std::size_t num_thread = -1)
        {
            if(!request_done()) return false;

            performance_counters::parcels::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds() - data.time_;

            {
                util::lci_environment::scoped_lock l;
                *(int*)&short_rt_ = tag_;
                while(LCI_puts(
                    util::lci_environment::rt_endpoint(),
                    short_rt_,
                    get_src_rank(),
                    1,
                    LCI_DEFAULT_COMP_REMOTE
                ) != LCI_OK) { LCI_progress(LCI_UR_DEVICE); }
            }

            decode_parcels(pp_, std::move(buffer_), num_thread);

            state_ = sent_release_tag;

            return done();
        }

        bool done()
        {
            return request_done();
        }

        hpx::chrono::high_resolution_timer timer_;

        connection_state state_;

        int src_;
        int tag_;
        header header_;
        buffer_type buffer_;

        void* request_ptr_;
        LCI_comp_t sync_;
        LCI_lbuffer_t lbuf_;
        LCI_short_t short_rt_;
        std::size_t chunks_idx_;

        Parcelport & pp_;
    };
}}}}

#endif

