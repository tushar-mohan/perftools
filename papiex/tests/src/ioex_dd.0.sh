#!/bin/sh
\rm dd.* 2>/dev/null
../ioex -- dd if=/dev/zero of=zero bs=1024k count=100
cat dd.*
\rm -f zero dd.* 2>/dev/null
