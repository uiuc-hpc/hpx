//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)

#include <hpx/assert.hpp>
#include <hpx/plugins/parcelport/mpi/header.hpp>
#include <hpx/runtime/parcelset/decode_parcels.hpp>
#include <hpx/runtime/parcelset/parcel_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#define DEBUG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
//#define DEBUG(...)

namespace hpx { namespace parcelset { namespace policies { namespace mpi
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
          , request_(MPI_REQUEST_NULL)
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
                    util::mpi_environment::scoped_lock l;
                    MPI_Irecv(
                        buffer_.transmission_chunks_.data()
                      , static_cast<int>(
                            buffer_.transmission_chunks_.size()
                          * sizeof(buffer_type::transmission_chunk_type)
                        )
                      , MPI_BYTE
                      , src_
                      , tag_
                      , util::mpi_environment::communicator()
                      , &request_
                    );
                    request_ptr_ = &request_;
                }
            }

            state_ = rcvd_transmission_chunks;

            return receive_data(num_thread);
        }

#ifdef HPX_USE_LCI
        bool is_comm_world(MPI_Comm comm) {
            int result, error;
            error = MPI_Comm_compare(comm, MPI_COMM_WORLD, &result);
            return error == MPI_SUCCESS && (result == MPI_CONGRUENT || result == MPI_IDENT); // identical objects or identical constituents and rank order
        }

        bool receive_data(std::size_t num_thread = -1) {
            if(!request_done()) return false;

            char *piggy_back = header_.piggy_back();
            if(piggy_back) {
                std::memcpy(&buffer_.data_[0], piggy_back, buffer_.data_.size());
            } else {
                util::mpi_environment::scoped_lock l;
                //DEBUG("Rank %d: starting receive_data()", LCI_RANK);
                if(request_ptr_ != nullptr) { } //DEBUG("Rank %d: request_ptr != nullptr", LCI_RANK); }
                int src_index = src_;
                if (!is_comm_world(util::mpi_environment::communicator())) {
                    MPI_Group g0, g1;
                    MPI_Comm_group(MPI_COMM_WORLD, &g0);
                    MPI_Comm_group(util::mpi_environment::communicator(), &g1);
                    MPI_Group_translate_ranks(g1, 1, &src_, g0, &src_index);
                }
                LCI_one2one_set_empty(&sync_);
                if (false && static_cast<int>(buffer_.data_.size()) < LCI_BUFFERED_LENGTH) {
                    // Turned off using buffered messages because they fail when too many are sent in a row (need to debug in the future)
                    //DEBUG("receiving buffered data to %d from %d", LCI_RANK, src_index);
                    DEBUG("Receiving buffered data to %d from %d with tag %d", LCI_RANK, src_index, tag_);
                    LCI_recvbc(
                            buffer_.data_.data(),
                            static_cast<int>(buffer_.data_.size()),
                            src_index,
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                    );
                    //DEBUG("Rank %d received from %d", LCI_RANK, src_index);
                } else {
                    LCI_recvd(
                            buffer_.data_.data(),
                            static_cast<int>(buffer_.data_.size()),
                            src_index,
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                    );
                }
                request_ptr_ = &sync_;
                //DEBUG("Rank %d received from %d", LCI_RANK, src_index);
            }
            state_ = rcvd_data;
            return receive_chunks(num_thread);
        }
#else
        bool receive_data(std::size_t num_thread = -1)
        {
            if(!request_done()) return false;

            char *piggy_back = header_.piggy_back();
            if(piggy_back)
            {
                std::memcpy(&buffer_.data_[0], piggy_back, buffer_.data_.size());
            }
            else
            {
                util::mpi_environment::scoped_lock l;
                MPI_Irecv(
                    buffer_.data_.data()
                  , static_cast<int>(buffer_.data_.size())
                  , MPI_BYTE
                  , src_
                  , tag_
                  , util::mpi_environment::communicator()
                  , &request_
                );
                request_ptr_ = &request_;
            }

            state_ = rcvd_data;

            return receive_chunks(num_thread);
        }
#endif

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
                    util::mpi_environment::scoped_lock l;
                    MPI_Irecv(
                        c.data()
                      , static_cast<int>(c.size())
                      , MPI_BYTE
                      , src_
                      , tag_
                      , util::mpi_environment::communicator()
                      , &request_
                    );
                    request_ptr_ = &request_;
                }
            }

            state_ = rcvd_chunks;

            return send_release_tag(num_thread);
        }

        bool send_release_tag(std::size_t num_thread = -1)
        {
            if(!request_done()) return false;
            DEBUG("Starting send_release_tag()");

            performance_counters::parcels::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds() - data.time_;

            {
                util::mpi_environment::scoped_lock l;
                MPI_Isend(
                    &tag_
                  , 1
                  , MPI_INT
                  , src_
                  , 1
                  , util::mpi_environment::communicator()
                  , &request_
                );
                request_ptr_ = &request_;
            }

            decode_parcels(pp_, std::move(buffer_), num_thread);

            state_ = sent_release_tag;

            return done();
        }

        bool done()
        {
            return request_done();
        }

#ifdef HPX_USE_LCI
        bool request_done() {
            if(request_ptr_ == nullptr) return true;

            util::mpi_environment::scoped_try_lock l;
            if(!l.locked) return false;

            int completed = 0;
            int ret = 0;
            if(request_ptr_ == &sync_) {
                LCI_progress(0,1);
                completed = !LCI_one2one_test_empty(&sync_);
            } else if (request_ptr_ == &request_) {
                ret = MPI_Test(&request_, &completed, MPI_STATUS_IGNORE);
                HPX_ASSERT(ret == MPI_SUCCESS);
            }
            if(completed) {
                if(request_ptr_ == &sync_) {
                    DEBUG("Rank %d received from %d", LCI_RANK, src_);
                }
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }
#else
        bool request_done()
        {
            if(request_ptr_ == nullptr) return true;

            util::mpi_environment::scoped_try_lock l;

            if(!l.locked) return false;

            int completed = 0;
            int ret = 0;
            ret = MPI_Test(request_ptr_, &completed, MPI_STATUS_IGNORE);
            HPX_ASSERT(ret == MPI_SUCCESS);
            if(completed)
            {
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }
#endif

        hpx::chrono::high_resolution_timer timer_;

        connection_state state_;

        int src_;
        int tag_;
        header header_;
        buffer_type buffer_;

        MPI_Request request_;
#ifdef HPX_USE_LCI
        void* request_ptr_;
        LCI_syncl_t sync_;
#else
        MPI_Request *request_ptr_;
#endif
        std::size_t chunks_idx_;

        Parcelport & pp_;
    };
}}}}

#endif

