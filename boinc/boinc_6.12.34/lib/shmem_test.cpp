// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// test program for shmem functions

// -a       attach and sleep
// -d       destroy
// -c       create

#define KEY 0xbeefcafe

#include "config.h"
#include <cstring>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "shmem.h"

int main(int argc, char** argv) {
    void* p;
    int retval;

    if (!strcmp(argv[1], "-a")) {
        retval = attach_shmem(KEY, &p);
        sleep(60);
    } else if (!strcmp(argv[1], "-d")) {
        destroy_shmem(KEY);
    } else if (!strcmp(argv[1], "-c")) {
        create_shmem(KEY, 100, &p);
    }

    return 0;
}

const char *BOINC_RCSID_6911713ff8 = "$Id: shmem_test.cpp 16069 2008-09-26 18:20:24Z davea $";
