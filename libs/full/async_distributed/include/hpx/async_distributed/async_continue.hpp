//  Copyright (c) 2007-2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/actions_base/actions_base_support.hpp>
#include <hpx/actions_base/traits/extract_action.hpp>
#include <hpx/async_distributed/async_continue_fwd.hpp>
#include <hpx/async_local/async_fwd.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/traits/promise_local_result.hpp>
#include <hpx/futures/traits/promise_remote_result.hpp>
#include <hpx/lcos/packaged_action.hpp>
#include <hpx/traits/is_distribution_policy.hpp>

#include <type_traits>
#include <utility>

namespace hpx {
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename Action, typename RemoteResult, typename Cont,
            typename Target, typename... Ts>
        lcos::future<typename traits::promise_local_result<
            typename result_of_async_continue<Action, Cont>::type>::type>
        async_continue_r(Cont&& cont, Target const& target, Ts&&... vs)
        {
            typedef typename traits::promise_local_result<
                typename result_of_async_continue<Action, Cont>::type>::type
                result_type;

            typedef
                typename hpx::traits::extract_action<Action>::remote_result_type
                    continuation_result_type;

            lcos::promise<result_type, RemoteResult> p;
            auto f = p.get_future();

            apply<Action>(hpx::actions::typed_continuation<result_type,
                              continuation_result_type>(
                              p.get_id(), std::forward<Cont>(cont)),
                target, std::forward<Ts>(vs)...);

            return f;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Cont, typename... Ts>
    lcos::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Action, Cont>::type>::type>
    async_continue(Cont&& cont, naming::id_type const& gid, Ts&&... vs)
    {
        typedef typename traits::promise_remote_result<
            typename detail::result_of_async_continue<Action, Cont>::type>::type
            result_type;

        return detail::async_continue_r<Action, result_type>(
            std::forward<Cont>(cont), gid, std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename Cont, typename... Ts>
    lcos::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Derived, Cont>::type>::type>
    async_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/
        ,
        Cont&& cont, naming::id_type const& gid, Ts&&... vs)
    {
        return async_continue<Derived>(
            std::forward<Cont>(cont), gid, std::forward<Ts>(vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Cont, typename DistPolicy,
        typename... Ts>
    typename std::enable_if<traits::is_distribution_policy<DistPolicy>::value,
        lcos::future<typename traits::promise_local_result<typename detail::
                result_of_async_continue<Action, Cont>::type>::type>>::type
    async_continue(Cont&& cont, DistPolicy const& policy, Ts&&... vs)
    {
        typedef typename traits::promise_remote_result<
            typename detail::result_of_async_continue<Action, Cont>::type>::type
            result_type;

        return detail::async_continue_r<Action, result_type>(
            std::forward<Cont>(cont), policy, std::forward<Ts>(vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename Cont, typename DistPolicy, typename... Ts>
    typename std::enable_if<traits::is_distribution_policy<DistPolicy>::value,
        lcos::future<typename traits::promise_local_result<typename detail::
                result_of_async_continue<Derived, Cont>::type>::type>>::type
    async_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/
        ,
        Cont&& cont, DistPolicy const& policy, Ts&&... vs)
    {
        return async_continue<Derived>(
            std::forward<Cont>(cont), policy, std::forward<Ts>(vs)...);
    }
}    // namespace hpx
#endif
