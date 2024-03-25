#include <hpx/parcelport_lci/completion_manager/completion_manager_sync.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>

namespace hpx::parcelset::policies::lci {
    LCI_request_t completion_manager_sync::poll()
    {
        LCI_request_t request;
        request.flag = LCI_ERR_RETRY;
        if (sync_list.empty())
        {
            return request;
        }
        {
            std::unique_lock l(lock, std::try_to_lock);
            if (l.owns_lock() && !sync_list.empty())
            {
                LCI_comp_t sync = sync_list.front();
                sync_list.pop_front();
                LCI_error_t ret = LCI_sync_test(sync, &request);
                if (ret == LCI_OK)
                {
                    HPX_ASSERT(request.flag == LCI_OK);
                    LCI_sync_free(&sync);
                }
                else
                {
                    if (config_t::progress_type ==
                        config_t::progress_type_t::poll)
                        pp_->do_progress_local();
                    sync_list.push_back(sync);
                }
            }
        }
        return request;
    }
}    // namespace hpx::parcelset::policies::lci