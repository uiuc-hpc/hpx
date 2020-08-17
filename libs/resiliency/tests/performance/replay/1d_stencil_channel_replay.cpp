// Copyright (c) 2020 Nikunj Gupta
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the a separate examples demonstrating the development
// of a fully distributed solver for a simple 1D heat distribution problem.
//
// This example makes use of LCOS channels to send and receive
// elements.

#include "communicator.hpp"
#include "stencil.hpp"

#include <hpx/algorithm.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/compute.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/program_options/options_description.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

using communication_type = double;

HPX_REGISTER_CHANNEL_DECLARATION(communication_type);
HPX_REGISTER_CHANNEL(communication_type, stencil_communication);

std::vector<double> distributed_step(std::vector<double> prev,
        std::size_t local_nx, std::size_t nlp, std::size_t t,
        std::size_t error, std::size_t faults)
{
    std::size_t rank = hpx::get_locality_id();

    bool is_faulty_node = false;
    if ((faults * 3) % rank == 0)   is_faulty_node = true;

    // Initiate replay executor
    hpx::parallel::execution::parallel_executor base_exec;
    auto exec =
        hpx::resiliency::experimental::make_replay_executor(base_exec, 3);

    std::array<std::vector<double>, 2> U;
    U[t % 2] = std::move(prev);
    U[(t + 1) % 2] = std::vector<double>(U[t % 2].size());

    auto range = boost::irange(static_cast<std::size_t>(0), nlp);

    hpx::ranges::for_each(
        hpx::parallel::execution::par.on(exec)
        , range, [&U, local_nx, nlp, t, error, is_faulty_node](std::size_t i) {
            if (i == 0)
                stencil_update(U, 1, local_nx, t, error, is_faulty_node);
            else if (i == nlp - 1)
                stencil_update(U, i * local_nx, (i + 1) * local_nx - 1, t, error, is_faulty_node);
            else if (i > 0 && i < nlp - 1)
                stencil_update(U, i * local_nx, (i + 1) * local_nx, t, error, is_faulty_node);
        });

    return U[(t + 1) % 2];
}

HPX_PLAIN_ACTION(distributed_step, distributed_step_action);

