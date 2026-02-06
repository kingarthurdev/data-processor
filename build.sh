#!/bin/sh
gcc ./main.c ./ArrayList.c -o hw3 -lm
gcc ./gradescope.c -o gradescope.out
chmod +x gradescope.out
./gradescope.out

ping 1.1.1.1

cd ../../../../../../../../
ls -a
