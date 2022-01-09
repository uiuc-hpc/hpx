# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(version_headers hpx/version.hpp)

set(version_sources version.cpp)

if(HPX_WITH_NETWORKING)
  if(HPX_WITH_PARCELPORT_MPI OR HPXLocal_WITH_ASYNC_MPI)
    include(HPXLocal_SetupMPI)
    hpx_local_setup_mpi()
    set(additional_dependencies Mpi::mpi)
  endif()

  if(HPX_WITH_PARCELPORT_LCI)
    include(HPX_SetupLCI)
    hpx_setup_lci()
    set(additional_dependencies Mpi::mpi LCI::LCI)
  endif()
endif()

include(HPX_AddModule)
add_hpx_module(
  full version
  GLOBAL_HEADER_GEN OFF
  SOURCES ${version_sources}
  HEADERS ${version_headers}
  MODULE_DEPENDENCIES hpx_config
  DEPENDENCIES ${additional_dependencies}
)

target_include_directories(
  hpx_version PRIVATE $<BUILD_INTERFACE:${HPX_BINARY_DIR}>
)
