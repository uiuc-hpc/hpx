# Copyright (c) 2007-2019 Hartmut Kaiser
# Copyright (c) 2011-2014 Thomas Heller
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
# Copyright (c)      2011 Vinay C Amatya
# Copyright (c)      2013 Jeroen Habraken
# Copyright (c) 2014-2016 Andreas Schaefer
# Copyright (c) 2017      Abhimanyu Rawat
# Copyright (c) 2017      Google
# Copyright (c) 2017      Taeguk Kwon
# Copyright (c) 2018 Christopher Hinz
# Copyright (c) 2020      Weile Wei
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(HPX_WITH_LIBCDS_GIT_REPOSITORY https://github.com/STEllAR-GROUP/libcds.git)
set(HPX_WITH_LIBCDS_GIT_TAG hpx_backend_alex_hpGeneric)
set(FETCHCONTENT_UPDATES_DISCONNECTED_libcds ON)

if(HPX_WITH_LIBCDS)
  include(FetchContent)
  include(HPX_Message)

  set(LIBCDS_GENERATE_SOURCELIST ON)

  set(LIBCDS_WITH_HPX ON)
  set(LIBCDS_AS_HPX_MODULE ON)

  hpx_info(
    "Fetching libCDS from repository: ${HPX_WITH_LIBCDS_GIT_REPOSITORY}, "
    "tag: ${HPX_WITH_LIBCDS_GIT_TAG}"
  )
  fetchcontent_declare(
    libcds
    GIT_REPOSITORY ${HPX_WITH_LIBCDS_GIT_REPOSITORY}
    GIT_TAG ${HPX_WITH_LIBCDS_GIT_TAG}
    GIT_SHALLOW TRUE
  )
  fetchcontent_getproperties(libcds)

  if(NOT libcds_POPULATED)
    fetchcontent_populate(libcds)
    set(LIBCDS_CXX_STANDARD ${HPX_CXX_STANDARD})
    add_subdirectory(${libcds_SOURCE_DIR} ${libcds_BINARY_DIR})
    list(TRANSFORM LIBCDS_SOURCELIST PREPEND "${libcds_SOURCE_DIR}/")
    set(LIBCDS_SOURCE_DIR ${libcds_SOURCE_DIR})
  endif()

endif()
