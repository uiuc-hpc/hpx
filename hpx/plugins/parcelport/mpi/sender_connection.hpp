//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)

#include <hpx/assert.hpp>
#include <hpx/functional/unique_function.hpp>
#include <hpx/plugins/parcelport/mpi/header.hpp>
#include <hpx/plugins/parcelport/mpi/locality.hpp>
#include <hpx/runtime/parcelset/detail/gatherer.hpp>
#include <hpx/runtime/parcelset/parcelport.hpp>
#include <hpx/runtime/parcelset/parcelport_connection.hpp>
#include <hpx/runtime/parcelset_fwd.hpp>
#include <hpx/timing/high_resolution_clock.hpp>

#include <cstddef>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#define DEBUG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
//#define DEBUG(...)
//#undef HPX_USE_LCI
#define MAX_PART 400000
//#define LCI_CHUNK_PARTS

// only for debuggin purposes
#include <iostream>
#include <fstream>
#include <string>

#ifndef SIPHASH_
#define SIPHASH_
#ifdef __cplusplus
extern "C" {
#endif

int siphash(const uint8_t *in, const size_t inlen, const uint8_t *k,
            uint8_t *out, const size_t outlen);

#ifdef __cplusplus
}
#endif

/* default: SipHash-2-4 */
#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)                                                        \
    (p)[0] = (uint8_t)((v));                                                   \
    (p)[1] = (uint8_t)((v) >> 8);                                              \
    (p)[2] = (uint8_t)((v) >> 16);                                             \
    (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)                                                        \
    U32TO8_LE((p), (uint32_t)((v)));                                           \
    U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p)                                                           \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |                        \
     ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) |                 \
     ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |                 \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define SIPROUND                                                               \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 32);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 16);                                                     \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 21);                                                     \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 17);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 32);                                                     \
    } while (0)

#ifdef DEBUG_SIPHASH
#define TRACE                                                                  \
    do {                                                                       \
        printf("(%3d) v0 %08x %08x\n", (int)inlen, (uint32_t)(v0 >> 32),       \
               (uint32_t)v0);                                                  \
        printf("(%3d) v1 %08x %08x\n", (int)inlen, (uint32_t)(v1 >> 32),       \
               (uint32_t)v1);                                                  \
        printf("(%3d) v2 %08x %08x\n", (int)inlen, (uint32_t)(v2 >> 32),       \
               (uint32_t)v2);                                                  \
        printf("(%3d) v3 %08x %08x\n", (int)inlen, (uint32_t)(v3 >> 32),       \
               (uint32_t)v3);                                                  \
    } while (0)
#else
#define TRACE
#endif

int siphash(const uint8_t *in, const size_t inlen, const uint8_t *k,
            uint8_t *out, const size_t outlen) {

    assert((outlen == 8) || (outlen == 16));
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t k0 = U8TO64_LE(k);
    uint64_t k1 = U8TO64_LE(k + 8);
    uint64_t m;
    int i;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    if (outlen == 16)
        v1 ^= 0xee;

    for (; in != end; in += 8) {
        m = U8TO64_LE(in);
        v3 ^= m;

        TRACE;
        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }

    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48;
    case 6:
        b |= ((uint64_t)in[5]) << 40;
    case 5:
        b |= ((uint64_t)in[4]) << 32;
    case 4:
        b |= ((uint64_t)in[3]) << 24;
    case 3:
        b |= ((uint64_t)in[2]) << 16;
    case 2:
        b |= ((uint64_t)in[1]) << 8;
    case 1:
        b |= ((uint64_t)in[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;

    TRACE;
    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    if (outlen == 16)
        v2 ^= 0xee;
    else
        v2 ^= 0xff;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);

    if (outlen == 8)
        return 0;

    v1 ^= 0xdd;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out + 8, b);

    return 0;
}

