# Copyright (c) 2019 Ste||ar Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# FIXME : in the future put it directly inside the cmake directory of the
# corresponding plugin

macro(setup_lci)
  if(NOT TARGET LCI::Shared)
    find_package(LCI REQUIRED)
  endif()
endmacro()

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  include(HPX_SetupMPI)
  setup_lci()
endif()
