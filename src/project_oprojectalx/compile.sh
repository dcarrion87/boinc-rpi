#!/bin/bash

cd lib
echo "* Compiling supporting objects"
g++ -c *.cpp *.c
echo "* Arching objects to libboinc.a"
ar -rc libboinc.a *.o
cd ..
echo "* Compiling alx"
g++ -I lib/ -I /usr/include/openssl alx.cpp -pthread lib/libboinc.a -o alx
echo "* Done!"
