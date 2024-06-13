#!/bin/sh
set -e
for i in $(seq 1 $#)
do
  echo $i
  eval F$i=$(mktemp --suffix=.c)
  gcc -E "$(eval echo \$$i)" -o $(eval echo \$F$i)
  eval O$i=$(mktemp)
  ./cc1 $(eval echo \$F$i) -o $(eval echo \$O$i)
  rm "$(eval echo \$F$i)"
done
gcc -m32 -x assembler $(eval echo $(seq -f '$O%.0f' -s ' ' 1 $#)) -z noexecstack
rm $(eval echo $(seq -f '$O%.0f' -s ' ' 1 $#))
