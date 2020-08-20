//  Copyright (c) 2020 Weile Wei
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>

#include <cds/container/michael_kvlist_hp.h>
#include <cds/container/michael_map.h>
#include <cds/init.h>    // for cds::Initialize and cds::Terminate

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <iterator>
#include <random>
#include <thread>
#include <vector>

// Declare traits
struct list_trait : public cds::container::michael_list::traits
{
    typedef std::less<std::size_t> less;
};

using gc_type = cds::gc::custom_HP<cds::gc::hp::details::DefaultTLSManager>;

// Declare traits-based list
using int2str_list = cds::container::MichaelKVList<gc_type, std::size_t,
    std::string, list_trait>;

template <typename Map>
void run(Map& map, const std::size_t nMaxItemCount)
{
    std::vector<std::size_t> rand_vec(nMaxItemCount);
    std::generate(rand_vec.begin(), rand_vec.end(), std::rand);

    std::vector<std::future<void>> futures;

    for (auto ele : rand_vec)
    {
        futures.push_back(std::async([&, ele]() {
            std::this_thread::sleep_for(std::chrono::seconds(rand() % 5));

            // enable this thread/task to run using libcds support
            cds::gc::hp::custom_smr<
                cds::gc::hp::details::DefaultTLSManager>::attach_thread();

            map.insert(ele, std::to_string(ele));
            cds::gc::hp::custom_smr<
                cds::gc::hp::details::DefaultTLSManager>::detach_thread();
        }));
    }

    for (auto& f : futures)
        f.get();

    std::size_t count = 0;

    while (!map.empty())
    {
        auto guarded_ptr = map.extract(rand_vec[count]);
        HPX_ASSERT(guarded_ptr->first == rand_vec[count]);
        HPX_ASSERT(guarded_ptr->second == std::to_string(rand_vec[count]));
        count++;
    }
}

int main(int argc, char* argv[])
{
    // Initialize libcds
    cds::Initialize();

    {
        using map_type = cds::container::MichaelHashMap<gc_type, int2str_list>;

        cds::gc::hp::custom_smr<cds::gc::hp::details::DefaultTLSManager>::
            construct(map_type::c_nHazardPtrCount + 1, 100, 16);

        // enable this thread/task to run using libcds support
        cds::gc::hp::custom_smr<
            cds::gc::hp::details::DefaultTLSManager>::attach_thread();

        const std::size_t nMaxItemCount =
            100;    // estimation of max item count in the hash map
        const std::size_t nLoadFactor =
            100;    // load factor: estimation of max number of items in the bucket

        map_type map(nMaxItemCount, nLoadFactor);

        run(map, nMaxItemCount);
    }

    // Terminate libcds
    cds::Terminate();
}
