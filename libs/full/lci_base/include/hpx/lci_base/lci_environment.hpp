//  Copyright (c) 2013-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)) ||      \
    defined(HPX_HAVE_MODULE_LCI_BASE)

#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/lci_base/lci.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <cstdlib>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

#define DEBUG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }

namespace hpx { namespace util {
    struct HPX_EXPORT lci_environment
    {
        static bool check_lci_environment(runtime_configuration const& cfg);

        static int init(int* argc, char*** argv, const int required,
            const int minimal, int& provided);
        static void init(int* argc, char*** argv, runtime_configuration& cfg);
        static void finalize();

        static bool enabled();
        static bool multi_threaded();
        static bool has_called_init();

        static int rank();
        static int size();

        static MPI_Comm& communicator();

        static LCI_endpoint_t& lci_endpoint();

        static std::string get_processor_name();

        struct scoped_lock
        {
            scoped_lock();
            scoped_lock(scoped_lock const&) = delete;
            scoped_lock& operator=(scoped_lock const&) = delete;
            ~scoped_lock();
            void unlock();
        };

        struct scoped_try_lock
        {
            scoped_try_lock();
            scoped_try_lock(scoped_try_lock const&) = delete;
            scoped_try_lock& operator=(scoped_try_lock const&) = delete;
            ~scoped_try_lock();
            void unlock();
            bool locked;
        };

        typedef hpx::lcos::local::spinlock mutex_type;

    private:
        static mutex_type mtx_;

        static bool enabled_;
        static bool has_called_init_;
        static int provided_threading_flag_;
        static MPI_Comm communicator_;

        static int is_initialized_;

        //static LCI_plist_t prop_;
        //static LCI_device_t dev_;
        static LCI_PL_t prop_;
        static LCI_MT_t mt_;
        static LCI_endpoint_t ep_;

    };
}}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#else

#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx { namespace util {
    struct HPX_EXPORT lci_environment
    {
        static bool check_lci_environment(runtime_configuration const& cfg);
    };
}}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#endif