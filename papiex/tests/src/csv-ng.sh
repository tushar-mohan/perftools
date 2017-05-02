#!/bin/bash -e

exe="basic"
comment="papiex CSV output"
tool="papiex"
args="-a --csv"
echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool $args ./$exe"
$tool $args ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool $args ./$exe"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.csv" "  found csv file"
findpattern ",Executable," "${exe}.${tool}.${host}.[0-9]*.1.csv"  "  csv format appears ok"
