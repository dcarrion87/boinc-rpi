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

// This program serves as both
// - An example BOINC application, illustrating the use of the BOINC API
// - A program for testing various features of BOINC
//
// NOTE: this file exists as both
// boinc/apps/upper_case.cpp
// and
// boinc_samples/example_app/uc2.cpp
// If you update one, please update the other!

// The program converts a mixed-case file to upper case:
// read "in", convert to upper case, write to "out"
//
// command line options
// -run_slow: sleep 1 second after each character
// -cpu_time N: use about N CPU seconds after copying files
// -early_exit: exit(10) after 30 chars
// -early_crash: crash after 30 chars
//

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstdio>
#include <cctype>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#endif

#include "str_util.h"
#include "util.h"
#include "filesys.h"
#include "boinc_api.h"
#include "mfile.h"
#include "graphics2.h"

#ifdef APP_GRAPHICS
#include "uc2.h"
UC_SHMEM* shmem;
#endif

using std::string;

#define CHECKPOINT_FILE "upper_case_state"
#define INPUT_FILENAME "in"
#define OUTPUT_FILENAME "out"

bool run_slow = false;
bool early_exit = false;
bool early_crash = false;
bool early_sleep = false;
double cpu_time = 20, comp_result;

// do a billion floating-point ops
// (note: I needed to add an arg to this;
// otherwise the MS C++ compiler optimizes away
// all but the first call to it!)
//
static double do_a_giga_flop(int foo) {
    double x = 3.14159*foo;
    int i;
    for (i=0; i<500000000; i++) {
        x += 5.12313123;
        x *= 0.5398394834;
    }
    return x;
}

int do_checkpoint(MFILE& mf, int nchars) {
    int retval;
    string resolved_name;

    FILE* f = fopen("temp", "w");
    if (!f) return 1;
    fprintf(f, "%d", nchars);
    fclose(f);

    retval = mf.flush();
    if (retval) return retval;
    boinc_resolve_filename_s(CHECKPOINT_FILE, resolved_name);
    retval = boinc_rename("temp", resolved_name.c_str());
    if (retval) return retval;

    return 0;
}

#ifdef APP_GRAPHICS
void update_shmem() {
    if (!shmem) return;

    // always do this; otherwise a graphics app will immediately
    // assume we're not alive
    shmem->update_time = dtime();

    // Check whether a graphics app is running,
    // and don't bother updating shmem if so.
    // This doesn't matter here,
    // but may be worth doing if updating shmem is expensive.
    //
    if (shmem->countdown > 0) {
        // the graphics app sets this to 5 every time it renders a frame
        shmem->countdown--;
    } else {
        return;
    }
    shmem->fraction_done = boinc_get_fraction_done();
    shmem->cpu_time = boinc_worker_thread_cpu_time();;
    boinc_get_status(&shmem->status);
}
#endif

int main(int argc, char **argv) {
    int i;
    int c, nchars = 0, retval, n;
    double fsize, fd;
    char input_path[512], output_path[512], chkpt_path[512], buf[256];
    MFILE out;
    FILE* state, *infile;

    for (i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-early_exit")) early_exit = true;
        if (!strcmp(argv[i], "-early_crash")) early_crash = true;
        if (!strcmp(argv[i], "-early_sleep")) early_sleep = true;
        if (!strcmp(argv[i], "-run_slow")) run_slow = true;
        if (!strcmp(argv[i], "-cpu_time")) {
            cpu_time = atof(argv[++i]);
        }
    }

    retval = boinc_init();
    if (retval) {
        fprintf(stderr, "%s boinc_init returned %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(retval);
    }

    // open the input file (resolve logical name first)
    //
    boinc_resolve_filename(INPUT_FILENAME, input_path, sizeof(input_path));
    infile = boinc_fopen(input_path, "r");
    if (!infile) {
        fprintf(stderr,
            "%s Couldn't find input file, resolved name %s.\n",
            boinc_msg_prefix(buf, sizeof(buf)), input_path
        );
        exit(-1);
    }

    // get size of input file (used to compute fraction done)
    //
    file_size(input_path, fsize);

    boinc_resolve_filename(OUTPUT_FILENAME, output_path, sizeof(output_path));

    // See if there's a valid checkpoint file.
    // If so seek input file and truncate output file
    //
    boinc_resolve_filename(CHECKPOINT_FILE, chkpt_path, sizeof(chkpt_path));
    state = boinc_fopen(chkpt_path, "r");
    if (state) {
        n = fscanf(state, "%d", &nchars);
        fclose(state);
    }
    if (state && n==1) {
        fseek(infile, nchars, SEEK_SET);
        boinc_truncate(output_path, nchars);
        retval = out.open(output_path, "ab");
    } else {
        retval = out.open(output_path, "wb");
    }
    if (retval) {
        fprintf(stderr, "%s APP: upper_case output open failed:\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
        fprintf(stderr, "%s resolved name %s, retval %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), output_path, retval
        );
        perror("open");
        exit(1);
    }

#ifdef APP_GRAPHICS
    // create shared mem segment for graphics, and arrange to update it
    //
    shmem = (UC_SHMEM*)boinc_graphics_make_shmem("uppercase", sizeof(UC_SHMEM));
    if (!shmem) {
        fprintf(stderr, "%s failed to create shared mem segment\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
    }
    update_shmem();
    boinc_register_timer_callback(update_shmem);
#endif

    // main loop - read characters, convert to UC, write
    //
    for (i=0; ; i++) {
        c = fgetc(infile);

        if (c == EOF) break;
        c = toupper(c);
        out._putchar(c);
        nchars++;
        if (run_slow) {
            boinc_sleep(1.);
        }

        if (early_exit && i>30) {
            exit(-10);
        }

        if (early_crash && i>30) {
            boinc_crash();
        }
        if (early_sleep && i>30) {
            g_sleep = true;
            while (1) boinc_sleep(1);
        }

        if (boinc_time_to_checkpoint()) {
            retval = do_checkpoint(out, nchars);
            if (retval) {
                fprintf(stderr, "%s APP: upper_case checkpoint failed %d\n",
                    boinc_msg_prefix(buf, sizeof(buf)), retval
                );
                exit(retval);
            }
            boinc_checkpoint_completed();
        }

        fd = nchars/fsize;
        if (cpu_time) fd /= 2;
        boinc_fraction_done(fd);
    }

    retval = out.flush();
    if (retval) {
        fprintf(stderr, "%s APP: upper_case flush failed %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(1);
    }

    // burn up some CPU time if needed
    //
    if (cpu_time) {
        double start = dtime();
        for (i=0; ; i++) {
            double e = dtime()-start;
            if (e > cpu_time) break;
            fd = .5 + .5*(e/cpu_time);
            boinc_fraction_done(fd);

            if (boinc_time_to_checkpoint()) {
                retval = do_checkpoint(out, nchars);
                if (retval) {
                    fprintf(stderr, "%s APP: upper_case checkpoint failed %d\n",
                        boinc_msg_prefix(buf, sizeof(buf)), retval
                    );
                    exit(1);
                }
                boinc_checkpoint_completed();
            }
            comp_result = do_a_giga_flop(i);
        }
    }
    boinc_fraction_done(1);
#ifdef APP_GRAPHICS
    update_shmem();
#endif
    boinc_finish(0);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR Args, int WinMode) {
    LPSTR command_line;
    char* argv[100];
    int argc;

    command_line = GetCommandLine();
    argc = parse_command_line( command_line, argv );
    return main(argc, argv);
}
#endif

const char *BOINC_RCSID_33ac47a071 = "$Id$";

