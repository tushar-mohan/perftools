#!/bin/bash -e

exe="basic"
comment="checks memory stats with papiex"
tool="papiex"
tool_args="-x"

echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
$tool $tool_args ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[0-9]+\s+.* Mem. resident peak KB" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  measures resident memory size ok"
findpattern "[0-9]+\s+.* Mem. heap KB" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  measures heap size ok"
findpattern "[0-9]+\s+.* Mem. stack KB" "${exe}.${tool}.${host}.[0-9]*.1.txt" "  measures stack size ok"
findpattern "^Mem. text KB\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"

