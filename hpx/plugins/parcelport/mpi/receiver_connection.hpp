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
#include <unistd.h>

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
            temp_buffer = nullptr;
            header_.assert_valid();

            performance_counters::parcels::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds();
            data.bytes_ = static_cast<std::size_t>(header_.numbytes());

            buffer_.data_.resize(static_cast<std::size_t>(header_.size()));
            buffer_.num_chunks_ = header_.num_chunks();

            chunk_tag_ = 0;
            DEBUG("Creating a new receiver_connection, num_chunks first=%d, num_chunks second=%d", buffer_.num_chunks_.first, buffer_.num_chunks_.second);
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
#ifdef HPX_USE_LCI // LCI version of receive_transmission_chunks()
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
                    LCI_one2one_set_empty(&sync_);
                    LCI_recvd(
                            buffer_.transmission_chunks_.data(),
                            static_cast<int>(
                                buffer_.transmission_chunks_.size()
                                * sizeof(buffer_type::transmission_chunk_type)
                            ),
                            src_, // TODO: account for communicators
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                    );
                    request_ptr_ = &sync_;
                }
                DEBUG("Reciever transmission chunks with tag_=%d, length=%d", tag_,static_cast<int>(buffer_.transmission_chunks_.size()*sizeof(buffer_type::transmission_chunk_type))); 
            }

            state_ = rcvd_transmission_chunks;

            return receive_data(num_thread);
        }
#else // MPI version of receive_transmission_chunks()
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
                    DEBUG("MPI receiving transmission chunks tag=%d, num=%lu", tag_, num_zero_copy_chunks + num_non_zero_copy_chunks);
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
#endif

#ifdef HPX_USE_LCI // helper functions for LCI communication
        bool is_comm_world(MPI_Comm comm) {
            int result, error;
            error = MPI_Comm_compare(comm, MPI_COMM_WORLD, &result);
            return error == MPI_SUCCESS && (result == MPI_CONGRUENT || result == MPI_IDENT); // identical objects or identical constituents and rank order
        }
        
        int get_src_index() {
            int index = src_;
            if(!is_comm_world(util::mpi_environment::communicator())) {
                MPI_Group g0, g1;
                MPI_Comm_group(MPI_COMM_WORLD, &g0);
                MPI_Comm_group(util::mpi_environment::communicator(), &g1);
                MPI_Group_translate_ranks(g1, 1, &src_, g0, &index);
            }
            return index;
        }
#endif

#ifdef HPX_USE_LCI // LCI receive_data()
        bool receive_data(std::size_t num_thread = -1) {
            if(!request_done()) return false;

            char *piggy_back = header_.piggy_back();
            if(piggy_back) {
                std::memcpy(&buffer_.data_[0], piggy_back, buffer_.data_.size());
            } else {
                util::mpi_environment::scoped_lock l;
                int src_index = get_src_index();
                LCI_one2one_set_empty(&sync_);
                if (false && static_cast<int>(buffer_.data_.size()) < LCI_BUFFERED_LENGTH) {
                    // Turned off using buffered messages because they fail when too many are sent in a row (need to debug in the future)
                    LCI_recvbc(
                            buffer_.data_.data(),
                            static_cast<int>(buffer_.data_.size()),
                            src_index,
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                    );
                } else {
                    LCI_recvd(
                            buffer_.data_.data(),
                            static_cast<int>(buffer_.data_.size()),
                            src_index,
                            //tag_*10,
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                    );
                    chunk_tag_ = 2;
                }
                request_ptr_ = &sync_;
            }
            DEBUG("Setting state_ to rcvd_data for tag=%d, length=%d", tag_, static_cast<int>(buffer_.data_.size()));
            state_ = rcvd_data;
            return receive_chunks(num_thread);
        }
#endif

