#!/bin/bash -e

exe="mpi_wave"
exe_args=""
comment="papiex MPI profiling"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
type -P $mpi_launch > /dev/null
if [ $? -ne 0 ]; then
    warn "$mpi_launch not found"
    false
else
    rm -rf $exe*$tool*
    [ $DEBUG -eq 0 ] || echo "  Testing: $mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args"
    $mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args > /dev/null 2>&1
    [ $? -eq 0 ] || die "could not execute: $mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args"
    checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found summary file"
    findpattern "^MPI Cycles %\s+\.+\s+${SCI_NUM_GREATHER_THAN_TEN}" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report measures MPI Cycles ok"
fi
