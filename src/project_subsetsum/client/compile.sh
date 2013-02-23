#!/bin/bash

cd lib
echo "* Compiling supporting objects"
g++ -c *.cpp *.c
echo "* Archiving objects to libboinc.a"
ar -rc libboinc.a *.o
cd ..
echo "* Compiling SubsetSum"
g++ -D__STDC_LIMIT_MACROS -DVERBOSE -DFALSE_ONLY -DENABLE_CHECKPOINTING -D_BOINC_ -O3 -funroll-loops -ftree-vectorize -Wall -Ilib  subset_sum_main.cpp -o subset_sum lib/libboinc.a lib/libsss_common.a -lpthread
echo "* Done!"
