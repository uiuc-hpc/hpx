..
    Copyright (c) 2020 Weile Wei
    Copyright (c) 2020 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _libs_libcds:

======
libcds
======

LibCDS Overview
###############

`LibCDS <https://github.com/khizmax/libcds>`_ implements a collection of
oncurrent containers that don't require external (manual) synchronization
for shared access, and safe memory reclamation (SMR) algorithms like
Hazard Pointer and user-space RCU that is used as an epoch-based SMR.

Supporting features
###################
Currently, this module **only** supports *Hazard Pointer*
(a type of safe memory reclamation schemas/garbage collectors)
based containers in LibCDS
at HPX light-weight user-level thread. In the future, we might consider
adding support to other containers, including dynamic Hazard Pointer, RCU, etc,
at HPX user-level thread,
and might also explore suitable containers (i.e. flat-combining queue) in HPX
runtime scheduler.

To find out which container supports Hazard Pointer based garbage collector,
one might want to check
`LibCDS Documentation <http://libcds.sourceforge.net/doc/cds-api/index.html>`_.
For example, after clicking `cds > Modules > FeldmanHashMap <http://libcds.sourceforge.net/doc/cds-api/classcds_1_1container_1_1_feldman_hash_map.html>`_
, one can find Template parameters in FeldmanHashMap class
:cpp:class:`cds::container::FeldmanHashMap< GC, Key, T, Traits >`
suggests `GC - safe memory reclamation schema. Can be gc::HP, gc::DHP or one of RCU type`.

Build Hazard Pointer w/ HPX threads
###################################
To build and your own lock free container in LibCDS using
HPX light-weight user-level thread, one might first get familiar with
`LibCDS <https://github.com/khizmax/libcds>`_ itself. The simplest way to
launch Hazard Pointer in HPX threads is to do the following:

.. code-block:: cpp
    #include <hpx/hpx_init.hpp>
    #include <cds/init.h>       // for cds::Initialize and cds::Terminate
    #include <cds/gc/hp.h>      // for cds::HP (Hazard Pointer) SMR

    int hpx_main(int, char**)
    {
        // Initialize libcds
        cds::Initialize();

        {
            // Initialize Hazard Pointer singleton
            const std::size_t nHazardPtrCount =
                1;    // Hazard pointer count per thread
            const std::size_t nMaxThreadCount = hpx::cds::hpxthread_manager_wrapper::
                max_concurrent_attach_thread_;    // Max count of simultaneous working thread in the application, default 100
            const std::size_t nMaxRetiredPtrCount =
                16;    // Capacity of the array of retired objects for the thread
            using hp_hpxtls =
                cds::gc::hp::custom_smr<cds::gc::hp::details::HPXTLSManager>;
            hp_hpxtls::construct(
                nHazardPtrCount, nMaxThreadCount, nMaxRetiredPtrCount);

            // If main thread uses lock-free containers
            // the main thread should be attached to libcds infrastructure
            hpx::cds::hpxthread_manager_wrapper cdswrap;

            // Now you can use HP-based containers in the main thread
            //...
        }

        // Terminate libcds
        cds::Terminate();

        return hpx::finalize();
    }

    int main(int argc, char* argv[])
    {
        return hpx::init(argc, argv);
    }

Use Hazard Pointer supported Container w/ HPX threads
#####################################################

To note, to use Hazard Pointer in the context of HPX user-level threads,
one must use LibCDS template
TLS manager :cpp:type:`cds::gc::hp::custom_smr<T>` supplied with
:cpp:type:`cds::gc::hp::details::HPXTLSManager`. This ensures the thread data is bound
to user-level thread, as hpx thread can migrate from one kernel thread to another
depending on HPX scheduler.
Without this, Hazard Pointer
will use default kernel thread and thus keep thread-private data at kernel-level
threads, for example,
:cpp:type:`cds::gc::hp::custom_smr<cds::gc::hp::details::DefaultLSManager>`.

To use any Hazard Pointer supported container, one also needs to populate
:cpp:type:`cds::gc::hp::details::HPXTLSManager` to all levels of the container.
One simplest map is :cpp:type:`FeldmanHashMap`:

.. code-block:: cpp
    using gc_type = cds::gc::custom_HP<cds::gc::hp::details::HPXTLSManager>;
    using key_type = std::size_t;
    using value_type = std::string;
    using map_type =
    cds::container::FeldmanHashMap<gc_type, key_type, value_type>;

A more complex map example can be found in `libcds_michael_map_hazard_pointer.cpp`,
where the map is built on top of a list. In this case, both map and list need to
use :cpp:type:`cds::gc::hp::details::HPXTLSManager` to template the Garbage Collector
type.

API
#####################################################

The following API functions are exposed:

- :cpp:func:`hpx::cds::hpxthread_manager_wrapper`: This is a wrapper of
:cpp:func:`cds::gc::hp::custom_smr<cds::gc::hp::details::HPXTLSManager>::attach_thread()`
and :cpp:func:`cds::gc::hp::custom_smr<cds::gc::hp::details::HPXTLSManager>::detach_thread()`
This allows the calling hpx thread attach to Hazard Pointer threading infrastructure.

- :cpp:var:`hpx::cds::hpxthread_manager_wrapper::max_concurrent_attach_thread_`:
This variable of :cpp:type:`std::atomic<std::size_t>`
is corresponding variable in LibCDS's :cpp:var:`nMaxThreadCount` in Hazard Pointer class.
This variable sets max count of thread with using HP GC in your application.
Default is 100. More reference can be found in
`HP in LibCDS <https://github.com/khizmax/libcds/blob/master/cds/gc/hp.h>`_.


See the :ref:`API reference <libs_libcds_api>` of this module for more
details.

