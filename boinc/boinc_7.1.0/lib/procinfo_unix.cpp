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


// process-enumeration stuff for Unix other than Mac OS X

#include "config.h"

#if HAVE_PROCFS_H
// Can't use large file calls with solaris procfs.
#if defined(_FILE_OFFSET_BITS) && ( _FILE_OFFSET_BITS == 64 )
#undef _FILE_OFFSET_BITS
#undef _LARGE_FILES
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE
#endif
#endif

#include <cstdio>
#include <string.h>
#include <sys/param.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_PROCFS_H
#include <procfs.h>  // definitions for solaris /proc structs
#endif

#include "str_util.h"
#include "str_replace.h"

#include "procinfo.h"

using std::vector;

// see:
// man 5 proc
// /usr/src/linux/fs/proc/array.C

struct PROC_STAT {
    int pid;
    char comm[256];
    char state;
    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;
    unsigned long flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;
    unsigned long utime;
    unsigned long stime;
    int cutime;
    int cstime;
    int priority;
    int nice;
    int zero;
    int itrealvalue;
    unsigned long starttime;
    unsigned long vsize;
    int rss;
    unsigned long rlim;
    unsigned long startcode;
    unsigned long endcode;
    unsigned long startstack;
    unsigned long kstkesp;
    unsigned long kstkeip;
    unsigned long signal;
    unsigned long blocked;
    unsigned long sigignore;
    unsigned long sigcatch;
    unsigned long wchan;
    unsigned long nswap;
    unsigned long cnswap;
    int exit_signal;
    int processor;

    int parse(char*);
};

int PROC_STAT::parse(char* buf) {
    int n = sscanf(buf,
        "%d (%s %c %d %d %d %d %d "
        "%lu %lu %lu %lu %lu %lu %lu "
        "%d %d %d %d %d %d "
        "%lu %lu "
        "%d "
        "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
        "%d %d",
        &pid,
        comm,
        &state,
        &ppid,
        &pgrp,
        &session,
        &tty_nr,
        &tpgid,
        &flags,
        &minflt,
        &cminflt,
        &majflt,
        &cmajflt,
        &utime,
        &stime,
        &cutime,
        &cstime,
        &priority,
        &nice,
        &zero,
        &itrealvalue,
        &starttime,
        &vsize,
        &rss,
        &rlim,
        &startcode,
        &endcode,
        &startstack,
        &kstkesp,
        &kstkeip,
        &signal,
        &blocked,
        &sigignore,
        &sigcatch,
        &wchan,
        &nswap,
        &cnswap,
        &exit_signal,
        &processor
    );
    if (n == 39) {
        char* p = strchr(comm, ')');
        if (p) *p = 0;
        return 0;
    }

    // I don't see a good choice of ERR_ for this...
    //
    return 1;
}

// build table of all processes in system
//
int procinfo_setup(PROC_MAP& pm) {

#if HAVE_DIRENT_H
    DIR *dir;
    dirent *piddir;
    FILE* fd;
    PROC_STAT ps;
    PROCINFO p;
    char pidpath[MAXPATHLEN];
    char buf[1024];
    int pid = getpid();
    int retval, final_retval = 0;

    dir = opendir("/proc");
    if (!dir) return 0;

    while (1) {
        piddir = readdir(dir);
        if (!piddir) break;
        if (!isdigit(piddir->d_name[0])) continue;

#if defined(HAVE_PROCFS_H) && defined(HAVE__PROC_SELF_PSINFO)  // solaris
        psinfo_t psinfo;
        sprintf(pidpath, "/proc/%s/psinfo", piddir->d_name);
        fd = fopen(pidpath, "r");
        if (fd) {
            p.clear();
            if (fread(&psinfo, sizeof(psinfo_t), 1, fd) == 1) {
                p.id = psinfo.pr_pid;
                p.parentid = psinfo.pr_ppid;
                p.swap_size = psinfo.pr_size*1024.;
                p.working_set_size = psinfo.pr_rssize * 1024.;
                strlcpy(p.command, psinfo.pr_fname, sizeof(p.command));
            }
            fclose(fd);
            sprintf(pidpath, "/proc/%s/usage", piddir->d_name);
            prusage_t prusage;
            fd = fopen(pidpath, "r");
            if (fd) {
                if (fread(&prusage, sizeof(prusage_t), 1, fd) == 1) {
                    p.user_time = (float)prusage.pr_utime.tv_sec +
                        ((float)prusage.pr_utime.tv_nsec)/1e+9;
                    p.kernel_time = (float)prusage.pr_stime.tv_sec +
                        ((float)prusage.pr_utime.tv_nsec)/1e+9;
                    // page faults: I/O + non I/O
                    p.page_fault_count = prusage.pr_majf + prusage.pr_minf;
                }
                fclose(fd);
                p.is_boinc_app = (p.id == pid || strcasestr(p.command, "boinc"));
                pm.insert(std::pair(p.id, p));
            }
        }
#else  // linux
        sprintf(pidpath, "/proc/%s/stat", piddir->d_name);
        fd = fopen(pidpath, "r");
        if (fd) {
            fgets(buf, sizeof(buf), fd);
            retval = ps.parse(buf);
            fclose(fd);

            if (retval) {
                final_retval = retval;
            } else {
                p.clear();
                p.id = ps.pid;
                p.parentid = ps.ppid;
                p.swap_size = ps.vsize;
                // rss = pages, need bytes
                // assumes page size = 4k
                p.working_set_size = ps.rss * (float)getpagesize();
                // page faults: I/O + non I/O
                p.page_fault_count = ps.majflt + ps.minflt;
                // times are in jiffies, need seconds
                // assumes 100 jiffies per second
                p.user_time = ps.utime / 100.;
                p.kernel_time = ps.stime / 100.;
                strlcpy(p.command, ps.comm, sizeof(p.command));
                p.is_boinc_app = (p.id == pid || strcasestr(p.command, "boinc"));
                p.is_low_priority = (ps.priority == 39);
                    // Linux seems to add 20 here,
                    // but this isn't documented anywhere
                pm.insert(std::pair<int, PROCINFO>(p.id, p));
            }
        }
#endif
    }
    closedir(dir);
#endif
    find_children(pm);
    return final_retval;

}
