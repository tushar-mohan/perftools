#!/bin/bash -e

exe="dd"
exe_args="if=/dev/zero of=zero bs=1024k count=10"
comment="papiex I/O profiling on dd"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool $tool_args -- $exe $exe_args"
$tool $tool_args -- $exe $exe_args > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool $tool_args -- $exe $exe_args"
rm -f zero
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[1-9][0-9]+\s+IO cycles" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  output measures IO cycles ok"
findpattern "^IO Cycles %\s+\.+\s+${SCI_NUM_GREATHER_THAN_TEN}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures IO Cycles ok"