#ifdef HPX_USE_LCI // LCI receive_chunks()
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
                    // May eventually want to use buffered communication here, but using direct because it is more similar to MPI
                    LCI_one2one_set_empty(&sync_);
                    //try setting all of the data to 0 before we receive it
                    memset(c.data(), 0, static_cast<int>(c.size()));

                    // make a temp buffer for receiving
                    //if(temp_buffer) {
                    //    DEBUG("ERROR: temp_buffer is already allocated when trying to send a chunk");
                    //} else {
                    //    temp_buffer = (uint8_t*)calloc(c.size(), sizeof(uint8_t));
                    //}

                    print_hash("Chunk before receiving", static_cast<int>(c.size()), (uint8_t*)c.data(), tag_, chunks_idx_);
                    LCI_error_t err = LCI_progress(0,1);
                    if(err != LCI_OK) {
                        DEBUG("receiver send_chunks() return error in LCI_progress");
                    }
                    err = LCI_recvd(
                        //temp_buffer,
                        c.data(),
                        static_cast<int>(c.size()),
                        //get_src_index(),
                        src_,
                        tag_,
                        util::mpi_environment::lci_endpoint(),
                        &sync_
                    );
                    if(err != LCI_OK) DEBUG("receiver send_chunks receive error");
                    chunk_tag_ = 1;
                    DEBUG("Receiver receiving chunks with tag_=%d, length=%d", tag_, static_cast<int>(c.size()));
                    request_ptr_ = &sync_;
                }
            }

            state_ = rcvd_chunks;

            return send_release_tag(num_thread);
        }

#endif

#ifndef HPX_USE_LCI // MPI receive_data()
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
                DEBUG("Receiving data with MPI, tag=%d", tag_);
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

            chunk_tag_ = 0; // tag_*10+1;
            //DEBUG("Receiver setting chunk_tag_ to %d", chunk_tag_);

            return receive_chunks(num_thread);
        }
        
#endif

