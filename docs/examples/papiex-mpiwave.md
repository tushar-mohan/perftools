In this post we will look at how `papiex` can be used to analyze an MPI application. The [program we will look at](https://computing.llnl.gov/tutorials/mpi/samples/C/mpi_wave.c) solves a concurrent wave equation using point-to-point communications.

```
      $ mkdir mpiwave; cd mpiwave
      $ curl https://computing.llnl.gov/tutorials/mpi/samples/C/mpi_wave.c | sed /draw_wave/d > mpi_wave.c
```

We use `sed` to remove references to a `draw_wave` function, which is not pertinent to this study.

## Building Papiex

Now let's download and build `papiex`:
```
      $ git clone https://github.com/tushar-mohan/perftools.git
      $ cd perftools
```

Before we start the build, let's make sure we have the build dependencies:
```
      $ ./build.sh --checkdeps
      Checking dependencies.. 
      gcc.. ok
      g++.. ok
      gfortran.. ok
      mpicc.. ok
      mpirun.. ok
      patch.. ok
      curl.. ok
      python.. ok
      cmake.. ok
      autoconf.. ok
      bunzip2.. ok

All build dependencies satisfied.
```

If you encounter problems with build dependencies, [check out the documentation](https://github.com/tushar-mohan/perftools/blob/master/README.md#build-dependencies) for your Linux distro.

Let's fire the build. We will use the defaults, and do a build of `papiex` alone, rather than the full build, which also includes HPCToolkit.

```
      $ ./build.sh
```

If all goes well, the output will contain something like:

```
=======================================================================
Tools are installed in:
/home/tushar/perftools/perftools-1.2.1

To use the tools
----------------
module load /home/tushar/perftools/perftools-1.2.1/perftools
	   - or -
source /home/tushar/perftools/perftools-1.2.1/perftools.sh
	   - or -
source /home/tushar/perftools/perftools-1.2.1/perftools.csh

To test install:
make test
make fulltest
=======================================================================
```

So, the tools were built, and installed in the subdirectory `perftools-1.2.1`.

Running a sanity test:
```
      $ make test
      ...
      basic-ng: papiex on serial, unthreaded program
      basic-ng: PASSED

      1 of 1 PASSED
```

If the test fails, you may need to [enable performance counter access for ordinary users](https://github.com/tushar-mohan/perftools/blob/master/README.md#enable-access-to-cpu-counters).

Let's add the tools to our shell environment:
```
      $ module load /home/tushar/perftools/perftools-1.2.1/perftools
```

You may also want to add the above command to our `.login` or `.profile`.

At this point, we have `papiex` in our `PATH`:
```
      $ papiex -V
      PerfTools version 1.2.1
      papiex driver version 1.2.6
      PAPI library version 5.4.0
      PAPI header version 5.4.1
      Build Feb 26 2018 16:07:35
```
