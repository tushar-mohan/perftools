#!/bin/sh
\rm -rf ioex_mpio.ioex.* 2>/dev/null
mpirun -np 4 ../ioex ./ioex_mpio 20
\rm -f zero >/dev/null
cat ioex_mpio.ioex.*/* 
\rm -rf ioex_mpio.ioex.* 2>/dev/null
