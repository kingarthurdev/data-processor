#!/bin/sh
gcc ./modifiedSimulator.c ./instructionFunctions.c -o hw5-sim
gcc ./main.c ./ArrayList.c -o hw5-asm
