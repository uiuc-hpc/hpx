# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  # setup LCI, if necessary
  include(HPX_SetupLCI)
  set(additional_dependencies Mpi::mpi LCI::Shared)
else()
  return()
endif()

# Default location is $HPX_ROOT/libs/lci_base/include
set(lci_base_headers hpx/lci_base/lci.hpp hpx/lci_base/lci_environment.hpp)

# Default location is $HPX_ROOT/libs/lci_base/include_compatibility
# cmake-format: off
set(lci_base_compat_headers
    hpx/plugins/parcelport/lci/lci.hpp => hpx/modules/lci_base.hpp
    hpx/plugins/parcelport/lci/lci_environment.hpp => hpx/modules/lci_base.hpp
)
# cmake-format: on

set(lci_base_sources lci_environment.cpp)

include(HPX_AddModule)
add_hpx_module(
  core lci_base
  GLOBAL_HEADER_GEN ON
  SOURCES ${lci_base_sources}
  HEADERS ${lci_base_headers}
  COMPAT_HEADERS ${lci_base_compat_headers}
  MODULE_DEPENDENCIES hpx_runtime_configuration hpx_util
  DEPENDENCIES ${additional_dependencies}
  CMAKE_SUBDIRS examples tests
)
