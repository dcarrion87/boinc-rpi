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

#ifndef __SCHED_HR__
#define __SCHED_HR__

extern bool already_sent_to_different_platform_quick(WORKUNIT&, APP&);

extern bool already_sent_to_different_platform_careful(
    WORKUNIT& workunit, APP&
);

extern bool hr_unknown_platform(HOST&);

// return the HR type to use for this app;
// app-specific HR type overrides global HR type
//
inline int app_hr_type(APP& app) {
    if (app.homogeneous_redundancy) {
        return app.homogeneous_redundancy;
    }
    return config.homogeneous_redundancy;
}

#endif
