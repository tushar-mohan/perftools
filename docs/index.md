[perftools](about) is an integration of Open Source performance tools 
that collect application performance data for high-performance computing
linux clusters.

`perftools` consists of the following open source tools/libraries:

  * papiex, [papi](http://icl.cs.utk.edu/papi/), libmonitor
  * [hpctoolkit](http://hpctoolkit.org)
  * ioex

`perftools` lays an emphasis on simple command-line usage. It favors saving 
output in open readable text, as opposed to proprietary binary formats. 
It also selects tools that run in a terminal, rather that requiring a 
windowing environment.

Start using `perftools` by following the posts below in sequence.
These posts explain how to build `perftools`, gather data using it,
and finally how to use [PerfBrowser Cloud](https://perfbrowser.perftools.org/)
to visualize the data.
 * [counting cpu metrics with papiex](examples/papiex-mpi-example/)
 * [profiling with hpcrun-flat](examples/hpcrun-mpi-example/)
