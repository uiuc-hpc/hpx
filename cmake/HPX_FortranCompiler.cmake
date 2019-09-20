# Copyright (c) 2011 Bryce Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(CMakeDetermineFortranCompiler)

if(CMAKE_Fortran_COMPILER_ID)
  hpx_info("Enabling Fortran support")
  enable_language(Fortran)
else()
  hpx_info("Fortran support is disabled")
endif()