#ifndef HPX_USE_LCI // MPI receive_chunks()
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
                    //DEBUG("Starting to receive chunk %lu with tag %d", chunks_idx_, tag_);
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
#endif

        bool send_release_tag(std::size_t num_thread = -1)
        {
            if(!request_done()) return false;

            performance_counters::parcels::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds() - data.time_;

            {
                util::mpi_environment::scoped_lock l;
                DEBUG("Sending release tag with MPI, tag=%d", tag_);
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
            chunk_tag_=3;
            DEBUG("Receiver calling done()");
            return request_done();
        }

#if defined(HPX_USE_LCI_) // LCI (and MPI) request_done()
        bool request_done() {
            if(request_ptr_ == nullptr) return true;

            util::mpi_environment::scoped_try_lock l;
            if(!l.locked) return false;

            int completed = 0;
            if(request_ptr_ == &sync_) {
                LCI_error_t err = LCI_progress(0,1);
                if(err != LCI_OK) {
                    DEBUG("recevier request_done() LCI_progress failed");
                }
                completed = !LCI_one2one_test_empty(&sync_);
                if(completed) {
                    DEBUG("Receiver request: status=%d, rank=%d, tag=%d, len=%lu, type=%d", sync_.request.status, sync_.request.rank, sync_.request.tag, sync_.request.length, sync_.request.type);
                    LCI_one2one_set_empty(&sync_);
                }
            } else if (request_ptr_ == &request_) {
                int ret = MPI_Test(&request_, &completed, MPI_STATUS_IGNORE);
                HPX_ASSERT(ret == MPI_SUCCESS);
            }
            if(completed) {
                if(chunk_tag_==2) {
                    chunk_tag_ = 0;
                    DEBUG("Receiver received LCI data for tag %d", tag_);
                    print_hash("Receiver data",buffer_.data_.size(),(uint8_t*)buffer_.data_.data(),tag_,0);
                }
                if(chunk_tag_==3) {
                    chunk_tag_ = 0;
                    DEBUG("Receiver done");
                }
                if(true && chunk_tag_==1) {
                    //sleep(5);
                    //LCI_progress(0,1);
                    chunk_tag_ = 0;
                    // copy from temp_buffer into the real buffer
                    //print_hash("Receiver temp_buffer", buffer_.chunks_[chunks_idx_-1].size(), temp_buffer, tag_, chunks_idx_);
                    //memcpy(buffer_.chunks_[chunks_idx_-1].data(), temp_buffer, buffer_.chunks_[chunks_idx_-1].size());
                    //free(temp_buffer);
                    //temp_buffer = nullptr;
                    //DEBUG("Received with tag_=%d and chunks_idx_=%lu", tag_, chunks_idx_);
                    /*
                    unsigned src_buf = 1;
                    LCI_progress(0,1);
                    while(LCI_sendbc(&src_buf, sizeof(unsigned), src_, tag_, util::mpi_environment::lci_endpoint()) != LCI_OK)
                        LCI_progress(0,1);
                    */
                    print_hash("Receiver",buffer_.chunks_[chunks_idx_-1].size(),(uint8_t*)buffer_.chunks_[chunks_idx_-1].data(),tag_,chunks_idx_);
                    int zero_count = 0;
                    for(size_t i=0; i<buffer_.chunks_[chunks_idx_-1].size(); i++) {
                        if((uint8_t*)buffer_.chunks_[chunks_idx_-1].data()[i] == 0) {
                            zero_count++;
                        }
                    }
                    DEBUG("Zero count for tag %d = %d", tag_, zero_count);
                    //print_hash("Receiver",buffer_.chunks_[chunks_idx_-1].size(),(uint8_t*)temp_buffer,tag_,chunks_idx_);

                    /*
                    uint8_t hash[9];
                    for(int i=0; i<9; i++)
                        hash[i] = NULL;
                    const uint8_t k[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
                    uint8_t* in_data = (uint8_t*)temp_buffer;
                    //uint8_t* in_data = (uint8_t*)buffer_.chunks_[chunks_idx_-1].data();
                    size_t in_count = buffer_.chunks_[chunks_idx_-1].size();
                    size_t out_size = 8;
                    siphash(in_data, in_count, k, hash, out_size);
                    DEBUG("Receiver:\tsrc=%d,\ttag_=%3d,\tchunks_idx_=%lu,\thash=%8s,\tsize=%7d,\taddr=%14p", src_, tag_, chunks_idx_-1, hash, static_cast<int>(buffer_.chunks_[chunks_idx_-1].size()), (void*)buffer_.chunks_[chunks_idx_-1].data());
                    */

                    /*
                    int error_count = 0;
                    int error_len = 0;
                    int max_error_len = 0;
                    int last_index = 0;
                    for(int i=0; i<static_cast<int>(buffer_.chunks_[chunks_idx_-1].size()); i++) {
                        if(buffer_.chunks_[chunks_idx_-1].data()[i] != temp_buffer[i]) {
                            error_count++;
                            error_len++;
                            if(error_count == 1) {
                                DEBUG("ERROR: LCI and MPI messages do not match");
                            }
                        } else {
                            if(error_len > max_error_len) { max_error_len = error_len; last_index = i-1;}
                            error_len = 0;
                        }
                    }
                    if(error_len > max_error_len) { max_error_len = error_len; last_index = static_cast<int>(buffer_.chunks_[chunks_idx_-1].size()); }
                    DEBUG("error_count = %d out of %d, max consecutive = %d, ending at index %d", error_count, static_cast<int>(buffer_.chunks_[chunks_idx_-1].size()), max_error_len, last_index);
                    */

                    //DEBUG("Receiver");
                    //DEBUG("Receiver sent confirmation to sender.");

                    //DEBUG("Received chunk with tag %d", tag_);
                    //
                    //LCI_PM_barrier();
                    //lc_hash_dump(util::mpi_environment::lci_mt(), 1 << 16);
                    //
                    /*LCI_ivalue_t src_buf = 30;
                    LCI_sendi(src_buf, src_, tag_*10, util::mpi_environment::lci_endpoint());*/
                    //MPI_Barrier(MPI_COMM_WORLD);
                    //DEBUG("Receiver sent to sender");

                } else {
                    //DEBUG("MPI receive complete");
                }
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }
#elif defined(HPX_USE_LCI) // LCI request_done() simplified (no printing)
        bool request_done()
        {
            if(request_ptr_ == nullptr) return true;

            util::mpi_environment::scoped_try_lock l;

            if(!l.locked) return false;

            if(request_ptr_ == &request_) {
                int completed = 0;
                int ret = 0;
                ret = MPI_Test(&request_, &completed, MPI_STATUS_IGNORE);
                HPX_ASSERT(ret == MPI_SUCCESS);
                if(completed)
                {
                    request_ptr_ = nullptr;
                    return true;
                }
                return false;
            } else if(request_ptr_ == &sync_) {
                int completed = 0;
                completed = !LCI_one2one_test_empty(&sync_);
                if(completed)
                {
                    LCI_one2one_set_empty(&sync_);
                    request_ptr_ = nullptr;
                    return true;
                } else {
                    LCI_progress(0,1);
                }
                return false;
            } else {
                DEBUG("ERROR: Bypassed the original request_ptr_ check and found it was NULL later");
                return true;
            }
            return false;
        }

#else // MPI only request_done()
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
        uint8_t* temp_buffer;
        LCI_syncl_t sync_;
        int chunk_tag_;
#else
        MPI_Request *request_ptr_;
#endif
        std::size_t chunks_idx_;

        Parcelport & pp_;
    };
}}}}

#endif

