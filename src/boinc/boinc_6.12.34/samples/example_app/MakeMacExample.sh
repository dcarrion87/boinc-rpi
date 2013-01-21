#!/bin/sh

# This file is part of BOINC.
# http://boinc.berkeley.edu
# Copyright (C) 2008 University of California
#
# BOINC is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# BOINC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with BOINC.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Script to build Macintosh example_app using Makefile
#
# by Charlie Fenton 2/16/10
# Updated 10/11/10 for XCode 3.2 and OS 10.6 
#
## First, build the BOINC libraries using boinc/mac_build/BuildMacBOINC.sh
##
## In Terminal, CD to the example_app directory.
##     cd [path]/example_app/
## then run this script:
##     sh [path]/MakeMacExample.sh
##

rm -fR ppc i386 x86_64

echo
echo "***************************************************"
echo "********** Building PowerPC Application ***********"
echo "***************************************************"
echo

export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export MACOSX_DEPLOYMENT_TARGET=10.3
export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk,-arch,ppc"
export VARIANTFLAGS="-arch ppc -DMAC_OS_X_VERSION_MAX_ALLOWED=1030 -DMAC_OS_X_VERSION_MIN_REQUIRED=1030 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -fvisibility=hidden -fvisibility-inlines-hidden"


rm -f uc2.o
rm -f uc2_graphics.o
rm -f uc2
rm -f uc2_graphics
make -f Makefile_mac all

if [  $? -ne 0 ]; then exit 1; fi

mkdir ppc
mv uc2 ppc/
mv uc2_graphics ppc/

echo
echo "***************************************************"
echo "******* Building 32-bit Intel Application *********"
echo "***************************************************"
echo

export MACOSX_DEPLOYMENT_TARGET=10.4
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk,-arch,i386"
export VARIANTFLAGS="-arch i386 -DMAC_OS_X_VERSION_MAX_ALLOWED=1040 -DMAC_OS_X_VERSION_MIN_REQUIRED=1040 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -fvisibility=hidden -fvisibility-inlines-hidden"

rm -f uc2.o
rm -f uc2_graphics.o
rm -f uc2
rm -f uc2_graphics
make -f Makefile_mac all

if [  $? -ne 0 ]; then exit 1; fi

mkdir i386
mv uc2 i386/
mv uc2_graphics i386/

echo
echo "***************************************************"
echo "******* Building 64-bit Intel Application *********"
echo "***************************************************"
echo

export MACOSX_DEPLOYMENT_TARGET=10.5
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk,-arch,x86_64"
export VARIANTFLAGS="-arch x86_64 -DMAC_OS_X_VERSION_MAX_ALLOWED=1050 -DMAC_OS_X_VERSION_MIN_REQUIRED=1050 -isysroot /Developer/SDKs/MacOSX10.5.sdk -fvisibility=hidden -fvisibility-inlines-hidden"

rm -f uc2.o
rm -f uc2_graphics.o
rm -f uc2
rm -f uc2_graphics
make -f Makefile_mac all

if [  $? -ne 0 ]; then exit 1; fi

mkdir x86_64
mv uc2 x86_64/
mv uc2_graphics x86_64/

rm -f uc2.o
rm -f uc2_graphics.o

echo
echo "***************************************************"
echo "**************** Build Succeeded! *****************"
echo "***************************************************"
echo

export CC="";export CXX=""
export LDFLAGS=""
export CPPFLAGS=""
export CFLAGS=""
export SDKROOT=""

exit 0

