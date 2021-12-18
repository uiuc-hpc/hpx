# Copyright (c) 2019 Ste||ar Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# FIXME : in the future put it directly inside the cmake directory of the
# corresponding plugin

macro(setup_lci)
  if(NOT TARGET LCI::LCI)
    if(NOT HPX_WITH_FETCH_LCI)
      find_package(LCI REQUIRED)
    elseif(NOT HPX_FIND_PACKAGE)
      if(FETCHCONTENT_SOURCE_DIR_LCI)
        hpx_info(
                "HPX_WITH_FETCH_LCI=${HPX_WITH_FETCH_LCI}, LCI will be used through CMake's FetchContent and installed alongside HPX (FETCHCONTENT_SOURCE_DIR_LCI=${FETCHCONTENT_SOURCE_DIR_LCI})"
        )
      else()
        hpx_info(
                "HPX_WITH_FETCH_LCI=${HPX_WITH_FETCH_LCI}, Asio will be fetched using CMake's FetchContent and installed alongside HPX (HPX_WITH_LCI_TAG=${HPX_WITH_LCI_TAG})"
        )
      endif()
      include(FetchContent)
      fetchcontent_declare(
              lci
              GIT_REPOSITORY https://github.com/uiuc-hpc/LC.git
              GIT_TAG ${HPX_WITH_LCI_TAG}
      )

      fetchcontent_getproperties(lci)
      if(NOT lci_POPULATED)
        fetchcontent_populate(lci)
        set(LCI_WITH_EXAMPLES OFF CACHE INTERNAL "")
        set(LCI_WITH_TESTS OFF CACHE INTERNAL "")
        set(LCI_WITH_BENCHMARKS OFF CACHE INTERNAL "")
        set(LCI_WITH_DOC OFF CACHE INTERNAL "")
        set(LCI_SERVER ibv CACHE INTERNAL "")
        set(LCI_PM_BACKEND mpi CACHE INTERNAL "")
        add_subdirectory(${lci_SOURCE_DIR} ${lci_BINARY_DIR})
        add_library(LCI::LCI ALIAS LCI)
      endif()

      install(
              TARGETS LCI
              EXPORT HPXLCITarget
              COMPONENT core
              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      )

      install(
              DIRECTORY ${lci_SOURCE_DIR}/include/
              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
              COMPONENT core
              FILES_MATCHING
              PATTERN "*.h"
      )

      export(
              TARGETS LCI
              NAMESPACE LCI::
              FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXLCITarget.cmake"
      )

      install(
              EXPORT HPXLCITarget
              NAMESPACE LCI::
              FILE HPXLCITarget.cmake
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
      )

      install(
              FILES "${lci_SOURCE_DIR}/cmake_modules/FindFabric.cmake"
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
      )
    endif()
  endif()
endmacro()

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  include(HPX_SetupMPI)
  setup_lci()
endif()
