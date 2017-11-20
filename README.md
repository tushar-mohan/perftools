# PapiEx

**PapiEx** provides a command-line interface to [PAPI](http://icl.cs.utk.edu/papi/)

To build and install PapiEx, see the [build instructions](INSTALL.md). For a copy of the license,
see the [COPYING](COPYING) file in the same directory.


Table of Contents
=================
  * [Setting the environment](#setting-the-environment)
  * [Getting Started](#getting-started)
  * [Preset and Native Events](#preset-and-native-events)
  * [Measurement Modes](#measurement-modes)
  * [MPI and I/O Cycles](#mpi-and-io-cycles)
  * [Min, Max, Mean and Sum](#min-max-mean-and-sum)
  * [Task Memory and Resource Usage](#task-memory-and-resource-usage)
  * [Rank Mapping](#rank-mapping)
  * [Calipers](#calipers)
  * [Bugs and support](#bugs-and-support)


## Setting The Environment
Before you can use papiex, you must set your environment.

    $ source /path/to/papiex-install/papiex.sh (sh, Bourne, ksh)
           - or -
    $ source /path/to/papiex-install/papiex.csh (csh, tcsh)
           - or -
    $ module load /path/to/papiex-install/papiex (environment-modules)


## Getting Started
In your quest to understand application performance, you may find PapiEx to be a good starting point.

Let's start with a simple unthreaded program that does nothing but compute some flops.

    $ cat basic.c 
    #include <stdlib.h>

    #define ITERS 1000000000

    volatile double a = 0.1, b = 1.1, c = 2.1;

    int main(int argc, char **argv)
    {
      int i;
      printf("Doing %d iters. of a += b * c on doubles.\n",ITERS);
      for (i=0;i<ITERS;i++)
        a += b * c;
      return(0);
    }

    $ gcc -g -O2 -o basic basic

Now let's run the program under PapiEx. The `-a` automatically selects useful hardware metrics to measure, and provides a good starting point. If you don't have some specific metrics in mind already, we recommend starting with it.

    $ papiex -a ./basic
    $ papiex -a ./basic
    Doing 1000000000 iters. of a += b * c on doubles.
    papiex:
    papiex: 0.9.8 (Build Oct  9 2015/14:17:42)
    papiex: Send bug reports to papiex-bugs@perftools.org
    papiex:
    papiex: Storing raw counts data in [basic.papiex.supermicro.26719.1.txt]
    papiex:
    papiex: Output summary in [basic.papiex.supermicro.26719.1.report.txt]

PapiEx created two files in the directory: _basic.papiex.supermicro.26719.1.txt_ and _basic.papiex.supermicro.26719.1.report.txt_.

The first file contains raw event counts, and is not very useful for humans. It's used by other tools to easily parse the tool's output. Instead, lets focus on the report file _basic.papiex.supermicro.26719.1.report.txt_.

You can open the report in your favorite text editor.

The top section of the output contains high-level data about the run, the host where the program was executed, the start time, the end time, the number of processes, etc. 

    papiex version                : 0.9.8
    papiex build                  : Oct  9 2015/14:17:42
    Executable                    : /home/tushar/mm/perftools/tests/sanity/basic
    Processor                     : Intel(R) Xeon(R) CPU E3-1220 v3 @ 3.10GHz
    Clockrate (MHz)               : 3101.000000
    Hostname                      : supermicro
    Options                       : MULTIPLEX,MEMORY,PAPI_TOT_INS,PAPI_LST_INS,PAPI_BR_INS,PAPI_LD_INS,PAPI_SR_INS,PAPI_TOT_CYC,PAPI_RES_STL,PAPI_L1_DCM,PAPI_L1_ICM,PAPI_TLB_DM,PAPI_TLB_IM,PAPI_L2_DCM,PAPI_L2_ICM,PAPI_CA_INV,PAPI_STL_ICY,PAPI_FUL_ICY,PAPI_BR_CN,PAPI_BR_MSP,PAPI_L2_DCA,PAPI_L2_ICA,NO_MPI_PROF,NO_IO_PROF,NO_THREADSYNC_PROF,NEXTGEN
    Domain                        : User
    Parent process id             : 26718
    Process id                    : 26719
    Start                         : Mon Oct 12 11:55:27 2015
    Finish                        : Mon Oct 12 11:55:30 2015
    Num. of tasks                 : 1

The `Options` field is interesting. It tells which events were measured. The `-a` flag added a number of events automatically as you can see.

The next section is the real meat of the output. It contains useful derived metrics for the entire run, such the IPC (instructions per cycle), load-store ratio. On architectures where floating point operations can be counted, MFLOPs is also printed. 

    IPC ..........................................  8.92704e-01
    Load Store Ratio .............................  2.36022e+00
    Instructions Per Dcache Miss .................  4.54694e+06

    Time:
    Wallclock seconds ............................  2.59174e+00
    No Issue Stall seconds .......................  1.73513e-04
    Resource Stall seconds .......................  2.25136e+00

    Cycles:
    Cycles In Domain .............................  8.94917e+09
    Real Cycles ..................................  8.01044e+09
    Running Time In Domain % .....................  1.00000e+02
    Virtual Cycles ...............................  8.02688e+09

    Instructions:
    Total Instructions ...........................  7.98897e+09
    Memory Instructions % ........................  2.19889e+01
    Branch Instructions % ........................  1.24623e+01

    Memory:
    Load Store Ratio .............................  2.36022e+00
    L1 Data Misses Per 1000 Load Stores ..........  1.00018e-03
    L1 Instruction Misses Per 1000 Instructions ..  5.37992e-04
    L2 Data Misses Per 1000 L2 Load Stores .......  7.11178e+02
    L2 Instruction Misses Per 1000 L2 Instructions   4.18799e+02
    Data TLB Misses Per 1000 Load Stores .........  7.34336e-05
    Instruction TLB Misses Per 1000 Instructions .  1.32683e-05

    Stalls:
    Resource Stall Cycles % ......................  7.80125e+01
    No Issue Cycle % .............................  6.01246e-03
    Full Issue Cycle % ...........................  1.87814e-03
    Branch Misprediction % .......................  6.32780e-06

As can be seen, despite the tight loop 21% instructions are still load/stores. However the code has good cache performance, see the `L1 Data Misses Per 1000 Load Stores`. That's like 1 miss per million load stores. So memory system performance is very good. Yet, the `Resource Stall Cycles` is a high 78%. It's not the branch prediction that's the problem, as can be seen from the really low `Branch Misprediction %`. In fact, the resource stalls are stemming from the floating-point pipeline. 

## Preset and Native Events
`papiex` can measure both PAPI presets and architecture-specific native events.

PapiEx provides a useful flag `-l`:

    $ papiex -l
    ...
    PAPI_L1_DCM                    Level 1 data cache misses
    PAPI_L1_ICM                    Level 1 instruction cache misses
    PAPI_L2_DCM                    Level 2 data cache misses
    ...
    perf::PERF_COUNT_HW_CPU_CYCLES PERF_COUNT_HW_CPU_CYCLES
    perf::CYCLES                   PERF_COUNT_HW_CPU_CYCLES
    perf::CPU-CYCLES               PERF_COUNT_HW_CPU_CYCLES
    perf::PERF_COUNT_HW_INSTRUCTIONS PERF_COUNT_HW_INSTRUCTIONS
    perf::PERF_COUNT_HW_CACHE_MISSES PERF_COUNT_HW_CACHE_MISSES
    perf::CACHE-MISSES             PERF_COUNT_HW_CACHE_MISSES
    ...
    BACLEARS                       Branch re-steered
    BR_INST_EXEC                   Branch instructions executed
    BR_INST_RETIRED                Branch instructions retired (Precise Event)
    BR_MISP_EXEC                   Mispredicted branches executed
    BR_MISP_RETIRED                Mispredicted retired branches (Precise Event)
    CPL_CYCLES                     Unhalted core cycles at a specific ring level
    CPU_CLK_THREAD_UNHALTED        Count core clock cycles whenever the clock signal on the specific core is running (not halted)
    CPU_CLK_UNHALTED               Count core clock cycles whenever the clock signal on the specific core is running (not halted)
    CYCLE_ACTIVITY                 Stalled cycles
    DTLB_LOAD_MISSES               Data TLB load misses
    DTLB_STORE_MISSES              Data TLB store misses
    FP_ASSIST                      X87 floating-point assists

The event names not beginning with `PAPI_` and `perf::` are _native events_. Their availability and semantics depends on the processor architecture. Yet, they often provide insights not obtained using the PAPI presets.

If you would like to understand more about an event, you can do so using `-L`:

    $ papiex -L PAPI_TOT_CYC
    Preset Event                  : PAPI_TOT_CYC
    Short Description             : Total cycles
    Long Description              : Total cycles
    Derived                       : No
    Derived type                  : NOT_DERIVED
    Postfix processing string     : 
    Native name[0]                : CPU_CLK_THREAD_UNHALTED:THREAD_P
    Number of register values     : 0
    Native event description      : Count core clock cycles whenever the clock signal on the specific core is running (not halted), masks:Cycles when thread is not halted

Running `papiex` without any arguments usually correponds to:

    $ papiex -U -e PAPI_TOT_CYC -e PAPI_FP_OPS

We say *usually* because these events may not be available on some platforms,
and in such cases `papiex` uses other events by default.

## Measurement Modes

The `-U` flag instructs `papiex` to measure events in user mode. It can also
measure events in kernel (`-K), interrupt (`-I`) and supervisor (`-S`) modes.
You can also specify multiple modes on the command line to measure events in
more than one mode in a run.

Let's try measuring some native events:

    $ papiex -e RESOURCE_STALLS -e UOPS_EXECUTED -e LSD -e ARITH ./basic

In the output report, scroll down to the section showing the raw event counts. You will see something like:

    Process (PID 30101) counts data:
    ARITH ........................................  2.12000e+02
    LSD ..........................................  7.99992e+09
    RESOURCE_STALLS ..............................  6.99998e+09
    Real cycles ..................................  7.99132e+09
    Real usecs ...................................  2.58382e+06
    UOPS_EXECUTED ................................  9.00038e+09
    Virtual cycles ...............................  8.00688e+09
    Virtual usecs ................................  2.58203e+06
    Wallclock usecs ..............................  2.58393e+06

See the high count for the `LSD` -- the loop stream detector -- yet a low count for arithmetic multiply instructions - `ARITH`. The time is spent in a floating-point loop(s) that contains 90% of the uOPS executed.

## MPI and IO Cycles
When PapiEx is built with `PROFILING_SUPPORT=1`, it automatically counts cycles 
spent across MPI and I/O calls.

To build papiex with profiling, edit the Makefile, and set `PROFILING_SUPPORT=1`
on the `install-papiex` target. Then rebuild papiex, by typing `make`.

To run papiex for an MPI program, you would do so like:

    $ mpirun <mpirun-args> papiex <papiex-args> exectuable <executable args>

Let's try running `papiex` on a simple MPI program:

    $ mpirun -np 4 papiex -a tests/sanity/mpi_wave
    ...

Now open the output report, and you will see a section:

    IO Cycles % ..................................  4.37839e+01
    MPI Cycles % .................................  4.97252e-01

PapiEx intercepts a majority of the common MPI and I/O calls of interest.
The manpage provides a list of calls intercepted while computing the above metrics.

## Min, Max, Mean and Sum
Often in understanding load imbalance it helps to know the minimum and maxiumum
counts for an event. Sometimes we also want to know the aggregate count across 
the tasks. Papiex provides statistics for all measure events and even derived
metrics.

    Event                                           Sum          Min          Max          Mean         CV
    IO cycles ....................................  3.65164e+09  1.81331e+07  1.81637e+09  9.12909e+08  7.38384e-01
    MPI Sync cycles ..............................  0.00000e+00
    MPI cycles ...................................  4.14715e+07  8.24395e+06  1.13609e+07  1.03679e+07  1.19567e-01
    Mem. heap KB .................................  3.09120e+05  7.72480e+04  7.73760e+04  7.72800e+04  7.17205e-04
    Mem. library KB ..............................  3.63200e+04  9.08000e+03  9.08000e+03  9.08000e+03  0.00000e+00
    Mem. locked KB ...............................  0.00000e+00
    Mem. resident peak KB ........................  3.22240e+04  7.99200e+03  8.22000e+03  8.05600e+03  1.17761e-02
    Mem. shared KB ...............................  1.65440e+04  4.10400e+03  4.22000e+03  4.13600e+03  1.17456e-02
    Mem. stack KB ................................  2.21600e+03  5.52000e+02  5.60000e+02  5.54000e+02  6.25289e-03
    Mem. text KB .................................  6.40000e+01  1.60000e+01  1.60000e+01  1.60000e+01  0.00000e+00
    Mem. virtual peak KB .........................  0.00000e+00
    PAPI_BR_CN ...................................  3.49390e+06  0.00000e+00  3.49390e+06  8.73475e+05  1.73205e+00
    PAPI_BR_INS ..................................  3.45055e+07  6.30798e+06  1.31089e+07  8.62637e+06  3.16961e-01
    PAPI_BR_MSP ..................................  0.00000e+00
    PAPI_CA_INV ..................................  2.82987e+06  0.00000e+00  2.23084e+06  7.07468e+05  1.29036e+00
    PAPI_FUL_ICY .................................  2.61320e+07  0.00000e+00  2.61320e+07  6.53300e+06  1.73205e+00
    PAPI_L1_DCM ..................................  2.68217e+06  6.05530e+05  7.44576e+05  6.70542e+05  7.50420e-02
    PAPI_L1_ICM ..................................  1.49308e+06  3.20007e+05  4.50128e+05  3.73271e+05  1.28369e-01
    PAPI_L2_DCA ..................................  0.00000e+00
    PAPI_L2_DCM ..................................  1.28810e+06  2.37355e+05  4.30562e+05  3.22025e+05  2.25860e-01
    PAPI_L2_ICA ..................................  1.49308e+06  3.20007e+05  4.50128e+05  3.73271e+05  1.28369e-01
    PAPI_L2_ICM ..................................  4.60093e+05  0.00000e+00  2.02054e+05  1.15023e+05  6.33660e-01
    PAPI_LD_INS ..................................  4.61218e+07  5.94085e+06  2.11986e+07  1.15304e+07  5.08805e-01
    PAPI_LST_INS .................................  6.46438e+07  9.37767e+06  2.63381e+07  1.61610e+07  3.96569e-01
    PAPI_RES_STL .................................  2.27465e+07  4.35786e+06  6.47534e+06  5.68663e+06  1.42851e-01
    PAPI_SR_INS ..................................  1.85220e+07  3.43682e+06  5.56544e+06  4.63051e+06  1.74799e-01
    PAPI_STL_ICY .................................  9.69780e+06  0.00000e+00  9.69780e+06  2.42445e+06  1.73205e+00
    PAPI_TLB_DM ..................................  1.58648e+05  2.79770e+04  4.64160e+04  3.96620e+04  1.80425e-01
    PAPI_TLB_IM ..................................  7.88760e+04  1.35610e+04  3.08000e+04  1.97190e+04  3.39682e-01
    PAPI_TOT_CYC .................................  1.26881e+08  2.63082e+07  3.73453e+07  3.17204e+07  1.29175e-01
    PAPI_TOT_INS .................................  2.94639e+08  4.11702e+07  9.93185e+07  7.36598e+07  2.83240e-01
    
The `Sum` field aggregates across all tasks. This is very useful when computing, for example,
aggregate floating-point operations across tasks in determining scaling behavior. The `CV` is
coefficient of variation and is derived by dividing the standard deviation with the mean. It 
provides a measure of imbalance.


## Task Memory and Resource Usage
PapiEx provides stats on memory (`-x`) and resource usage (`-r`).
The `-a` automatically implies `-x`.

    Mem. heap KB .................................  1.09200e+03
    Mem. library KB ..............................  7.08400e+03
    Mem. locked KB ...............................  0.00000e+00
    Mem. resident peak KB ........................  4.26000e+03
    Mem. shared KB ...............................  2.48800e+03
    Mem. stack KB ................................  5.44000e+02
    Mem. text KB .................................  4.00000e+00
    Mem. virtual peak KB .........................  0.00000e+00

## Rank Mapping
For MPI jobs `papiex` provides a mapping of the MPI rank to the
host as well as a sorted list by rank for each event.

    Rank mapping:
    [0] => node101 (PID 168)
    [1] => node203 (PID 171)
    [2] => node105 (PID 174)
    [3] => node106 (PID 176)

Above, we see that the MPI rank `0` was placed on host `node101`.

Below we see a sorted list for each event, and the number in the square
bracket shows which MPI rank that corresponded to. For example, the 
maximum `IO cycles` occurred in task `0`.

    Rank counts data (by field):
    IO cycles
     1.81637e+09 [0]
     1.22527e+09 [1]
     5.91856e+08 [2]
     1.81331e+07 [3]
    
    MPI Sync cycles
     0.00000e+00 [3]
     0.00000e+00 [0]
     0.00000e+00 [1]
     0.00000e+00 [2]
    
    MPI cycles
     1.13609e+07 [3]
     1.10055e+07 [1]
     1.08611e+07 [2]
     8.24395e+06 [0]
    
    Mem. heap KB
     7.73760e+04 [0]
     7.72480e+04 [1]
     7.72480e+04 [2]
     7.72480e+04 [3]
    ...



## Calipers
PapiEx allows you to insert measure performance stats between arbitrary
instrumentation points, called calipers. You can insert multiple such
calipers, and even give these calipers helpful names. Let's try an example:

    $ cat caliper.c
    #include <stdio.h>
    #include "papiex.h"
    
    volatile double a = 0.1, b = 1.1, c = 2.1;
    
    int main(int argc, char **argv)
    {
      int i;
    
      PAPIEX_START_ARG(1,"printf");
      printf("Doing 100000000 iters. of a += b * c on doubles.\n");
      PAPIEX_STOP_ARG(1);
    
      PAPIEX_START_ARG(2,"for loop");
      for (i=0;i<100000000;i++)
        a += b * c;
      PAPIEX_STOP_ARG(2);
    
      return(0);
    }

`PAPIEX_START_ARG` and `PAPIEX_STOP_ARG` are useful macros that act like
sentinels. They expand to calls to `papiex_start` and `papiex_stop`. These
calls are explained in the `papiex` manpage. You may also use the simpler
variants `PAPIEX_START` and `PAPIEX_STOP` if you do not wish to name your
calipers. In this example, we create two named calipers -- `printf`
and `for loop`. Notice, we included `papiex.h` in the source file, so these
macros can be expanded, and prototypes for `papiex_start` and `papiex_stop`
can be found.

Now compile the code:

    $ gcc caliper.c `papiex-config --cflags --libs` -o caliper

`papiex-config` is a useful command that gives the include and library
paths needed to find the `papiex` header and library.

Now run the executable under `papiex`:

    $ papiex -a ./caliper 
    Doing 100000000 iters. of a += b * c on doubles.
    papiex:
    papiex: 1.0.0 (Build Nov 30 2015/17:20:44)
    papiex: Send bug reports to papiex-bugs@perftools.org
    papiex:
    papiex: Storing raw counts data in [caliper.papiex.163a34055b91.318.1.txt]
    papiex:
    papiex: Output summary in [caliper.papiex.163a34055b91.318.1.report.txt]

If you inspect the `papiex` report generated, you will see measurements for three
sections -- the full program, the `printf` caliper and the `for loop` caliper:

    ...
    IO cycles ....................................  1.94421e+05
    Mem. heap KB .................................  1.09200e+03
    ...
    Mem. text KB .................................  4.00000e+00
    Mem. virtual peak KB .........................  0.00000e+00
    PAPI_BR_CN ...................................  6.73444e+07
    PAPI_BR_INS ..................................  9.86034e+07
    PAPI_BR_MSP ..................................  0.00000e+00
    ...
    PAPI_TLB_DM ..................................  9.00000e+01
    PAPI_TLB_IM ..................................  1.03000e+02
    PAPI_TOT_CYC .................................  8.86579e+08
    PAPI_TOT_INS .................................  9.06886e+08
    Real cycles ..................................  8.09626e+08
    Real usecs ...................................  2.61793e+05
    Thr Sync cycles ..............................  0.00000e+00
    Virtual cycles ...............................  8.11265e+08
    Virtual usecs ................................  2.61616e+05
    Wallclock usecs ..............................  2.62462e+05
      [1] Measurements ...........................  1.00000e+00
      [1] PAPI_BR_CN .............................  0.00000e+00
      [1] PAPI_BR_INS ............................  2.55900e+03 [  0.0]
      [1] PAPI_L1_DCM ............................  2.58000e+02 [  7.5]
      [1] PAPI_L1_ICM ............................  3.66000e+02 [  8.2]
      ...
      [1] PAPI_TOT_CYC ...........................  1.74510e+04 [  0.0]
      [1] PAPI_TOT_INS ...........................  1.15510e+04 [  0.0]
      [1] Real cycles ............................  1.58901e+05 [  0.0]
      [2] Measurements ...........................  1.00000e+00
      [2] PAPI_BR_CN .............................  6.73444e+07 [100.0]
      [2] PAPI_BR_INS ............................  9.85970e+07 [100.0]
      [2] PAPI_BR_MSP ............................  0.00000e+00
      [2] PAPI_CA_INV ............................  1.60000e+01 [100.0]
      [2] PAPI_FUL_ICY ...........................  1.14970e+04 [100.0]
      [2] PAPI_L1_DCM ............................  1.53500e+03 [ 44.5]
      [2] PAPI_L1_ICM ............................  1.94900e+03 [ 43.5]
      [2] PAPI_L2_DCA ............................  3.97000e+02 [100.0]
      [2] PAPI_L2_DCM ............................  1.48000e+02 [ 13.8]
      [2] PAPI_L2_ICA ............................  1.94900e+03 [ 43.5]
      [2] PAPI_L2_ICM ............................  4.40000e+02 [ 21.4]
      [2] PAPI_LD_INS ............................  2.34640e+08 [100.0]
      [2] PAPI_LST_INS ...........................  3.43900e+08 [100.0]
      [2] PAPI_RES_STL ...........................  6.22862e+08 [100.0]
      [2] PAPI_SR_INS ............................  1.09260e+08 [100.0]
      [2] PAPI_STL_ICY ...........................  3.43940e+04 [100.0]
      [2] PAPI_TLB_DM ............................  7.00000e+00 [  7.8]
      [2] PAPI_TLB_IM ............................  1.70000e+01 [ 16.5]
      [2] PAPI_TOT_CYC ...........................  8.86528e+08 [100.0]
      [2] PAPI_TOT_INS ...........................  9.06856e+08 [100.0]
      [2] Real cycles ............................  8.09104e+08 [ 99.9]
    
    Caliper Label Map:
      [2]   for loop
      [1]   printf

The unindented mesurements are for the full program. The indented
entries with `[1]` and `[2]` correspond to the regions between the
two calipers -- `printf` and `for loop`. There is a map at the bottom
that shows the mapping of the caliper indices to the names. The
numebr in the square bracket indicates the `%` of counts for the
caliper point relative to the program as a whole. For example

      [2] PAPI_TLB_IM ............................  1.70000e+01 [ 16.5]

means that the TLB instruction misses in the caliper region `[2]` were
`16.5%` of the process's entire TLB instruction misses.

When using calipers it is important to use them around regions that do
run significant work. Our example of using it around a `printf` is not
a good idea.    

## Bugs and Support
Papiex is supported on a voluntary basis, and as such we rely on 
community participation to help find bugs and implement fixes.

Please send your bug reports to: 

    papiex-bugs@perftools.org
