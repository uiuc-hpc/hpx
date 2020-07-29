//  Copyright (c) 2020 Weile Wei
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>

#include <cds/container/fcpriority_queue.h>
#include <cds/init.h>     // for cds::Initialize and cds::Terminate

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <random>
#include <vector>
#include <cassert>

template<class PQueue>
void run(PQueue &pq) {
    // generate a list of random numbers
    std::size_t num_elem = 10000;
    std::vector <std::size_t> nums(num_elem);
    std::generate(nums.begin(), nums.end(), std::rand);

    // launch num_elem of threads, each of which puts a random number
    // into fc priority queue
    for (auto i : nums) {
        pq.push(i);
    }

    // sort the list to expected order
    std::sort(nums.begin(), nums.end());

    // valid if fc priority queue sorts elements in order
    for (auto i : nums) {
        std::size_t val;
        pq.pop(val);
        assert(i == val);
    }
}

int hpx_main(int, char **) {
    // Initialize libcds
    cds::Initialize();

    {
        cds::threading::Manager::attachThread();

        // Create flat-combining priority queue object
        using fc_pqueue_type = cds::container::FCPriorityQueue <std::size_t,
        std::priority_queue<std::size_t, std::deque < std::size_t>,
        std::greater<std::size_t>>>;

        fc_pqueue_type pq;

        run(pq);

        cds::threading::Manager::detachThread();
    }

    // Terminate libcds
    cds::Terminate();

    return hpx::finalize();
}

int main(int argc, char *argv[]) {
    return hpx::init(argc, argv);
}
