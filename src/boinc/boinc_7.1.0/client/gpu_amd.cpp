// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2012 University of California
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

// Detection of AMD/ATI GPUs
//
// Docs:
// http://developer.amd.com/gpu/ATIStreamSDK/assets/ATI_Stream_SDK_CAL_Programming_Guide_v2.0%5B1%5D.pdf
// ?? why don't they have HTML docs??

#ifdef _WIN32
#include "boinc_win.h"
#else
#ifdef __APPLE__
// Suppress obsolete warning when building for OS 10.3.9
#define DLOPEN_NO_WARN
#include <mach-o/dyld.h>
#endif
#include "config.h"
#include <dlfcn.h>
#endif

#include <vector>
#include <string>

using std::vector;
using std::string;

#include "coproc.h"
#include "util.h"

#include "client_msgs.h"
#include "gpu_detect.h"

// criteria:
//
// - double precision support
// - local RAM
// - speed
//
int ati_compare(COPROC_ATI& c1, COPROC_ATI& c2, bool loose) {
    if (c1.attribs.doublePrecision && !c2.attribs.doublePrecision) return 1;
    if (!c1.attribs.doublePrecision && c2.attribs.doublePrecision) return -1;
    if (loose) {
        if (c1.attribs.localRAM> 1.4*c2.attribs.localRAM) return 1;
        if (c1.attribs.localRAM< .7* c2.attribs.localRAM) return -1;
        return 0;
    }
    if (c1.attribs.localRAM > c2.attribs.localRAM) return 1;
    if (c1.attribs.localRAM < c2.attribs.localRAM) return -1;
    double s1 = c1.peak_flops;
    double s2 = c2.peak_flops;
    if (s1 > s2) return 1;
    if (s1 < s2) return -1;
    return 0;
}

#ifdef _WIN32
typedef int (__stdcall *ATI_ATTRIBS) (CALdeviceattribs *attribs, CALuint ordinal);
typedef int (__stdcall *ATI_CLOSE)(void);
typedef int (__stdcall *ATI_GDC)(CALuint *numDevices);
typedef int (__stdcall *ATI_GDI)(void);
typedef int (__stdcall *ATI_INFO) (CALdeviceinfo *info, CALuint ordinal);
typedef int (__stdcall *ATI_VER) (CALuint *cal_major, CALuint *cal_minor, CALuint *cal_imp);
typedef int (__stdcall *ATI_STATUS) (CALdevicestatus*, CALdevice);
typedef int (__stdcall *ATI_DEVICEOPEN) (CALdevice*, CALuint);
typedef int (__stdcall *ATI_DEVICECLOSE) (CALdevice);

ATI_ATTRIBS __calDeviceGetAttribs = NULL;
ATI_CLOSE   __calShutdown = NULL;
ATI_GDC     __calDeviceGetCount = NULL;
ATI_GDI     __calInit = NULL;
ATI_INFO    __calDeviceGetInfo = NULL;
ATI_VER     __calGetVersion = NULL;
ATI_STATUS  __calDeviceGetStatus = NULL;
ATI_DEVICEOPEN  __calDeviceOpen = NULL;
ATI_DEVICECLOSE  __calDeviceClose = NULL;

#else

int (*__calInit)();
int (*__calGetVersion)(CALuint*, CALuint*, CALuint*);
int (*__calDeviceGetCount)(CALuint*);
int (*__calDeviceGetAttribs)(CALdeviceattribs*, CALuint);
int (*__calShutdown)();
int (*__calDeviceGetInfo)(CALdeviceinfo*, CALuint);
int (*__calDeviceGetStatus)(CALdevicestatus*, CALdevice);
int (*__calDeviceOpen)(CALdevice*, CALuint);
int (*__calDeviceClose)(CALdevice);

#endif

