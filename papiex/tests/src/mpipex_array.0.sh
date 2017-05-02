#!/bin/sh
[ "`which mpirun 2>/dev/null`" != "" ] && MPIRUN="mpirun -np 4"
if [ "`which srun 2>/dev/null`" != "" ]; then
  part=`hostname | cut -f1 -d-`
  MPIRUN="srun -p $part -n 4"
fi
\rm *.mpipex.* 2>/dev/null
$MPIRUN ../mpipex ./mpipex_array 
cat *.mpi*.1
\rm -f *.mpipex.* 2>/dev/null
