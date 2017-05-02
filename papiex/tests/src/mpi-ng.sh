#!/bin/bash -e

executables="hostname mpi_hello mpi_wave mpi_pi mpi_exit"
declare -A comments=( \
  [hostname]="test non-mpi program under mpirun" \
  [mpi_hello]="test init/finalize" \
  [mpi_wave]="test send/recv" \
  [mpi_pi]="test collectives" \
  [mpi_exit]="test exit with mpi" \
)
comment="checks papiex with increasingly complex MPI programs"
tool="papiex"
tool_args="-a"

echo "$TEST: $comment"
type -P $mpi_launch > /dev/null
if [ $? -ne 0 ]; then
    warn "$mpi_launch not found"
    false
else
for exe in $executables; do
  echo "   Testing: $exe (${comments[$exe]})"
  rm -rf $exe.*$tool*
  if [ $DEBUG -ne 0 ]; then
      echo "  Testing: $mpi_launch $mpi_launch_args $tool $tool_args $exe"
  fi
  $mpi_launch $mpi_launch_args $tool $tool_args $exe >/dev/null 2>&1
# We really need to be capturing the output somehow for debugging
  if [ $? -eq 0 ]; then
      case $exe in
	  mpi*)
              checkdir "${exe}.${tool}.${host}.[0-9]*.1" "  found output directory"
              checkfile "${exe}.${tool}.${host}.[0-9]*.1/task_0.report.txt"
              checkfile "${exe}.${tool}.${host}.[0-9]*.1/task_1.report.txt"
              checkfile "${exe}.${tool}.${host}.[0-9]*.1/task_2.report.txt"
              checkfile "${exe}.${tool}.${host}.[0-9]*.1/task_3.report.txt"
              checkfile "${exe}.${tool}.${host}.[0-9]*.1.report.txt" "  found task summary file"
              findpattern "Num. of tasks\s+: 4" "${exe}.${tool}.${host}.[0-9]*.1.report.txt"  "  report aggregates metrics across tasks ok"
              ;;
	  *)
              countfiles "${exe}.${tool}.${host}.[0-9]*.1.txt" 4 "  found a task file for each process"
              countfiles "${exe}.${tool}.${host}.[0-9]*.1.report.txt" 4 "  found a report file for each process"
              ;;
      esac
  else
    warn "Failed: $mpi_launch $mpi_launch_args $tool $tool_args $exe $exe_args"
    false
  fi
done
fi