void COPROC_ATI::get(
    bool use_all,
    vector<string>& warnings,
    vector<int>& ignore_devs
) {
    CALuint numDevices, cal_major, cal_minor, cal_imp;
    char buf[256];
    int retval;

    attribs.struct_size = sizeof(CALdeviceattribs);
    numDevices =0;

#ifdef _WIN32

#if defined _M_X64
    const char* atilib_name = "aticalrt64.dll";
    const char* amdlib_name = "amdcalrt64.dll";
#else
    const char* atilib_name = "aticalrt.dll";
    const char* amdlib_name = "amdcalrt.dll";
#endif

    HINSTANCE callib = LoadLibrary(atilib_name);
    if (callib) {
        atirt_detected = true;
    } else {
        callib = LoadLibrary(amdlib_name);
        if (callib) {
            amdrt_detected = true;
        }
    }

    if (!callib) {
        warnings.push_back("No ATI library found.");
        return;
    }

    __calInit = (ATI_GDI)GetProcAddress(callib, "calInit" );
    __calGetVersion = (ATI_VER)GetProcAddress(callib, "calGetVersion" );
    __calDeviceGetCount = (ATI_GDC)GetProcAddress(callib, "calDeviceGetCount" );
    __calDeviceGetAttribs =(ATI_ATTRIBS)GetProcAddress(callib, "calDeviceGetAttribs" );
    __calShutdown = (ATI_CLOSE)GetProcAddress(callib, "calShutdown" );
    __calDeviceGetInfo = (ATI_INFO)GetProcAddress(callib, "calDeviceGetInfo" );
    __calDeviceGetStatus = (ATI_STATUS)GetProcAddress(callib, "calDeviceGetStatus" );
    __calDeviceOpen = (ATI_DEVICEOPEN)GetProcAddress(callib, "calDeviceOpen" );
    __calDeviceClose = (ATI_DEVICECLOSE)GetProcAddress(callib, "calDeviceClose" );

#else

    void* callib;

    callib = dlopen("libaticalrt.so", RTLD_NOW);
    if (!callib) {
        warnings.push_back("No ATI library found");
        return;
    }

    atirt_detected = true;

    __calInit = (int(*)()) dlsym(callib, "calInit");
    __calGetVersion = (int(*)(CALuint*, CALuint*, CALuint*)) dlsym(callib, "calGetVersion");
    __calDeviceGetCount = (int(*)(CALuint*)) dlsym(callib, "calDeviceGetCount");
    __calDeviceGetAttribs = (int(*)(CALdeviceattribs*, CALuint)) dlsym(callib, "calDeviceGetAttribs");
    __calShutdown = (int(*)()) dlsym(callib, "calShutdown");
    __calDeviceGetInfo = (int(*)(CALdeviceinfo*, CALuint)) dlsym(callib, "calDeviceGetInfo");
    __calDeviceGetStatus = (int(*)(CALdevicestatus*, CALdevice)) dlsym(callib, "calDeviceGetStatus");
    __calDeviceOpen = (int(*)(CALdevice*, CALuint)) dlsym(callib, "calDeviceOpen");
    __calDeviceClose = (int(*)(CALdevice)) dlsym(callib, "calDeviceClose");

#endif

    if (!__calInit) {
        warnings.push_back("calInit() missing from CAL library");
        return;
    }
    if (!__calGetVersion) {
        warnings.push_back("calGetVersion() missing from CAL library");
        return;
    }
    if (!__calDeviceGetCount) {
        warnings.push_back("calDeviceGetCount() missing from CAL library");
        return;
    }
    if (!__calDeviceGetAttribs) {
        warnings.push_back("calDeviceGetAttribs() missing from CAL library");
        return;
    }
    if (!__calDeviceGetInfo) {
        warnings.push_back("calDeviceGetInfo() missing from CAL library");
        return;
    }
    if (!__calDeviceGetStatus) {
        warnings.push_back("calDeviceGetStatus() missing from CAL library");
        return;
    }
    if (!__calDeviceOpen) {
        warnings.push_back("calDeviceOpen() missing from CAL library");
        return;
    }
    if (!__calDeviceClose) {
        warnings.push_back("calDeviceClose() missing from CAL library");
        return;
    }

    retval = (*__calInit)();
    if (retval != CAL_RESULT_OK) {
        sprintf(buf, "calInit() returned %d", retval);
        warnings.push_back(buf);
        return;
    }
    retval = (*__calDeviceGetCount)(&numDevices);
    if (retval != CAL_RESULT_OK) {
        sprintf(buf, "calDeviceGetCount() returned %d", retval);
        warnings.push_back(buf);
        return;
    }
    retval = (*__calGetVersion)(&cal_major, &cal_minor, &cal_imp);
    if (retval != CAL_RESULT_OK) {
        sprintf(buf, "calGetVersion() returned %d", retval);
        warnings.push_back(buf);
        return;
    }

    if (!numDevices) {
        warnings.push_back("No usable CAL devices found");
        return;
    }

    COPROC_ATI cc, cc2;
    string s, gpu_name;
    for (CALuint i=0; i<numDevices; i++) {
        retval = (*__calDeviceGetInfo)(&info, i);
        if (retval != CAL_RESULT_OK) {
            sprintf(buf, "calDeviceGetInfo() returned %d", retval);
            warnings.push_back(buf);
            return;
        }
        retval = (*__calDeviceGetAttribs)(&attribs, i);
        if (retval != CAL_RESULT_OK) {
            sprintf(buf, "calDeviceGetAttribs() returned %d", retval);
            warnings.push_back(buf);
            return;
        }
        switch ((int)attribs.target) {
        case CAL_TARGET_600:
            gpu_name="ATI Radeon HD 2900 (RV600)";
            break;
        case CAL_TARGET_610:
            gpu_name="ATI Radeon HD 2300/2400/3200 (RV610)";
            attribs.numberOfSIMD=1;        // set correct values (reported wrong by driver)
            attribs.wavefrontSize=32;
            break;
        case CAL_TARGET_630:
            gpu_name="ATI Radeon HD 2600 (RV630)";
            // set correct values (reported wrong by driver)
            attribs.numberOfSIMD=3;
            attribs.wavefrontSize=32;
            break;
        case CAL_TARGET_670:
            gpu_name="ATI Radeon HD 3800 (RV670)";
            break;
        case CAL_TARGET_710:
            gpu_name="ATI Radeon HD 4350/4550 (R710)";
            break;
        case CAL_TARGET_730:
            gpu_name="ATI Radeon HD 4600 series (R730)";
            break;
        case CAL_TARGET_7XX:
            gpu_name="ATI Radeon (RV700 class)";
            break;
        case CAL_TARGET_770:
            gpu_name="ATI Radeon HD 4700/4800 (RV740/RV770)";
            break;
        case 8:
            gpu_name="ATI Radeon HD 5800 series (Cypress)";
            break;
        case 9:
            gpu_name="ATI Radeon HD 5700 series (Juniper)";
            break;
        case 10:
            gpu_name="ATI Radeon HD 5x00 series (Redwood)";
            break;
        case 11:
            gpu_name="ATI Radeon HD 5x00 series (Cedar)";
            break;
        case 12:
            gpu_name="AMD Radeon HD (unknown)";
            break;
        case 13:
            gpu_name="AMD Radeon HD 6x00 series (Sumo)";
            break;
        case 14:
            gpu_name="AMD Radeon HD 6200/6300/7300 series (Wrestler)";
            break;
        case 15:
            gpu_name="AMD Radeon HD 6900 series (Cayman)";
            break;
        case 16:
            gpu_name="AMD Radeon HD (unknown)";
            break;
        case 17:
            gpu_name="AMD Radeon HD 6800 series (Barts)";
            break;
        case 18:
            gpu_name="AMD Radeon HD 6x00 series (Turks)";
            break;
        case 19:
            gpu_name="AMD Radeon HD 6300 series (Caicos)";
            break;
        case 20:
            gpu_name="AMD Radeon HD 7900 series (Tahiti)";
            break;
        case 21:
            gpu_name="AMD Radeon HD 7800 series (Pitcairn)";
            break;
        case 22:
            gpu_name="AMD Radeon HD 7700 series (Cape Verde)";
            break;
        case 23:
            gpu_name="AMD Radeon HD 7400/7500/7600 serie (Trinity)";
            break;
        default:
            gpu_name="AMD Radeon HD (unknown)";
            break;
        }
        cc.have_cal = true;
        cc.attribs = attribs;
        cc.info = info;
        strcpy(cc.name, gpu_name.c_str());
        sprintf(cc.version, "%d.%d.%d", cal_major, cal_minor, cal_imp);
        cc.amdrt_detected = amdrt_detected;
        cc.atirt_detected = atirt_detected;
        cc.device_num = i;
        cc.set_peak_flops();
        cc.get_available_ram();
        ati_gpus.push_back(cc);
    }

    // shut down CAL, otherwise Lenovo won't be able to switch to low-power GPU
    //
    retval = (*__calShutdown)();

    if (!ati_gpus.size()) {
        warnings.push_back("No ATI GPUs found");
        return;
    }

    // find the most capable non-ignored instance
    //
    bool first = true;
    unsigned int i;
    for (i=0; i<ati_gpus.size(); i++) {
        if (in_vector(ati_gpus[i].device_num, ignore_devs)) continue;
        if (first) {
            *this = ati_gpus[i];
            first = false;
        } else if (ati_compare(ati_gpus[i], *this, false) > 0) {
            *this = ati_gpus[i];
        }
    }

    // see which other instances are equivalent,
    // and set the "count" and "device_nums" fields
    //
    count = 0;
    for (i=0; i<ati_gpus.size(); i++) {
        ati_gpus[i].description(buf);
        if (in_vector(ati_gpus[i].device_num, ignore_devs)) {
            ati_gpus[i].is_used = COPROC_IGNORED;
        } else if (use_all || !ati_compare(ati_gpus[i], *this, true)) {
            device_nums[count] = ati_gpus[i].device_num;
            count++;
            ati_gpus[i].is_used = COPROC_USED;
        } else {
            ati_gpus[i].is_used = COPROC_UNUSED;
        }
    }
}

// get available RAM of ATI GPU
//
void COPROC_ATI::get_available_ram() {
    CALdevicestatus st;
    CALdevice dev;
    int retval;

    available_ram = attribs.localRAM*MEGA;

    st.struct_size = sizeof(CALdevicestatus);

    retval = (*__calDeviceOpen)(&dev, device_num);
    if (retval) {
        if (log_flags.coproc_debug) {
            msg_printf(0, MSG_INFO,
                "[coproc] calDeviceOpen(%d) returned %d", device_num, retval
            );
        }
        return;
    }
    retval = (*__calDeviceGetStatus)(&st, dev);
    if (retval) {
        if (log_flags.coproc_debug) {
            msg_printf(0, MSG_INFO,
                "[coproc] calDeviceGetStatus(%d) returned %d",
                device_num, retval
            );
        }
        (*__calDeviceClose)(dev);
        return;
    }
    available_ram = st.availLocalRAM*MEGA;
    (*__calDeviceClose)(dev);
}
