// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2011 University of California
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

#ifndef _PROC_CONTROL_
#define _PROC_CONTROL_

#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

extern void get_descendants(int pid, std::vector<int>& pids);
extern bool any_process_exists(std::vector<int>& pids);
extern void kill_all(std::vector<int>& pids);
#ifdef _WIN32
extern void kill_descendants();
extern int suspend_or_resume_threads(DWORD pid, DWORD threadid, bool resume);
#else
extern void kill_descendants(int child_pid=0);
#endif
extern void suspend_or_resume_descendants(int pid, bool resume);
extern void suspend_or_resume_process(int pid, bool resume);

#endif
