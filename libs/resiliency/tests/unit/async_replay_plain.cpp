//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2020 Hartmut Kaiser
//  Copyright (c) 2018-2019 Adrian Serio
//  Copyright (c) 2019-20 Nikunj Gupta
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

int hpx_main()
{
    std::vector<hpx::id_type> locals = hpx::find_all_localities();

    // Allow a task to replay on the same locality if it only 1 locality
    if (locals.size() == 1)
        locals.insert(locals.end(), 9, hpx::find_here());

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay(10, &universal_ans);

        auto result = f.get();
        HPX_TEST(result == 42 || result == 84);
    }

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay_validate(
                10, &validate, &universal_ans);

        auto result = f.get();
        HPX_TEST(result == 42);
    }

    {
        universal_action our_action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay(locals, our_action);

        auto result = f.get();
        HPX_TEST(result == 42 || result == 84);
    }

    {
        universal_action our_action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay_validate(
                locals, &validate, our_action);

        auto result = f.get();

        HPX_TEST(result == 42);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST(hpx::init(argc, argv) == 0);
    return hpx::util::report_errors();
}
