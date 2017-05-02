#!/bin/sh
\rm *.hpcex.*
../hpcex ./papiex_thr_flops
hpcprof -e ./papiex_thr_flops *.hpcex.*
