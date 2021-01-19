# Copyright (c) 2019 Ste||ar Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# FIXME : in the future put it directly inside the cmake directory of the
# corresponding plugin

# TODO: include LCI (and eventually get rid of MPI, but not right now)

include(HPX_Message)

#find_package(LCI)
#link_libraries(LCI::Shared)

macro(setup_lci)
  if(NOT TARGET LCI::Shared)
    find_package(LCI)
    add_library(LCI INTERFACE IMPORTED)
    #link_libaries(LCI::Shared)
  endif()
  #link_libraries(LCI::Shared)
  #if(NOT TARGET Mpi::mpi)
    #find_package(LCI)
    #link_libraries(LCI::Shared)
    #endif()
endmacro()

# FIXME : not sure if this comment is still up-to-date If we compile with the
# MPI parcelport enabled, we need to additionally add the MPI include path here,
# because for the main library, it's only added for the plugin.
if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI) OR HPX_WITH_ASYNC_MPI)
  setup_lci()
endif()
