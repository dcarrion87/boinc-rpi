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

// crash and burn
#include "config.h"
#include <cstdio>
#include <cctype>

int main() {
  char * hello = (char *) 100;
  int c, n=0;
  fprintf(stderr, "APP: upper_case starting\n");
  printf("%s",hello);
    while (1) {
        c = getchar();
        if (c == EOF) break;
        c = toupper(c);
        putchar(c);
        n++;
    }
    fprintf(stderr, "APP: upper_case ending, wrote %d chars\n", n);
}

const char *BOINC_RCSID_130fd0309d = "$Id$";
