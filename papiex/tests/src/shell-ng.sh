#!/bin/bash -e

bin="bash"
tool="papiex"

exe="bash_empty"
comment="test papiex on an empty bash script"

echo "$TEST: $comment"
rm -rf $bin*$tool*
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
checkfile "${bin}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "${bin}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[0-9]+\s+PAPI_.*" "${bin}.${tool}.${host}.[0-9]*.1.txt" "  output measures PAPI events ok"
findpattern "^Wallclock seconds\s+\.+\s+${SCI_NUM}" "${bin}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures wallclock ok"


exe="bash_fork_exec"
comment="bash script spawns other processes"
binaries="bash sleep"
echo "$TEST: $comment"
for bin in $binaries; do
    rm -rf $bin*$tool*
done
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
countfiles "bash.${tool}.${host}.[0-9]*.1.txt" 2 "  found a task file for each bash process"
countfiles "bash.${tool}.${host}.[0-9]*.1.report.txt" 2 "  found a report file for each bash process"
checkfile "sleep.${tool}.${host}.[0-9]*.1.txt" "  found output file"
checkfile "sleep.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
findpattern "[0-9]+\s+PAPI_.*" "sleep.${tool}.${host}.[0-9]*.1.txt" "  output measures PAPI events ok"
findpattern "^Wallclock seconds\s+\.+\s+${SCI_NUM}" "sleep.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures wallclock ok"

exe="badps"
comment="test papiex with ps"
binaries="bash ps basename"
echo "$TEST: $comment"
for bin in $binaries; do
    rm -rf $bin*$tool*
done
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
countfiles "ps.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for ps process"
countfiles "ps.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for ps process"
countfiles "basename.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for basename process"
countfiles "basename.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for basename process"
countfiles "bash.${tool}.${host}.[0-9]*.1.txt" 4 "  found a task file for each bash process"
countfiles "bash.${tool}.${host}.[0-9]*.1.report.txt" 4 "  found a report file for each bash process"
findpattern "[0-9]+\s+PAPI_.*" "ps.${tool}.${host}.[0-9]*.1.txt" "  output measures PAPI events ok"
findpattern "^Wallclock seconds\s+\.+\s+${SCI_NUM}" "ps.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures wallclock ok"

exe="testsh"
comment="test papiex sh shell script"
binaries="hostname dash sh bash sleep dd"
echo "$TEST: $comment"
for bin in $binaries; do
    rm -rf $bin*$tool*
done
[ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
$tool ./$exe > /dev/null 2>&1
[ $? -eq 0 ] || die "could not execute: $tool ./$exe"
countfiles "sleep.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for sleep process"
countfiles "sleep.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for sleep process"
countfiles "hostname.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for hostname process"
countfiles "hostname.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for hostname process"
countfiles "dd.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for dd process"
countfiles "dd.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for dd process"
countfiles "*sh.${tool}.${host}.[0-9]*.1.txt" 4 "  found a file for each *sh process"
countfiles "*sh.${tool}.${host}.[0-9]*.1.report.txt" 4 "  found a report file for each *sh process"

exe="testcsh"
comment="test papiex csh shell script"
binaries="hostname csh bsd-csh sleep dd"
echo "$TEST: $comment (SKIPPED: fix papiex to run with testcsh)"
# echo "$TEST: $comment"
# for bin in $binaries; do
#     rm -rf $bin*$tool*
# done
# [ $DEBUG -eq 0 ] || echo "  $tool ./$exe"
# $tool ./$exe > /dev/null 2>&1
# [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
# countfiles "sleep.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for sleep process"
# countfiles "sleep.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for sleep process"
# countfiles "hostname.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for hostname process"
# countfiles "hostname.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for hostname process"
# countfiles "dd.${tool}.${host}.[0-9]*.1.txt" 1 "  found a task file for dd process"
# countfiles "dd.${tool}.${host}.[0-9]*.1.report.txt" 1 "  found a report file for dd process"
# countfiles "*sh.${tool}.${host}.[0-9]*.1.txt" 4 "  found a file for each *sh process"
# countfiles "*sh.${tool}.${host}.[0-9]*.1.report.txt" 4 "  found a report file for each *sh process"


echo "$TEST: testing papiex with login shells"
for exe in sh csh; do
  comment="$exe"
  echo "  Testing: $comment (SKIPPED: please fix papiex to run with ${exe}login)"; continue
  echo "  Testing: $comment"
  rm -rf $exe*$tool*
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./"${exe}login" > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./${exe}login"
  checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
  checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
done
