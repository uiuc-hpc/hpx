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
#include <hpx/modules/testing.hpp>

#include <cds/container/feldman_hashmap_hp.h>
#include <cds/init.h>    // for cds::Initialize and cds::Terminate

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <random>
#include <string>
#include <vector>

template <typename T>
struct atomwrapper
{
    std::atomic<T> _a;

    atomwrapper()
      : _a()
    {
    }

    atomwrapper(const std::atomic<T>& a)
      : _a(a.load())
    {
    }

    atomwrapper(const atomwrapper& other)
      : _a(other._a.load())
    {
    }

    atomwrapper& operator=(const atomwrapper& other)
    {
        _a.store(other._a.load());
    }
};

using gc_type = cds::gc::custom_HP<cds::gc::hp::details::HPXTLSManager>;
using key_type = std::size_t;
using value_type = atomwrapper<std::size_t>;

template <typename Map>
void run(Map& map, const std::size_t n_items, const std::size_t n_threads)
{
    // a reference counter vector to keep track of counter on
    // value [0, 1000)
    std::array<std::atomic<std::size_t>, 1000> counter_vec;
    std::fill(std::begin(counter_vec), std::end(counter_vec), 0);

    std::vector<hpx::thread> threads;

    // each thread inserts number of n_items/n_threads items to the map
    std::vector<std::vector<std::size_t>> numbers_vec(
        n_threads, std::vector<std::size_t>(n_items / n_threads, 0));

    // map init
    std::size_t val = 0;
    while (val < 1000)
    {
        std::atomic<std::size_t> counter(0);
        map.insert(val, counter);
        val++;
    }

    auto insert_val_and_increase_counter =
        [&](std::vector<std::size_t>& number_vec) {
            hpx::cds::hpxthread_manager_wrapper cds_hpx_wrap;
            hpx::this_thread::sleep_for(std::chrono::seconds(rand() % 5));
            for (auto val : number_vec)
            {
                typename Map::guarded_ptr gp;

                gp = map.get(val);
                if (gp)
                {
                    (gp->second)._a++;
                    counter_vec[gp->first]++;
                }
            }
        };

    for (auto& v : numbers_vec)
    {
        std::generate(v.begin(), v.end(),
            [&]() { return rand() % (n_items / n_threads); });

        threads.emplace_back(insert_val_and_increase_counter, std::ref(v));
    }

    // wait for all threads to complete
    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        HPX_TEST_EQ(counter_vec[it->first], (it->second)._a);
        //        std::cout << "map key: " << it->first << " value: " << (it->second)._a
        //                  << " and counter_vec: " << counter_vec[it->first] << "\n";
    }
}

int hpx_main(int, char**)
{
    using map_type =
        cds::container::FeldmanHashMap<gc_type, key_type, value_type>;

    // Initialize libcds and hazard pointer
    hpx::cds::libcds_wrapper cds_init_wrapper(
        hpx::cds::smr_t::hazard_pointer_hpxthread,
        map_type::c_nHazardPtrCount + 1, 100, 16);

    {
        // enable this thread/task to run using libcds support
        hpx::cds::hpxthread_manager_wrapper cds_hpx_wrap;

        map_type map;

        const std::size_t n_items = 10000;
        const std::size_t n_threads = 10;

        run(map, n_items, n_threads);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
