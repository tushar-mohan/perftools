#!/bin/bash -e

exe="basic"
tool="hpcex"
comment="checks hpcex/hpcstruct/hpcproftt on a simple program"
tool_args=""

echo "$TEST: $comment"
rm -rf $exe.*$tool* *.hpcstruct *.hpcproftt.txt

skip_test=0
for t in $tool hpcstruct hpcproftt; do
  if  ! which $t >/dev/null; then
    echo "Could not find $t. It seems hpctoolkit is not installed." >&2
    echo "Skipping $TEST"; skip_test=1
    break
  fi
done

if [ $skip_test -eq 0 ]; then
  [ $DEBUG -eq 0 ] || echo "  $tool $tool_args ./$exe"
  $tool $tool_args ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: $tool ./$exe"
  checkfile "${exe}.${tool}.${host}*.[0-9]*.0x0"
  hpcstruct ./$exe > /dev/null 2>&1
  [ $? -eq 0 ] || die "could not execute: hpcstruct ./$exe"
  checkfile "${exe}.hpcstruct"
  HPCPROFTT_OUT="${exe}.hpcproftt.txt"
  hpcproftt --src=all ${exe}.${tool}.${host}*.[0-9]*.0x0 > $HPCPROFTT_OUT
  [ $? -eq 0 ] || die "could not execute: hpcproftt --src=all ${exe}.${tool}.${host}*.[0-9]*.0x0"
  checkfile "$HPCPROFTT_OUT"
  findpattern "\s+[0-9.]+%.*${exe}.c" "$HPCPROFTT_OUT"  "  hpcproftt file summary ok"
  findpattern "\s+[0-9.]+%.*${exe}.*main" "$HPCPROFTT_OUT"  "  hpcproftt procedure summary ok"
  findpattern "\s+[0-9.]+%.*${exe}.c.*14" "$HPCPROFTT_OUT"  "  hpcproftt statement summary ok"
fi
