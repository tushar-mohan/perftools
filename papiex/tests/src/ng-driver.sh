#!/bin/bash

# usage:
# [DEBUG=1] [QUICK=1] ./ng-driver [quick]

mpi_launch="mpirun"
mpi_launch_args="-np 4"

DEBUG=${DEBUG:-0}
QUICK=${QUICK:-0}
SCI_NUM="[0-9]\.[0-9e]+-?[0-9]+"
SCI_NUM_GREATER_THAN_TEN="[0-9]\.[0-9e]+[1-9]+"
NON_ZERO_NUM="[1-9][0-9]+"
host=`hostname -s`

failed=0
passed=0
num_tests=0

PATH="$PWD:$PATH"

function die() {
  echo $* >&2
  exit 1
}

function warn() {
  error_code=1
  echo $* >&2
}

function checkfile() {
  errmsg="Could not find $1"
  [ $DEBUG -eq 0 ] || echo Looking for $1
  [ -s $1 ] || warn $errmsg
  [ $DEBUG -eq 0 ] || [ "$2" == "" ] || echo "$2"
}

# checkfile_real "descriptions" "pattern or singleton"

function checkfile_real () {
    desc=$1
    shift
    while (( "$#" )); do
	[ $DEBUG -eq 0 ] || echo "$1"
	if [ ! -s $1 -o ! -r $1 ]; then
	    warn "$desc, $1 not found, not readable or not > size 0"
	fi
	shift
    done
    return
}

function countfiles() {
  errmsg="Could not find $2 files matching $1"
  [ $DEBUG -eq 0 ] || echo Looking for $2 files matching $1
  num_files=$(ls -1 $1 | wc -l)
  [ $num_files -eq $2 ] || warn $errmsg
  [ $DEBUG -eq 0 ] || [ "$3" == "" ] || echo "$3"
}


function checkdir() {
  errmsg="Could not find directory $1"
  [ $DEBUG -eq 0 ] || echo Looking for directory $1
  [ -d $1 ] || warn $errmsg
  [ $DEBUG -eq 0 ] || [ "$2" == "" ] || echo "$2"
}

function findpattern() {
  errmsg="Could not find pattern $1 in $2"
  [ $DEBUG -eq 0 ] || echo Looking for $1
  egrep "$1" $2 > /dev/null || warn $errmsg
  [ $DEBUG -eq 0 ] || [ "$3" == "" ] || echo "$3"
}

echo
echo "Running tests (for DEBUG output, do: export DEBUG=1)"..
for test in *-ng; do
  echo
  num_tests=`expr $num_tests + 1`
  TEST=$(basename $test)
  error_code=0
  . ./$test
  if [ $error_code -eq 0 ]; then
    status="PASSED"
    passed=`expr $passed + 1`
  else
    status="FAILED"
    failed=`expr $failed + 1`
  fi
  echo "$TEST: $status"

  # only run one test if quick set
  [ $QUICK -eq 0 ] || break
done

echo
[ $passed -eq 0 ] || echo "$passed of $num_tests PASSED"
[ $failed -eq 0 ] || echo "$failed of $num_tests FAILED"
