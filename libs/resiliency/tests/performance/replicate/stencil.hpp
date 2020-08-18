// Copyright (c) 2020 Nikunj Gupta
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/include/compute.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/parallel_algorithm.hpp>
#include <hpx/include/util.hpp>


#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

using data_type = std::vector<double>;

const double k = 0.5;     // heat transfer coefficient
const double dt = 1.;     // time step
const double dx = 1.;     // grid spacing

template <typename Container>
void init(
    std::array<Container, 2>& U, std::size_t Nx,
    std::size_t rank = 0, std::size_t num_localities = 1)
{
    // Initialize: Boundaries are set to 1, interior is 0
    if (rank == 0)
    {
        U[0][0] = 1.0;
        U[1][0] = 1.0;
    }
    if (rank == num_localities - 1)
    {
        U[0][Nx-1] = 100.0;
        U[1][Nx-1] = 100.0;
    }
}

// Random number generator
std::random_device randomizer;
std::mt19937 gen(randomizer());
std::uniform_int_distribution<std::size_t> dis(1, 100);

void stencil_update(std::array<data_type, 2>& U, const std::size_t& begin
        , const std::size_t& end, const std::size_t t, std::size_t error,
        bool is_faulty_node)
{
    data_type& curr = U[t % 2];
    data_type& next = U[(t + 1) % 2];

    for (std::size_t i = begin; i < end; ++i)
    {
        next[i] = curr[i] + ((k*dt)/(dx*dx)) *
                                    (curr[i-1] - 2*curr[i] + curr[i+1]);
    }

    if (is_faulty_node)
    {
        if (dis(gen) < (error * 10))
            throw std::runtime_error("Error occured");
    }
    else
    {
        if (dis(gen) < error)
            throw std::runtime_error("Error occured");
    }
}
