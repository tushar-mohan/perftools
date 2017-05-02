#!/bin/bash -e

tool="papiex"
tool_args=""

echo "$TEST: testing papiex with Unix syscalls and C library functions"
for exe in abort assert close "exit" _exit dlopen puts sigint system; do
  comment="$exe()"
  echo "  Testing: $comment"
  rm -rf $exe*$tool*
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
  checkfile "${exe}.${tool}.${host}.[0-9]*.1.txt" "  found output file"
  checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
done

for exe in fork; do
  comment="$exe()"
  echo "  Testing: $comment"
  rm -rf $exe*$tool*
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1.txt" 2 "  found a file for each process"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1.report.txt" 2 "  found a report file for each process"
done

for exe in forkexecl forkexeclp; do
  comment="$exe()"
  echo "  Testing: $comment"
  rm -rf $exe*$tool* basic*$tool*
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1.txt" 2 "  found a file for each process"
  countfiles "${exe}.${tool}.${host}.[0-9]*.1.report.txt" 2 "  found a report file for each process"
  checkfile "basic.${tool}.${host}.[0-9]*.1.txt" "  found output file"
  checkfile "basic.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
done
