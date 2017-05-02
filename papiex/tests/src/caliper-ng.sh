#!/bin/bash -e

exe="caliper_c"
comment="calipers in C programs"
tool="papiex"

echo "$TEST: $comment"
rm -rf $exe*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
#Wallclock usecs ..............................  2.66829e+05
#[printf] Measurements ...........................  1.00000e+00
#[printf] PAPI_TOT_CYC ...........................  8.54700e+03 [  0.0]
#[printf] PAPI_TOT_INS ...........................  5.56900e+03 [  0.0]
#[printf] Real cycles ............................  3.35100e+04 [  0.0]
#[for loop] Measurements ...........................  1.00000e+00
#[for loop] PAPI_TOT_CYC ...........................  9.02824e+08 [100.0]
#[for loop] PAPI_TOT_INS ...........................  9.00001e+08 [100.0]
#[for loop] Real cycles ............................  8.24681e+08 [100.0]
#
#Caliper Labels:
#  [1]   printf
#  [2]   for loop
findpattern "^Wallclock seconds\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures wallclock ok"
findpattern "^\[printf\] Measurements\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  caliper1 found"
findpattern "^\[for loop\] Measurements\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  caliper2 found"
findpattern "^\[printf\] Real cycles\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  counts in caliper1 found"
findpattern "^\[for loop\] Real cycles\s+\.+\s+${SCI_NUM}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  counts in caliper2 found"
findpattern "^\s+\[1\]\s+printf" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  label map for caliper1 found"
findpattern "^\s+\[2\]\s+for loop" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  label map for caliper2 found"

