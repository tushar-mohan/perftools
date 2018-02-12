Perftools
=========
*Perftools* is a collection of performance tools for HPC environments.
The tools allow low-overhead measurement of CPU metrics, memory usage,
I/O and MPI. 

This suite consists of:

* [papiex](papiex/README.md) - provides a simple and uniform interface to collect 
detailed job, process and thread-level statistics using [PAPI](http://icl.cs.utk.edu//papi). 
* ioex - collects and displays I/O statistics in uninstrumented programs (coming soon!)
* [hpctoolkit](http://hpctoolkit.org/) - PerfTools provides a build of Rice University's
HPCToolKit, along with helper scripts to facilitate compatibility with PerfBrowser.


Dependencies
------------
The following, publicly-available Open Source softwares may be used as
part of the *Perftools* build process. Some of them are patched to support 
additional functionality. They are included with the distribution or if not
found, downloaded automatically from the Internet and compiled.

* [Libpfm](http://perfmon2.sourceforge.net/manv4/libpfm.html) (required)
* [PAPI](http://icl.cs.utk.edu/papi/) (required)
* [Libmonitor](https://code.google.com/p/libmonitor/) (required)
* [mpiP](http://mpip.sourceforge.net/) (optional)
* [Hpctoolkit](http://hpctoolkit.org/) (optional)
* [libunwind](http://www.nongnu.org/libunwind/) (optional)

Getting the tools
-----------------

      $ git clone https://github.com/tushar-mohan/perftools.git

Setting up the Build Environment
--------------------------------
`perftools` requires a number of operating system and build
utilities, such `make`, GNU-toolchain, `autoconf`, MPI, etc.


Build the tools
---------------

  To build the minimal (PAPI and papiex) set of tools:
     
      $ ./build.sh

  To build the full set of tools, including HPCToolkit do:

      $ ./build.sh --all

The script honors `PREFIX` to set the install directory:

      $ ./build --all PREFIX=/opt/perftools-1.0.0

If your build environment is non-GNU, you may want to set
`CC`, `CXX`, `FORTRAN`, `MPICC`, `MPIF77`, `CC_OMPMPI` and `CC_OMP` 
on the command-line of the build script, like:

      $ ./build.sh --all CC=icc CXX=iCC FORTRAN=ifort

You can also use `make` directly. Type `make help` for instructions.

The build will download third-party dependencies during the build.
If you prefer, you can download all the dependencies first with:

      $ ./build --download

The build-system keeps a cached copy of the downloads in the
`distfiles/` sub-directory to avoid repeating a download.


Testing the tools
----------------

To quickly test the tools:

      $ make test

For complete coverage:

      $ make fulltest


Using the tools
---------------

Your tools install top-level directory contains a few scripts to help set up the environment.

For systems where `module` is in use:

    module load /path/to/perftools

For Bourne and C-shell users, perform one of the following:

    $ source /path/to/perftools.sh
    $ source /path/to/perftools.csh

Then you are free to run:

    $ papiex ...
    $ mpirun ... papiex ...

And similar for the other tools that may have been built:

    $ mpirun ... mpipex ...
    $ mpirun ... hpcex ...

Documentation is available in the form of man pages and a Wiki here.


License
-------
MIT

*Note* - During the build third-party software is downloaded that maybe
covered under separate licenses. Please refer to individual software
components, such as PAPI and HPCToolkit for their respective licenses.
