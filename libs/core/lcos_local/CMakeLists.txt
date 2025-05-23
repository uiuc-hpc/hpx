# Copyright (c) 2019-2024 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/lcos_local/include
set(lcos_local_headers
    hpx/lcos_local/and_gate.hpp
    hpx/lcos_local/channel.hpp
    hpx/lcos_local/composable_guard.hpp
    hpx/lcos_local/conditional_cv.hpp
    hpx/lcos_local/conditional_trigger.hpp
    hpx/lcos_local/detail/preprocess_future.hpp
    hpx/lcos_local/receive_buffer.hpp
    hpx/lcos_local/trigger.hpp
)

# Default location is $HPX_ROOT/libs/lcos_local/include_compatibility
# cmake-format: off
set(lcos_local_compat_headers
    hpx/local_lcos.hpp => hpx/modules/lcos_local.hpp
    hpx/lcos/local/and_gate.hpp => hpx/include/local_lcos.hpp
    hpx/lcos/local/channel.hpp => hpx/include/local_lcos.hpp
    hpx/lcos/local/composable_guard.hpp => hpx/include/local_lcos.hpp
    hpx/lcos/local/conditional_trigger.hpp => hpx/include/local_lcos.hpp
    hpx/lcos/local/receive_buffer.hpp => hpx/include/local_lcos.hpp
    hpx/lcos/local/spmd_block.hpp => hpx/algorithms/parallel/spmd_block.hpp
    hpx/lcos/local/trigger.hpp => hpx/include/local_lcos.hpp
    hpx/local_lcos/and_gate.hpp => hpx/lcos_local/and_gate.hpp
    hpx/local_lcos/channel.hpp => hpx/lcos_local/channel.hpp
    hpx/local_lcos/composable_guard.hpp => hpx/lcos_local/composable_guard.hpp
    hpx/local_lcos/conditional_trigger.hpp => hpx/lcos_local/conditional_trigger.hpp
    hpx/local_lcos/receive_buffer.hpp => hpx/lcos_local/receive_buffer.hpp
    hpx/local_lcos/spmd_block.hpp => hpx/lcos_local/spmd_block.hpp
    hpx/local_lcos/trigger.hpp => hpx/lcos_local/trigger.hpp
)
# cmake-format: on

set(lcos_local_sources composable_guard.cpp preprocess_future.cpp)

include(HPX_AddModule)
add_hpx_module(
  core lcos_local
  SOURCES ${lcos_local_sources}
  HEADERS ${lcos_local_headers}
  COMPAT_HEADERS ${lcos_local_compat_headers}
  MODULE_DEPENDENCIES
    hpx_concurrency
    hpx_config
    hpx_execution
    hpx_executors
    hpx_futures
    hpx_lock_registration
    hpx_pack_traversal
    hpx_errors
    hpx_memory
    hpx_type_support
  CMAKE_SUBDIRS examples tests
)
