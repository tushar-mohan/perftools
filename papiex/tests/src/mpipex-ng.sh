#!/bin/bash

exe="mpi_pi"
exe_args=""
comment="mpipex MPI profiling"
tool="mpipex"
tool_args=""
# Let's just test standard default
#tool_args="-k 1"
#mpip_env="-y -k 0 -t 0.01 -e -p -q"
#export MPIP="$mpip_env" 
# This test should assume mpiP is there and that batch sched works
#if [ ! -f "$MMPERFTOOLS_INSTALL/lib/libmpiP.so" ]; then
#  echo "Could not find libmpiP. Perhaps you haven't built mpiP."
#  echo "Skipping $TEST"
#else

echo "$TEST: $comment"
type -P $mpi_launch > /dev/null
if [ $? -ne 0 ]; then
    warn "$mpi_launch not found"
    false
else
rm -rf ${exe}.*.mpiP*
if [ $DEBUG -ne 0 ]; then
    echo "  Testing: $batch $mpi_launch_args $tool $tool_args $exe $exe_args"
fi
$mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args > /dev/null 2>&1
if [ $? -eq 0 ]; then
    checkfile_real "mpiP summary file" "${exe}.*.mpiP" 
    checkfile_real "mpiP CSV file" "${exe}.*.mpiP.csv" 
    #  1   0 mpi_wave.f           332 update_                  Send
	#  1   0 0x4009d8                 [unknown]                Reduce
    #findpattern "\s+[0-9]\s+0\s+mpi_wave.f\s+332\s+update_\s+Send$" "${exe}.4.[0-9]*.1.mpi*" "  report does site lookup ok"
    #Reduce                103      1.151    0.37  100.00        400
    findpattern ".*Reduce.*" "${exe}.4.[0-9]*.1.mpi*" "  report does site lookup ok"
else
    warn "Failed: $mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args"
    false
fi
fi
