//  Copyright (c) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/local/barrier.hpp>
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME) && !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/distributed/barrier.hpp>
#endif
