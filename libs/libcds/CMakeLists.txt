# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# if the user has not enabled libCDS, exit immediately
if(NOT ${HPX_WITH_LIBCDS})
  return()
endif()

include(HPX_SetupLibCDS)

if(NOT libcds_POPULATED)
  hpx_error("HPX_WITH_LIBCDS was set to ON, but HPX failed to fetch LibCDS")
endif()

hpx_add_config_define(HPX_HAVE_LIBCDS)

# Default location is $HPX_ROOT/libs/libcds/include
set(libcds_headers hpx/libcds/hpx.hpp hpx/libcds/hpx_manager.hpp)

# Default location is $HPX_ROOT/libs/libcds/include_compatibility
set(libcds_compat_headers)

set(libcds_sources src/hpx_tls_manager.cpp ${LIBCDS_SOURCELIST})

include(HPX_AddModule)
add_hpx_module(
  libcds
  COMPATIBILITY_HEADERS OFF SOURCE_PATH_ABSOLUTE
  DEPRECATION_WARNINGS
  GLOBAL_HEADER_GEN OFF
  SOURCES ${libcds_sources}
  HEADERS ${libcds_headers}
  COMPAT_HEADERS ${libcds_compat_headers}
  DEPENDENCIES hpx_assertion hpx_config hpx_concepts hpx_type_support
               hpx_threading_base hpx_threading
  CMAKE_SUBDIRS examples tests
)

target_include_directories(hpx_libcds PRIVATE ${LIBCDS_SOURCE_DIR})
target_compile_definitions(hpx_libcds PUBLIC CDS_THREADING_HPX)
target_compile_definitions(hpx_libcds PUBLIC CDS_EXPORT_API=HPX_EXPORT)