void print_hash(const char* name, size_t in_count, uint8_t* data, int tag_, size_t chunks_idx_, int my_rank=-1, int other_rank=-1) {
    uint8_t hash[9];
    for(int i=0; i<9; i++)
        hash[i] = NULL;
    size_t out_size = 8;
    const uint8_t k[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    siphash(data, in_count, k, hash, out_size);
    DEBUG("%s:\ttag_=%3d,\trank=%d,\tother rank=%d,\tchunks_idx_=%lu,\thash=%8s,\tsize=%7d,\taddr=%14p", name, tag_, my_rank, other_rank, chunks_idx_, hash, static_cast<int>(in_count), (void*)data);

}
#endif

namespace hpx { namespace parcelset { namespace policies { namespace mpi
{
    struct sender;
    struct sender_connection;

    int acquire_tag(sender *);
    void add_connection(sender *, std::shared_ptr<sender_connection> const&);

    struct sender_connection
      : parcelset::parcelport_connection<
            sender_connection
          , std::vector<char>
        >
    {
    private:
        typedef sender sender_type;

        typedef util::function_nonser<
            void(std::error_code const&, parcel const&)
        > write_handler_type;

        typedef std::vector<char> data_type;

        enum connection_state
        {
            initialized
          , sent_header
          , sent_transmission_chunks
          , sent_data
          , sent_chunks
        };

        typedef
            parcelset::parcelport_connection<sender_connection, data_type>
            base_type;

    public:
        sender_connection(sender_type* s, int dst, parcelset::parcelport* pp)
          : state_(initialized)
          , sender_(s)
          , tag_(-1)
          , dst_(dst)
          , request_(MPI_REQUEST_NULL)
          , request_ptr_(nullptr)
          , chunks_idx_(0)
          , ack_(0)
          , pp_(pp)
          , there_(parcelset::locality(locality(dst_)))
        { }

        parcelset::locality const& destination() const
        {
            return there_;
        }

        void verify_(parcelset::locality const& /* parcel_locality_id */) const
        {
        }

        template <typename Handler, typename ParcelPostprocess>
        void async_write(Handler && handler, ParcelPostprocess && parcel_postprocess)
        {
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!postprocess_handler_);
            HPX_ASSERT(!buffer_.data_.empty());
            buffer_.data_point_.time_ = hpx::chrono::high_resolution_clock::now();
            request_ptr_ = nullptr;
            chunks_idx_ = 0;
            tag_ = acquire_tag(sender_);
            header_ = header(buffer_, tag_);
            header_.assert_valid();

            state_ = initialized;

            handler_ = std::forward<Handler>(handler);

            if(!send())
            {
                postprocess_handler_
                    = std::forward<ParcelPostprocess>(parcel_postprocess);
                add_connection(sender_, shared_from_this());
            }
            else
            {
                HPX_ASSERT(!handler_);
                error_code ec;
                parcel_postprocess(ec, there_, shared_from_this());
            }
        }

        bool send()
        {
            switch(state_)
            {
                case initialized:
                    return send_header();
                case sent_header:
                    return send_transmission_chunks();
                case sent_transmission_chunks:
                    return send_data();
                case sent_data:
                    return send_chunks();
                case sent_chunks:
                    return done();
                default:
                    HPX_ASSERT(false);
            }

            return false;
        }

        bool send_header() // TODO: adapt this to use LCI
        {
            {
                util::mpi_environment::scoped_lock l;
                HPX_ASSERT(state_ == initialized);
                HPX_ASSERT(request_ptr_ == nullptr);
                char hostname[256];
                gethostname(hostname, sizeof(hostname));
                MPI_Isend(
                    header_.data()
                  , header_.data_size_
                  , MPI_BYTE
                  , dst_
                  , 0
                  , util::mpi_environment::communicator()
                  , &request_
                );
                request_ptr_ = &request_;
            }

            state_ = sent_header;
            return send_transmission_chunks();
        }

#ifdef HPX_USE_LCI
        bool is_comm_world(MPI_Comm comm) {
            int result, error;
            error = MPI_Comm_compare(comm, MPI_COMM_WORLD, &result);
            return error == MPI_SUCCESS && (result == MPI_CONGRUENT || result == MPI_IDENT); // identical objects or identical constituents and rank order
        }

        int get_dst_index() {
            int index = dst_;
            if (!is_comm_world(util::mpi_environment::communicator())) {
                MPI_Group g0, g1;
                MPI_Comm_group(MPI_COMM_WORLD, &g0);
                MPI_Comm_group(util::mpi_environment::communicator(), &g1);
                MPI_Group_translate_ranks(g1, 1, &dst_, g0, &index);
            }
            return index;
        }

        bool send_transmission_chunks()
        {
            HPX_ASSERT(state_ == sent_header);
            HPX_ASSERT(request_ptr_ != nullptr);
            if(!request_done()) return false;

            HPX_ASSERT(request_ptr_ == nullptr);

            std::vector<typename parcel_buffer_type::transmission_chunk_type>& chunks =
                buffer_.transmission_chunks_;
            if(!chunks.empty())
            {
                util::mpi_environment::scoped_lock l;
                LCI_one2one_set_empty(&sync_);
                while(LCI_sendd(
                            chunks.data(),
                            static_cast<int>(
                                chunks.size()
                                 * sizeof(parcel_buffer_type::transmission_chunk_type)
                            ),
                            get_dst_index(),
                            tag_,
                            util::mpi_environment::lci_endpoint(),
                            &sync_
                      ) != LCI_OK ){ LCI_progress(0,1); }

                request_ptr_ = &sync_;
            }

            state_ = sent_transmission_chunks;
            return send_data();
        }

        bool send_data()
        {
            HPX_ASSERT(state_ == sent_transmission_chunks);
            if(!request_done()) return false;

            if(!header_.piggy_back()) {
                util::mpi_environment::scoped_lock l;
                if (false && static_cast<int>(buffer_.data_.size()) < LCI_BUFFERED_LENGTH) {
                    // Turned off using buffered messages because they fail when too many are sent in a row (need to debug in the future)
                    while(LCI_sendbc(
                                buffer_.data_.data(),
                                static_cast<int>(buffer_.data_.size()),
                                get_dst_index(),
                                tag_,
                                util::mpi_environment::lci_endpoint()
                          ) != LCI_OK ) {LCI_progress(0,1); }
                } else { // direct send
                    LCI_one2one_set_empty(&sync_);
                    while(LCI_sendd(
                                buffer_.data_.data(),
                                static_cast<int>(buffer_.data_.size()),
                                get_dst_index(),
                                tag_,
                                util::mpi_environment::lci_endpoint(),
                                &sync_
                          ) != LCI_OK ){ LCI_progress(0,1); }
                    request_ptr_ = &sync_;
                    //print_hash("Sender data",buffer_.data_.size(),(uint8_t*)buffer_.data_.data(),tag_,0, LCI_RANK, dst_);
                }
            }
            state_ = sent_data;
            return send_chunks();
        }

        bool send_chunks()
        {
            HPX_ASSERT(state_ == sent_data);

            while(chunks_idx_ < buffer_.chunks_.size())
            {
                serialization::serialization_chunk& c = buffer_.chunks_[chunks_idx_];
                if(c.type_ == serialization::chunk_type_pointer)
                {
                    if(!request_done()) return false;
                    else
                    {
                        util::mpi_environment::scoped_lock l;
                        LCI_one2one_set_empty(&sync_);
                        while(LCI_sendd(
                                const_cast<void *>(c.data_.cpos_),
                                static_cast<int>(c.size_),
                                get_dst_index(),
                                tag_,
                                util::mpi_environment::lci_endpoint(),
                                &sync_
                        ) != LCI_OK) { 
                            LCI_progress(0,1);
                        }

                        request_ptr_ = &sync_;
                        //print_hash("Sender",buffer_.chunks_[chunks_idx_].size_,(uint8_t*)buffer_.chunks_[chunks_idx_].data_.cpos_,tag_,chunks_idx_, LCI_RANK, dst_);
                    }
                 }

                chunks_idx_++;
            }
            state_ = sent_chunks;

            return done();
        }

        bool request_done() {
            if(request_ptr_ == nullptr) return true;

            util::mpi_environment::scoped_try_lock l;
            if(!l.locked) return false;

            int completed = 0;
            if(request_ptr_ == &sync_) {
                LCI_progress(0,1);
                completed = !LCI_one2one_test_empty(&sync_);
                if(completed) {
                    LCI_one2one_set_empty(&sync_);
                }
            } else if (request_ptr_ == &request_) {
                int ret = MPI_Test(&request_, &completed, MPI_STATUS_IGNORE);
                HPX_ASSERT(ret == MPI_SUCCESS);
            }
            if(completed) {                
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }
#endif

#ifndef HPX_USE_LCI
        bool send_transmission_chunks()
        {
            HPX_ASSERT(state_ == sent_header);
            HPX_ASSERT(request_ptr_ != nullptr);
            if(!request_done()) return false;

            HPX_ASSERT(request_ptr_ == nullptr);

            std::vector<typename parcel_buffer_type::transmission_chunk_type>& chunks =
                buffer_.transmission_chunks_;
            if(!chunks.empty())
            {
                util::mpi_environment::scoped_lock l;
                MPI_Isend(
                    chunks.data()
                  , static_cast<int>(
                        chunks.size()
                      * sizeof(parcel_buffer_type::transmission_chunk_type)
                    )
                  , MPI_BYTE
                  , dst_
                  , tag_
                  , util::mpi_environment::communicator()
                  , &request_
                );
                request_ptr_ = &request_;
            }

            state_ = sent_transmission_chunks;
            return send_data();
        }

        bool send_data()
        {
            HPX_ASSERT(state_ == sent_transmission_chunks);
            if(!request_done()) return false;

            if(!header_.piggy_back())
            {
                util::mpi_environment::scoped_lock l;
                MPI_Isend(
                    buffer_.data_.data()
                  , static_cast<int>(buffer_.data_.size())
                  , MPI_BYTE
                  , dst_
                  , tag_
                  , util::mpi_environment::communicator()
                  , &request_
                );
                request_ptr_ = &request_;
            }
            state_ = sent_data;

            return send_chunks();
        }

        bool send_chunks()
        {
            HPX_ASSERT(state_ == sent_data);

            while(chunks_idx_ < buffer_.chunks_.size())
            {
                serialization::serialization_chunk& c = buffer_.chunks_[chunks_idx_];
                if(c.type_ == serialization::chunk_type_pointer)
                {
                    if(!request_done()) return false;
                    else
                    {
                        util::mpi_environment::scoped_lock l;
                        MPI_Isend(
                            const_cast<void *>(c.data_.cpos_)
                          , static_cast<int>(c.size_)
                          , MPI_BYTE
                          , dst_
                          , tag_
                          , util::mpi_environment::communicator()
                          , &request_
                        );
                        request_ptr_ = &request_;
                    }
                 }

                chunks_idx_++;
            }

            state_ = sent_chunks;

            return done();
        }

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

        bool done()
        {
            if(!request_done()) return false;

            error_code ec;
            handler_(ec);
            handler_.reset();
            buffer_.data_point_.time_ =
                hpx::chrono::high_resolution_clock::now() - buffer_.data_point_.time_;
            pp_->add_sent_data(buffer_.data_point_);
            buffer_.clear();

            state_ = initialized;

            return true;
        }

        connection_state state_;
        sender_type * sender_;
        int tag_;
        int dst_;
        util::unique_function_nonser<
            void(
                error_code const&
            )
        > handler_;
        util::unique_function_nonser<
            void(
                error_code const&
              , parcelset::locality const&
              , std::shared_ptr<sender_connection>
            )
        > postprocess_handler_;

        header header_;

        MPI_Request request_;
#ifdef HPX_USE_LCI
        void *request_ptr_;
        LCI_syncl_t sync_;
#else
        MPI_Request *request_ptr_;
#endif
        std::size_t chunks_idx_;
        char ack_;

        parcelset::parcelport* pp_;

        parcelset::locality there_;

    };
}}}}

#endif


