#include <hpx/parcelport_lci/completion_manager/completion_manager_sync_single.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>

namespace hpx::parcelset::policies::lci {
    LCI_request_t completion_manager_sync_single::poll()
    {
        LCI_request_t request;
        request.flag = LCI_ERR_RETRY;

        bool succeed = lock.try_lock();
        if (succeed)
        {
            LCI_error_t ret = LCI_sync_test(sync, &request);
            if (ret == LCI_ERR_RETRY)
            {
                if (config_t::progress_type == config_t::progress_type_t::poll)
                    pp_->do_progress_local();
                lock.unlock();
            }
        }
        return request;
    }
}    // namespace hpx::parcelset::policies::lci