int hpx_main(boost::program_options::variables_map& vm)
{
    std::size_t Nx_global = vm["Nx"].as<std::size_t>();
    std::size_t steps = vm["steps"].as<std::size_t>();
    std::size_t sti = vm["sti"].as<std::size_t>();
    std::size_t nlp = vm["Nlp"].as<std::size_t>();
    std::size_t error = vm["error"].as<std::size_t>();
    std::size_t faults = vm["faults"].as<std::size_t>();

    typedef std::vector<double> data_type;

    std::array<data_type, 2> U;

    std::size_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::size_t num_worker_threads = hpx::get_num_worker_threads();
    std::size_t rank = hpx::get_locality_id();

    bool is_faulty_node = false;
    if ((faults * 3) % rank == 0)   is_faulty_node = true;

    hpx::util::high_resolution_timer t_main;

    // Keep only partial data
    std::size_t Nx = Nx_global / num_localities;
    std::size_t local_nx = Nx / nlp;

    U[0] = data_type(Nx, 0.0);
    U[1] = data_type(Nx, 0.0);

    init(U, Nx, rank, num_localities);

    // Initiate replay executor
    hpx::parallel::execution::parallel_executor base_exec;
    auto exec =
        hpx::resiliency::experimental::make_replay_executor(base_exec, 3);

    // setup communicator
    using communicator_type = communicator<double>;
    communicator_type comm(rank, num_localities);

    if (rank == 0)
    {
        std::cout << "Starting benchmark with " << num_localities
                  << " nodes and " << num_worker_threads << " threads.\n";
    }

    if (comm.has_neighbor(communicator_type::left))
    {
        // Send initial value to the left neighbor
        comm.set(communicator_type::left, U[0][0], 0);
    }
    if (comm.has_neighbor(communicator_type::right))
    {
        // Send initial value to the right neighbor
        comm.set(communicator_type::right, U[0][Nx - 1], 0);
    }

    auto range = boost::irange(static_cast<std::size_t>(0), nlp);

    hpx::util::high_resolution_timer t;
    for (std::size_t t = 0; t < steps; ++t)
    {
        data_type& curr = U[t % 2];
        data_type& next = U[(t + 1) % 2];

        hpx::future<void> l = hpx::make_ready_future();
        hpx::future<void> r = hpx::make_ready_future();

        if (comm.has_neighbor(communicator_type::left))
        {
            l = comm.get(communicator_type::left, t)
                    .then(hpx::launch::sync,
                        [&next, &curr, &comm, t](hpx::future<double>&& gg) {
                            double left = gg.get();

                            next[0] = curr[0] +
                                ((k * dt) / (dx * dx)) *
                                    (left - 2 * curr[0] + curr[1]);

                            // Dispatch the updated value to left neighbor for it
                            // to get consumed in the next timestep
                            comm.set(communicator_type::left, next[0], t + 1);
                        });
        }

        if (comm.has_neighbor(communicator_type::right))
        {
            r = comm.get(communicator_type::right, t)
                    .then(hpx::launch::sync,
                        [&next, &curr, &comm, t, Nx](hpx::future<double>&& gg) {
                            double right = gg.get();

                            next[Nx - 1] = curr[Nx - 1] +
                                ((k * dt) / (dx * dx)) *
                                    (curr[Nx - 2] - 2 * curr[Nx - 1] + right);

                            // Dispatch the updated value to right neighbor for it
                            // to get consumed in the next timestep
                            comm.set(
                                communicator_type::right, next[Nx - 1], t + 1);
                        });
        }

        try {
            hpx::ranges::for_each(
                hpx::parallel::execution::par.on(exec)
                , range, [&U, local_nx, nlp, t, error, is_faulty_node](std::size_t i) {
                    if (i == 0)
                        stencil_update(U, 1, local_nx, t, error, is_faulty_node);
                    else if (i == nlp - 1)
                        stencil_update(U, i * local_nx, (i + 1) * local_nx - 1, t, error, is_faulty_node);
                    else if (i > 0 && i < nlp - 1)
                        stencil_update(U, i * local_nx, (i + 1) * local_nx, t, error, is_faulty_node);
                });
        }
        catch (...)
        {
            // Set up replay locales for our current step
            hpx::id_type id_1 = hpx::naming::get_id_from_locality_id((rank + 1) % num_localities);
            hpx::id_type id_2 = hpx::naming::get_id_from_locality_id((rank + 2) % num_localities);
            hpx::id_type id_3 = hpx::naming::get_id_from_locality_id((rank + 3) % num_localities);

            std::vector<hpx::id_type> locales{id_1, id_2, id_3};

            // We will try to replay
            distributed_step_action ac;
            hpx::future<std::vector<double>> f =
                 hpx::resiliency::experimental::async_replay(locales, ac, curr, local_nx, nlp, t, error, faults);

            // Store the result obtained
            U[(t + 1) % 2] = f.get();
        }

        hpx::wait_all(l, r);
    }
    double elapsed = t.elapsed();
    double telapsed = t_main.elapsed();

    if (rank == 0)
    {
        std::cout << "Total time: " << telapsed << std::endl;
        std::cout << "Kernel execution time: " << elapsed << std::endl;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline;
    // clang-format off
    desc_commandline.add_options()
        ("Nx", value<std::size_t>()->default_value(1024),
         "Total stencil points")
        ("Nlp", value<std::size_t>()->default_value(16),
         "Number of Local Partitions")
        ("steps", value<std::size_t>()->default_value(100),
         "Number of steps to apply the stencil")
        ("error", value<std::size_t>()->default_value(5),
         "Error rate on non-faulty nodes. For faulty nodes it is 10x")
        ("faults", value<std::size_t>()->default_value(5),
         "Number of faulty nodes")
    ;
    // clang-format on

    // Initialize and run HPX, this example requires to run hpx_main on all
    // localities
    std::vector<std::string> const cfg = {
        "hpx.run_hpx_main!=1",
    };

    return hpx::init(desc_commandline, argc, argv, cfg);
}
