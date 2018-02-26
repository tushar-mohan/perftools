#!/bin/bash


usage=$(cat <<-EOT
	./build.sh [--all|--clean|--checkdeps] [PREFIX=/path/to/install/dir] [CC=gcc] [FORTRAN=ifort]...

	All arguments are optional. Variables set on command-line are passed to make.
	If PREFIX is not set then tools are installed a sub-directory of the current directory.

	Set PAPI_PREFIX (or PAPI_INC_PATH and PAPI_LIB_PATH) to use a custom build of PAPI.

	e.g.,
	To build papiex and its dependencies:
	$ ./build.sh

	To build all the perftools (papiex, hpctoolkit,..):
	$ ./build.sh --all

	To use a custom build of PAPI:
	$ ./build.sh PAPI_PREFIX=/usr/src/papi-5.4.1

	To check if build tools and dependencies are installed:
	$ ./build --checkdeps
EOT
)

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
 --checkdeps) make checkdeps ;;
       *) echo -e "$usage"; exit 1;;
esac
