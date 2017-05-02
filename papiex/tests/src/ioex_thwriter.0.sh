#!/bin/sh
\rm -rf ioex_thwriter.ioex.* 2>/dev/null
../ioex ./ioex_thwriter 10
\rm -f zero zero_* >/dev/null
cat ioex_thwriter.ioex.*/* 
\rm -rf ioex_thwriter.ioex.* 2>/dev/null
