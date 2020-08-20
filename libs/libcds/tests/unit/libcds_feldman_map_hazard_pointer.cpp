//  Copyright (c) 2020 Weile Wei
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/libcds/hpx_tls_manager.hpp>

#include <cds/container/feldman_hashmap_hp.h>
#include <cds/init.h>    // for cds::Initialize and cds::Terminate

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <random>
#include <vector>

using gc_type = cds::gc::custom_HP<cds::gc::hp::details::HPXTLSManager>;
using key_type = std::size_t;
using value_type = std::string;

template <typename Map>
void run(Map& map, const std::size_t nMaxItemCount)
{
    std::vector<key_type> rand_vec(nMaxItemCount);
    std::generate(rand_vec.begin(), rand_vec.end(), std::rand);

    std::vector<hpx::future<void>> futures;

    for (auto ele : rand_vec)
    {
        futures.push_back(hpx::async([&, ele]() {
            // enable this thread/task to run using libcds support
            hpx::cds::hpxthread_manager_wrapper cds_hpx_wrap;

            hpx::this_thread::sleep_for(std::chrono::seconds(rand() % 5));
            map.insert(ele, std::to_string(ele));
        }));
    }

    hpx::wait_all(futures);

    std::size_t count = 0;

    while (!map.empty())
    {
        auto guarded_ptr = map.extract(rand_vec[count]);
        HPX_ASSERT(guarded_ptr->first == rand_vec[count]);
        HPX_ASSERT(guarded_ptr->second == std::to_string(rand_vec[count]));
        count++;
    }
}

int hpx_main(int, char**)
{
    // Initialize libcds
    hpx::cds::libcds_wrapper cds_init_wrapper;

    {
        using map_type =
            cds::container::FeldmanHashMap<gc_type, key_type, value_type>;

        const int max_attach_concurrent_thread =
            hpx::cds::hpxthread_manager_wrapper::max_concurrent_attach_thread;
        cds::gc::hp::custom_smr<cds::gc::hp::details::HPXTLSManager>::construct(
            map_type::c_nHazardPtrCount + 1, max_attach_concurrent_thread, 16);

        // enable this thread/task to run using libcds support
        hpx::cds::hpxthread_manager_wrapper cds_hpx_wrap;

        const std::size_t nMaxItemCount =
            100;    // estimation of max item count in the hash map

        map_type map;

        run(map, nMaxItemCount);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
