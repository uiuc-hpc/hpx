# Copyright (c) 2007-2013 Hartmut Kaiser
# Copyright (c) 2014-2015 Thomas Heller
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddLibrary)

# ##############################################################################
# Decide whether to use the LCI based parcelport
# ##############################################################################
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  hpx_debug("add_parcelport_lci_module")
  include(HPX_AddParcelport)
  add_parcelport(
    lci STATIC
    SOURCES "${PROJECT_SOURCE_DIR}/plugins/parcelport/lci/parcelport_lci.cpp"
    HEADERS
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/header.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/locality.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/receiver.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/receiver_connection.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/sender.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/sender_connection.hpp"
      "${PROJECT_SOURCE_DIR}/hpx/plugins/parcelport/lci/tag_provider.hpp"
          DEPENDENCIES hpx_actions hpx_command_line_handling hpx_core
          hpx_dependencies_boost Mpi::mpi LCI::LCI
    INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    FOLDER "Core/Plugins/Parcelport/LCI"
  )
endif()
