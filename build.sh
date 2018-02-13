#!/bin/bash


usage="

    `basename $0` [--all|--clean] [PREFIX=/path/to/install/dir] [CC=gcc] [MPICC=mpicc] [FORTRAN=ifort]...

All arguments are optional. Variables set on command-line are passed to make.
If PREFIX is not set then tools are installed a sub-directory of the current directory.

Set PAPI_PREFIX (or PAPI_INC_PATH and PAPI_LIB_PATH) to use a custom build of PAPI.

e.g.,

    $ ./build.sh --all PREFIX=/opt/perftools

"

function do_install() {
    if [ $# -ne 0 ] ; then
	target=$1
    else
	target=""
    fi
    make $target
}

option=$1
shift
make_args="$@"

case $option in
      "") do_install ;;
   --all) do_install all ;;
 --clean) make clobber-all ;;
       *) echo -e "$usage"; exit 1;;
esac
