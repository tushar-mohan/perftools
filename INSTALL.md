PerfTools
=========


Requirements
------------

The only requirement to build PerfTools is the availability of gcc and standard Unix utilities
such as `make` and `autoconf`. Your system needs to support *shared libraries*.


## Enable access to CPU events
If you are using papiex/PAPI as a normal user then you may need to enable
collection of CPU event data for ordinary users. 

Check the existing value:

      $ cat /proc/sys/kernel/perf_event_paranoid
      1

You need a value of `-1`, `0` or `1`. `0` allows collection of CPU event data.
`1` also allows collection of kernel profiling data. `0` or `1` should suffice
for most needs.

      # echo 0 > /proc/sys/kernel/perf_event_paranoid



Build PapiEx
------------

The easiest way to build and use papiex is to use the bundled
PAPI and libmonitor. 

       $ make
       $ make test
       $ make fulltest

This builds and installs papiex in a sub-directory of the working
directory. If you would like to install it somewhere else, you can
supply the `PREFIX` argument to `make`.

      $ make PREFIX=/path/to/where/you/want/to/install/papiex install
      $ make PREFIX=... test
      $ make PREFIX=... fulltest


To use an existing PAPI installation, and not use the bundled PAPI:

      $ make PAPI_INC_PATH=/path/to/papi/install/include \
             PAPI_LIB_PATH=/path/to/papi/install/lib

      - or -

      $ make PAPI_PREFIX=/path/to/papi/install

Similarly you can use `MONITOR_PREFIX` or `MONITOR_INC_PATH` and
`MONITOR_LIB_PATH` to use an existing libmonitor. Please note 
papiex uses a version of monitor that comes out of the SciDAC project.

Build time arguments that are honored:

 * `MONITOR_PREFIX` or `MONITOR_INC_PATH` and `MONITOR_LIB_PATH`
 * `PAPI_PREFIX` or `PAPI_INC_PATH` and `PAPI_LIB_PATH`

To understand how to use papiex, see [README.md](README.md)


Platforms Tested
----------------

 * Haswell (Xeon(R) CPU E5-2698 v3), gcc 4.9.3
 * Ivy Bridge (Family: 6  Model: 58  Stepping: 9), gcc 5.4.0, Linux 4.10.0
 * POWER8E, gcc 4.8.5


Troubleshooting
---------------

* If papiex complains of error adding events, check that you have [enabled
  collection of CPU events](#enable-access-to-cpu-events) in the kernel. 

* If the PAPI or monitor libraries are not installed in the standard run time 
  linker search path then you had better set the environment variable
  `LD_LIBRARY_PATH` to point to the correct places. 

      $ setenv LD_LIBRARY_PATH /usr/local/lib:/opt/local/lib (for csh)
      $ export LD_LIBRARY_PATH /usr/local/lib:/opt/local/lib (for sh/bash/ksh)
