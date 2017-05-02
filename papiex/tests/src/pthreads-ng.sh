#!/bin/bash -e

exe="pthreads"
comment="checks papiex with POSIX threads"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
#for exe in pthreads badpthreads badpthreads2; do
for exe in pthreads; do
  rm -rf $exe.*$tool*
  echo "  Testing $exe"
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
  checkdir "${exe}.${tool}.${host}.[0-9]*.1" "  found output directory"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1/thread_[0-9].txt" 3 "  found a file for each thread"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1/thread_[0-9].report.txt" 3 "  found a report file for each thread"
  checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found thread summary file"
  findpattern "3 threads" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report aggregates metrics ok"
done 

