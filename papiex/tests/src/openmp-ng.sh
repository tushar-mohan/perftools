#!/bin/bash -e

exe="omp"
comment="checks papiex with OpenMP"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
rm -rf $exe.*$tool*
[ $DEBUG -eq 0 ] || echo "  Testing: $tool $tool_args ./$exe"
OMP_NUM_THREADS=4 $tool $tool_args ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || warn "could not execute: $tool ./$exe"
checkdir "${exe}.${tool}.${host}.[0-9]*.1" "  found output directory"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_0.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_1.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_2.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_3.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_0.report.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_1.report.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_2.report.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1/thread_3.report.txt"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found thread summary file"
findpattern "4 threads" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report aggregates metrics ok"

