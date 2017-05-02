#!/bin/bash -e

exe="basic"
comment="checks multiplexing PAPI events in papiex"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
$tool $tool_args ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[0-9]+\s+PAPI_.*" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  output measures PAPI events ok"
findpattern "^Instructions Per Dcache Miss\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures derived metrics ok"

