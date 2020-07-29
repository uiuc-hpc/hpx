//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2019 Hartmut Kaiser
//  Copyright (c) 2018-2019 Adrian Serio
//  Copyright (c) 2019-2020 Nikunj Gupta
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
std::uniform_real_distribution<double> dist(0., 1.);

int universal_ans(std::size_t err, std::size_t size)
{
    // Pretending to do some useful work
    hpx::this_thread::sleep_for(std::chrono::microseconds(size));

    // Introduce artificial errors
    if (dist(mt) < (err / 100.))
        throw std::runtime_error("runtime error occured.");

    return 42;
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

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint64_t err = vm["err"].as<std::uint64_t>();
    std::uint64_t size = vm["err"].as<std::uint64_t>();
    std::uint64_t num_tasks = vm["err"].as<std::uint64_t>();

    universal_action ac;
    std::vector<hpx::id_type> locales = hpx::find_all_localities();

    {
        hpx::util::high_resolution_timer t;

        std::vector<hpx::future<int>> tasks;
        for (std::size_t i = 0; i < num_tasks; ++i)
            tasks.push_back(hpx::resiliency::experimental::async_replicate(
                locales, ac, err, size));

        hpx::wait_all(tasks);

        double elapsed = t.elapsed();
        std::cout << "Replicate: " << elapsed << std::endl;
    }

    {
        hpx::util::high_resolution_timer t;

        std::vector<hpx::future<int>> tasks;
        for (std::size_t i = 0; i < num_tasks; ++i)
            tasks.push_back(
                hpx::resiliency::experimental::async_replicate_validate(
                    locales, &validate, ac, err, size));

        hpx::wait_all(tasks);

        double elapsed = t.elapsed();
        std::cout << "Replicate Validate: " << elapsed << std::endl;
    }

    {
        hpx::util::high_resolution_timer t;

        std::vector<hpx::future<int>> tasks;
        for (std::size_t i = 0; i < num_tasks; ++i)
            tasks.push_back(hpx::resiliency::experimental::async_replicate_vote(
                locales, &vote, ac, err, size));

        hpx::wait_all(tasks);

        double elapsed = t.elapsed();
        std::cout << "Replicate Vote: " << elapsed << std::endl;
    }

    {
        hpx::util::high_resolution_timer t;

        std::vector<hpx::future<int>> tasks;
        for (std::size_t i = 0; i < num_tasks; ++i)
            tasks.push_back(
                hpx::resiliency::experimental::async_replicate_vote_validate(
                    locales, &vote, &validate, ac, err, size));

        hpx::wait_all(tasks);

        double elapsed = t.elapsed();
        std::cout << "Replicate Vote Validate: " << elapsed << std::endl;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Configure application-specific options
    hpx::program_options::options_description desc_commandline;

    desc_commandline.add_options()("err",
        hpx::program_options::value<std::uint64_t>()->default_value(1),
        "Amount of artificial error injection")("size",
        hpx::program_options::value<std::uint64_t>()->default_value(200),
        "Grain size of a task")("num-tasks",
        hpx::program_options::value<std::uint64_t>()->default_value(1000000),
        "Number of tasks to invoke");

    // Initialize and run HPX
    return hpx::init(desc_commandline, argc, argv);
}
