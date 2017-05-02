#!/bin/sh

ran_ok=""
for scr in `ls papiex*.sh hpcex*.sh ioex*.sh mpipex*.sh 2>/dev/null`; do
  base=${scr%.sh}
  \rm -f  $base.out 2>/dev/null
  echo -n "Running $scr .. "
  ./$scr > $base.out 2>&1
  ret=$?
  if [ $ret -ne 0 ]; then
    echo -e '\E[48;31m'"\033[1mFAILED\033[0m" " ($ret)"
  else
    echo -e '\E[48;32m'"\033[1mOK\033[0m"
    ran_ok="${ran_ok} $base"
  fi
done

echo "Now verifying output for: $ran_ok"
./verify $* $ran_ok
retVal=$?

if [ $retVal -eq 0 ]; then
  echo "All tests PASSED"
else
  echo "$retVal tests FAILED"
fi

exit $retVal
