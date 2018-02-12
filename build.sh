#!/bin/bash


usage="
`basename $0` <--all|--clean> [PREFIX=/path/to/install/directory] [CC=gcc] [MPICC=mpicc] [FORTRAN=ifort]...

All arguments after the first are optional, and are passed to make.
If PREFIX is not set then tools are installed a sub-directory of the current directory.

e.g.,

$ ./build.sh --all PREFIX=/opt/perftools"

BUILD_LOG="$PWD/build.log"

function do_download() {
    make download-all
}

function do_install() {
    if [ $# -ne 0 ] ; then
	target=$1
    else
	target=""
    fi
    make $target
    if [ $? -ne 0 ]; then
	echo "Your build *** FAILED ***. tail $BUILD_LOG shows.... "
	echo "-----------------------------------------------------"
	tail $BUILD_LOG
	exit $rc
    fi
}

option=$1
shift
make_args="$@"

case $option in
  "") do_install ;;
  --all) do_install all ;;
  --download) do_download ;;
  --clean) make clobber-all ; rm -f $BUILD_LOG ;;
  *) echo -e "$usage"; exit 1;;
esac
