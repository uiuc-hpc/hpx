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

#include <memory>

namespace hpx::parcelset::policies::lci {
    void sender_connection::async_write(
        sender_connection::handler_type&& handler,
        sender_connection::postprocess_handler_type&& parcel_postprocess)
    {
        load(HPX_FORWARD(handler_type, handler),
            HPX_FORWARD(postprocess_handler_type, parcel_postprocess));
        if (!parcelport::enable_lci_backlog_queue ||
            HPX_UNLIKELY(parcelport::is_sending_early_parcel))
        {
            while (!send())
                continue;
            return;
        }
        else
        {
            if (!backlog_queue::empty(dst_rank))
            {
                backlog_queue::push(shared_from_this());
                return;
            }
            bool isSent = send();
            if (!isSent)
            {
                backlog_queue::push(shared_from_this());
                return;
            }
        }
    }

    void sender_connection::load(sender_connection::handler_type&& handler,
        sender_connection::postprocess_handler_type&& parcel_postprocess)
    {
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        buffer_.data_point_.time_ = hpx::chrono::high_resolution_clock::now();
#endif

        HPX_ASSERT(!handler_);
        HPX_ASSERT(!postprocess_handler_);
        HPX_ASSERT(!buffer_.data_.empty());
        handler_ = HPX_FORWARD(Handler, handler);
        postprocess_handler_ =
            HPX_FORWARD(ParcelPostprocess, parcel_postprocess);

        // calculate the maximum number of long messages to send
        // 2 messages (data + transmission chunk) + zero-copy chunks
        int num_zero_copy_chunks = static_cast<int>(buffer_.num_chunks_.first);
        size_t max_header_size =
            LCI_get_iovec_piggy_back_size(num_zero_copy_chunks + 2);
        // TODO: directly use LCI packet
        char* header_buffer = (char*) malloc(max_header_size);
        header header_ = header(buffer_, header_buffer, max_header_size);
        header_.assert_valid();

        // calculate the exact number of long messages to send
        int long_msg_num = 0;
        // data
        if (!header_.piggy_back_data())
            ++long_msg_num;
        if (num_zero_copy_chunks != 0)
        {
            // transmission chunks
            if (!header_.piggy_back_tchunk())
                ++long_msg_num;
            long_msg_num += num_zero_copy_chunks;
        }

        // initialize iovec
        iovec = LCI_iovec_t();
        iovec.piggy_back.address = header_.data();
        iovec.piggy_back.length = header_.size();
        iovec.count = long_msg_num;
        if (long_msg_num > 0)
        {
            int i = 0;
            iovec.lbuffers =
                (LCI_lbuffer_t*) malloc(iovec.count * sizeof(LCI_lbuffer_t));
            if (!header_.piggy_back_data())
            {
                // data (non-zero-copy chunks)
                iovec.lbuffers[i].address = buffer_.data_.data();
                iovec.lbuffers[i].length = buffer_.data_.size();
                iovec.lbuffers[i].segment = LCI_SEGMENT_ALL;
                ++i;
            }
            if (num_zero_copy_chunks > 0)
            {
                // transmission chunk
                if (!header_.piggy_back_tchunk())
                {
                    std::vector<
                        typename parcel_buffer_type::transmission_chunk_type>&
                        tchunks = buffer_.transmission_chunks_;
                    int tchunks_length = static_cast<int>(tchunks.size() *
                        sizeof(parcel_buffer_type::transmission_chunk_type));
                    iovec.lbuffers[i].address = tchunks.data();
                    iovec.lbuffers[i].length = tchunks_length;
                    // TODO: register the buffer here
                    iovec.lbuffers[i].segment = LCI_SEGMENT_ALL;
                    ++i;
                }
                // zero-copy chunks
                for (int j = 0; j < (int) buffer_.chunks_.size(); ++j)
                {
                    serialization::serialization_chunk& c = buffer_.chunks_[j];
                    if (c.type_ ==
                        serialization::chunk_type::chunk_type_pointer)
                    {
                        HPX_ASSERT(long_msg_num > i);
                        iovec.lbuffers[i].address =
                            const_cast<void*>(c.data_.cpos_);
                        iovec.lbuffers[i].length = c.size_;
                        iovec.lbuffers[i].segment = LCI_SEGMENT_ALL;
                        ++i;
                    }
                }
            }
            HPX_ASSERT(long_msg_num == i);
        }
    }

    bool sender_connection::send() {
        static hpx::spinlock send_mtx;

        int ret;
        void* buffer_to_free = iovec.piggy_back.address;
        if (iovec.count == 0)
        {
            if (parcelport::enable_lci_try_lock_send)
                send_mtx.lock();
            ret = LCI_putma(util::lci_environment::get_endpoint(),
                iovec.piggy_back, dst_rank, 0, LCI_DEFAULT_COMP_REMOTE);
            if (parcelport::enable_lci_try_lock_send)
                send_mtx.unlock();
            if (ret == LCI_OK)
            {
                free(buffer_to_free);
                done();
            }
        }
        else
        {
            auto* sharedPtr_p =
                new std::shared_ptr<sender_connection>(shared_from_this());
            // In order to keep the send_connection object from being
            // deallocated. We have to allocate a shared_ptr in the heap
            // and pass a pointer to shared_ptr to LCI.
            // We will get this pointer back via the send completion queue
            // after this send completes.
            if (parcelport::enable_lci_try_lock_send)
                send_mtx.lock();
            ret = LCI_putva(util::lci_environment::get_endpoint(), iovec,
                util::lci_environment::get_scq(), dst_rank, 0,
                LCI_DEFAULT_COMP_REMOTE, sharedPtr_p);
            if (parcelport::enable_lci_try_lock_send)
                send_mtx.unlock();
            // After this point, if ret == OK, this object can be shared by
            // two threads (the sending thread and the thread polling the
            // completion queue). Care must be taken to avoid data race.
            if (ret == LCI_OK)
            {
                free(buffer_to_free);
            } else {
                delete sharedPtr_p;
            }
        }
        return ret == LCI_OK;
    }

    void sender_connection::done()
    {
        if (iovec.count > 0)
            free(iovec.lbuffers);
        error_code ec;
        handler_(ec);
        handler_.reset();
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        buffer_.data_point_.time_ = hpx::chrono::high_resolution_clock::now() -
            buffer_.data_point_.time_;
        pp_->add_sent_data(buffer_.data_point_);
#endif
        buffer_.clear();
        iovec.count = -1;

        if (postprocess_handler_)
        {
            hpx::move_only_function<void(error_code const&,
                parcelset::locality const&, std::shared_ptr<sender_connection>)>
                postprocess_handler;
            std::swap(postprocess_handler, postprocess_handler_);
            error_code ec2;
            postprocess_handler(ec2, there_, shared_from_this());
        }
    }
}    // namespace hpx::parcelset::policies::lci

#endif
