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

// -c       create semaphore
// -d       destroy semaphore
// -l       lock semaphore, sleep 10 secs, unlock

#include "config.h"
#include <unistd.h>

#include "synch.h"

#define KEY 0xdeadbeef

int main(int argc, char** argv) {
    if (!strcmp(argv[1], "-c")) {
        create_semaphore(KEY);
    } else if (!strcmp(argv[1], "-d")) {
        destroy_semaphore(KEY);
    } else if (!strcmp(argv[1], "-l")) {
        lock_semaphore(KEY);
        sleep(10);
        unlock_semaphore(KEY);
    }

    return 0;
}

const char *BOINC_RCSID_7eba26a197 = "$Id: synch_test.cpp 16069 2008-09-26 18:20:24Z davea $";
