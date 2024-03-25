#include <hpx/parcelport_lci/completion_manager/completion_manager_sync_single_nolock.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>

namespace hpx::parcelset::policies::lci {
    LCI_request_t completion_manager_sync_single_nolock::poll()
    {
        LCI_request_t request;
        request.flag = LCI_ERR_RETRY;

        LCI_sync_test(sync, &request);
        if (request.flag == LCI_ERR_RETRY)
            if (config_t::progress_type == config_t::progress_type_t::poll)
                pp_->do_progress_local();
        return request;
    }
}    // namespace hpx::parcelset::policies::lci