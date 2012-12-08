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

// This file defines a Fortran wrapper to the BOINC API.

// Define this symbol (here or in Makefile) if you want graphics functions
//#define GRAPHICS

#include "config.h"
#include "util.h"
#include "boinc_api.h"
#ifdef GRAPHICS
#include "graphics_api.h"
#endif
#include "boinc_zip.h"

// helper class that makes a C-string from a character array and length,
// automatically deleted on destruction.
// Fortran strings are passed as character array plus length
//
class STRING_FROM_FORTRAN {
    char* p;
public:
    STRING_FROM_FORTRAN(const char* s, int s_len) {
        p = new char[s_len + 1];
        memcpy(p, s, s_len);
        p[s_len] = 0;
    }
    ~STRING_FROM_FORTRAN() { delete [] p; }
    void strip_whitespace() {
        ::strip_whitespace(p);
    }
    const char* c_str() { return p; }
};

// remove terminating null and pad with blanks a la FORTRAN
//
static void string_to_fortran(char* p, int len) {
    for (int i=strlen(p); i<len; i++) {
        p[i] = ' ';
    }
}

extern "C" {

void boinc_init_() {
    boinc_init();
}

void boinc_finish_(int* status) {
    boinc_finish(*status);
}

#ifdef GRAPHICS
void boinc_init_graphics_(){
    boinc_init_graphics();
}

void boinc_finish_graphics_(){
    boinc_finish_graphics();
}
#endif

void boinc_is_standalone_(int* result) {
    *result = boinc_is_standalone();
}

void boincrf_(const char* s, char* t, int s_len, int t_len) {
    STRING_FROM_FORTRAN sff(s, s_len);
    sff.strip_whitespace();
    boinc_resolve_filename(sff.c_str(), t, t_len);
    string_to_fortran(t, t_len);
}

void boinc_parse_init_data_file_() {
    boinc_parse_init_data_file();
}

void boinc_write_init_data_file_() {
    boinc_write_init_data_file();
}

void boinc_time_to_checkpoint_(int* result) {
    *result = boinc_time_to_checkpoint();
}

void boinc_checkpoint_completed_() {
    boinc_checkpoint_completed();
}

void boinc_fraction_done_(double* d) {
    boinc_fraction_done(*d);
}

void boinc_wu_cpu_time_(double* d_out) {
    boinc_wu_cpu_time(*d_out);
}

void boinc_calling_thread_cpu_time_(double* d) {
    boinc_calling_thread_cpu_time(*d);
}

void boinc_zip_(int* zipmode, const char* zipfile,
    const char* path, int zipfile_len, int path_len
) {
    //zipmode = 0 to unzip or 1 to zip. FORTRAN variable of type INTEGER.
    //zipfile, path = FORTRAN variables of type CHARACTER.
    STRING_FROM_FORTRAN zipfileff(zipfile, zipfile_len);
    STRING_FROM_FORTRAN pathff(path, path_len);
    zipfileff.strip_whitespace();
    pathff.strip_whitespace();
    boinc_zip(*zipmode,zipfileff.c_str(),pathff.c_str());
} 

}   // extern "C"

const char *BOINC_RCSID_4f5153609c = "$Id: boinc_api_fortran.cpp 16069 2008-09-26 18:20:24Z davea $";
