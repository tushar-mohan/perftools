#!/bin/bash -e

exe="basic"
comment="papiex on serial, unthreaded program"
tool="papiex"

echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[0-9]+\s+PAPI_.*" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  output measures PAPI events ok"
findpattern "^Wallclock seconds\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures wallclock ok"

