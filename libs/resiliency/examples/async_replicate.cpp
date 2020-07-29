//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2020 Hartmut Kaiser
//  Copyright (c) 2019 Adrian Serio
//  Copyright (c) 2019-2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/actions_base/plain_action.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> dist(1.0, 10.0);

int universal_ans()
{
    if (dist(mt) > 5)
        return 42;
    return 84;
}

HPX_PLAIN_ACTION(universal_ans, universal_action);

bool validate(int ans)
{
    return ans == 42;
}

int vote(std::vector<int>&& results)
{
    return results.at(0);
}

int hpx_main()
{
    std::vector<hpx::naming::id_type> locals = hpx::find_all_localities();

    // Allow a task to replay on the same locality if it only 1 locality
    if (locals.size() == 1)
        locals.insert(locals.end(), 9, hpx::find_here());

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate(10, &universal_ans);

        auto result = f.get();
        std::cout << "Universal answer returned: " << result << std::endl;
    }

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_validate(
                10, &validate, &universal_ans);

        auto result = f.get();

        std::cout << "Correct universal answer: " << result << std::endl;
    }

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote(
                10, &vote, &universal_ans);

        auto result = f.get();

        std::cout << "Correct universal answer: " << result << std::endl;
    }

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote_validate(
                10, &vote, &validate, &universal_ans);

        auto result = f.get();

        std::cout << "Correct universal answer: " << result << std::endl;
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate(locals, action);

        auto result = f.get();

        std::cout << "Universal answer returned: " << result << std::endl;
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_validate(
                locals, &validate, action);

        auto result = f.get();

        std::cout << "Universal answer returned: " << result << std::endl;
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote(
                locals, &vote, action);

        auto result = f.get();

        std::cout << "Universal answer returned: " << result << std::endl;
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote_validate(
                locals, &vote, &validate, action);

        auto result = f.get();

        std::cout << "Correct universal answer: " << result << std::endl;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
