#!/bin/bash

cd lib
echo "* Compiling supporting objects"
g++ -c *.cpp *.c
echo "* Arching objects to libboinc.a"
ar -rc libboinc.a *.o
cd ..
echo "* Compiling data collector"
g++ -I lib/ -I /usr/include/openssl data_collect.cpp pugixml.cpp -pthread lib/libboinc.a -o data_collect
echo "* Done!"
