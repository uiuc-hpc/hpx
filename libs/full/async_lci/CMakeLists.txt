# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# Note: HPX_WITH_ASYNC_MPI is handled in the main CMakeLists.txt

# if the user does not want lci async futures, quit - the module will not be
# enabled
if(NOT ${HPX_WITH_ASYNC_LCI})
  return()
endif()

# setup MPI, if necessary
if(NOT TARGET Mpi::mpi)
  include(HPX_SetupMPI)
  setup_mpi()
  include(HPX_SetupLCI)
  setup_lci()
endif()

# Default location is $HPX_ROOT/libs/lci/include
set(async_lci_headers hpx/async_lci/lci_executor.hpp
                      hpx/async_lci/lci_future.hpp
)

# Default location is $HPX_ROOT/libs/lci/src
set(lci_sources lci_future.cpp)

include(HPX_AddModule)
add_hpx_module(
  full async_lci
  GLOBAL_HEADER_GEN ON
  SOURCES ${lci_sources}
  HEADERS ${async_lci_headers}
  DEPENDENCIES hpx_core hpx_parallelism Mpi::mpi LCI::Shared
  MODULE_DEPENDENCIES hpx_lci_base hpx_runtime_local
  CMAKE_SUBDIRS examples tests
)
