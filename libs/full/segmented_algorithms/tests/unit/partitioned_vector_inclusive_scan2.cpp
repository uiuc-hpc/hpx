//  Copyright (c) 2016 Minh-Khanh Do
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_scan.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/testing.hpp>

#include "partitioned_vector_scan.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>

#define DEBUG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
//#define DEBUG(...) 

///////////////////////////////////////////////////////////////////////////////
#define msg7(a, b, c, d, e, f, g)                                              \
    std::cout << std::setw(60) << a << std::setw(40) << b << std::setw(10)     \
              << c << std::setw(6) << " " << #d << " " << e << " " << f << " " \
              << g << " ";
#define msg9(a, b, c, d, e, f, g, h, i)                                        \
    std::cout << std::setw(60) << a << std::setw(40) << b << std::setw(10)     \
              << c << std::setw(6) << " " << #d << " " << e << " " << f << " " \
              << g << " " << h << " " << i << " ";

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct opt
{
    T operator()(T v1, T v2) const
    {
        return v1 + v2;
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_with_policy(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        regular, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    DEBUG("inclusive_scan_algo_tests_with_policy A, size=%lu", size);
    hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    DEBUG("inclusive_scan_algo_tests_with_policy B");
    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    DEBUG("inclusive_scan_algo_tests_with_policy C");
    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_segmented_out_with_policy(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T> out,
    std::vector<T> ver, ExPolicy const& policy)
{
    // NOTE: this is the test where we get the errors
    DEBUG("inclusive_scan_algo_tests_segmented_out_with_policy START");
    msg9(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
    DEBUG("inclusive_scan_algo_tests_segmented_out_with_policy END");
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_inplace_with_policy(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        inplace, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    hpx::partitioned_vector<T> in(size, dist_policy);
    iota_vector(in, T(1));

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), in.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(in, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_segmented_out_with_policy_async(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T> out,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg9(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async_segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_inplace_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async_inplace, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    hpx::partitioned_vector<T> in(size, dist_policy);
    iota_vector(in, T(1));

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::parallel::inclusive_scan(
        policy, in.begin(), in.end(), in.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(in, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename DistPolicy>
void inclusive_scan_tests_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup partitioned vector to test
    hpx::partitioned_vector<T> in(size, policy);
    iota_vector(in, T(1));

    std::vector<T> ver(in.size());
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    DEBUG("inclusive_scan_tests_with_policy A, size=%lu", size);
    hpx::parallel::v1::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    //sync
    DEBUG("inclusive_scan_tests_with_policy B");
    inclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, seq);
    DEBUG("inclusive_scan_tests_with_policy C");
    inclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, par);

    //async
    DEBUG("inclusive_scan_tests_with_policy D");
    inclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, seq(task));
    DEBUG("inclusive_scan_tests_with_policy E");
    inclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, par(task));
}

template <typename T, typename DistPolicy>
void inclusive_scan_tests_segmented_out_with_policy(
    std::size_t size, DistPolicy const& in_policy, DistPolicy const& out_policy)
{
    using namespace hpx::execution;

    DEBUG("inclusive_scan_tests_segmented_out_with_policy A, size=%lu", size);

    // setup partitioned vector to test
    hpx::partitioned_vector<T> in(size, in_policy);
    iota_vector(in, T(1));

    hpx::partitioned_vector<T> out(size, out_policy);

    std::vector<T> ver(in.size());
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::v1::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    //sync
    inclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, seq);
    inclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, par);

    //async
    inclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, seq(task));
    inclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, par(task));

    DEBUG("inclusive_scan_tests_segmented_out_with_policy B");
}

template <typename T, typename DistPolicy>
void inclusive_scan_tests_inplace_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup verification vector
    std::vector<T> ver(size);
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::v1::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    // sync
    inclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, seq);
    inclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, par);

    // async
    inclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, seq(task));
    inclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, par(task));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
void inclusive_scan_tests(std::vector<hpx::id_type>& localities)
{
    DEBUG("inclusive_scan_tests() A");
    std::size_t const length = 1000000; // original size
    //std::size_t const length = 250000;
    //std::size_t const length = 100000;

    inclusive_scan_tests_with_policy<T>(length, hpx::container_layout);
    inclusive_scan_tests_with_policy<T>(length, hpx::container_layout(3));
    inclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(3, localities));
    inclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(localities));

    inclusive_scan_tests_with_policy<T>(1000, hpx::container_layout(1000));
    
    DEBUG("inclusive_scan_tests() B");

    // multiple localities needed for the following tests
    DEBUG("inclusive_scan_tests_segmented_out_with_policy 1");
    inclusive_scan_tests_segmented_out_with_policy<T>(length,
        hpx::container_layout(localities), hpx::container_layout(localities));

    DEBUG("inclusive_scan_tests_segmented_out_with_policy 2");
    inclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(3));

    DEBUG("inclusive_scan_tests_segmented_out_with_policy 3");
    inclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(10));

    DEBUG("inclusive_scan_tests_segmented_out_with_policy 4");
    inclusive_scan_tests_inplace_with_policy<T>(
        length, hpx::container_layout(localities));
    DEBUG("inclusive_scan_tests() C");
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    DEBUG("inclusive_scan2 main() A");
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    DEBUG("inclusive_scan2 main() B");
    inclusive_scan_tests<double>(localities);
    DEBUG("inclusive_scan2 main() C");
    return hpx::util::report_errors();
}
#endif
