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
// Modify claimed credit based on the historical granted credit if
// the project is configured to do this
//

// functions related to the computation and granting of credit
// Note: this is credit.cpp rather than sched_credit.cpp
// because you might grant credit e.g. from a trickle handler

#include <math.h>

#include "boinc_db.h"
#include "error_numbers.h"

#include "sched_config.h"
#include "sched_msgs.h"
#include "sched_util.h"
#include "sched_shmem.h"
#include "sched_types.h"

#include "credit.h"

// TODO: delete
double fpops_to_credit(double fpops, double intops) {
    // TODO: use fp_weight if specified in config file
    double fpc = (fpops/1e9)*COBBLESTONE_FACTOR/SECONDS_PER_DAY;
    double intc = (intops/1e9)*COBBLESTONE_FACTOR/SECONDS_PER_DAY;
    return std::max(fpc, intc);
}

// Grant the host (and associated user and team)
// the given amount of credit for work that started at the given time.
// Update the user and team records,
// but not the host record (caller must update)
//
int grant_credit(
    DB_HOST& host, double start_time, double cpu_time, double credit
) {
    DB_USER user;
    DB_TEAM team;
    int retval;
    char buf[256];
    double now = dtime();

    // first, process the host

    update_average(
        now,
        start_time, credit, CREDIT_HALF_LIFE,
        host.expavg_credit, host.expavg_time
    );
    host.total_credit += credit;

    // then the user

    retval = user.lookup_id(host.userid);
    if (retval) {
        log_messages.printf(MSG_CRITICAL,
            "lookup of user %d failed %d\n",
            host.userid, retval
        );
        return retval;
    }

    update_average(
        now,
        start_time, credit, CREDIT_HALF_LIFE,
        user.expavg_credit, user.expavg_time
    );
    sprintf(
        buf, "total_credit=total_credit+%.15e, expavg_credit=%.15e, expavg_time=%.15e",
        credit,  user.expavg_credit, user.expavg_time
    );
    retval = user.update_field(buf);
    if (retval) {
        log_messages.printf(MSG_CRITICAL,
            "update of user %d failed %d\n",
             host.userid, retval
        );
    }

    // and finally the team

    if (user.teamid) {
        retval = team.lookup_id(user.teamid);
        if (retval) {
            log_messages.printf(MSG_CRITICAL,
                "lookup of team %d failed %d\n",
                user.teamid, retval
            );
            return retval;
        }
        update_average(
            now,
            start_time, credit, CREDIT_HALF_LIFE,
            team.expavg_credit, team.expavg_time
        );
        sprintf(buf,
            "total_credit=total_credit+%.15e, expavg_credit=%.15e, expavg_time=%.15e",
            credit,  team.expavg_credit, team.expavg_time
        );
        retval = team.update_field(buf);
        if (retval) {
            log_messages.printf(MSG_CRITICAL,
                "update of team %d failed %d\n",
                team.id, retval
            );
        }
    }
    return 0;
}

///////////////////// V2 CREDIT STUFF STARTS HERE ///////////////////

// levels of confidence in a credit value
//
#define PFC_MODE_NORMAL  0
    // PFC was computed in the "normal" way, i.e.
    // - claimed PFC
    // - app version scaling (i.e. not anonymous platform)
    // - host scaling
#define PFC_MODE_APPROX  1
    // PFC was approximated, but still (in the absence of cheating)
    // reflects the size of the particular job
#define PFC_MODE_WU_EST  2
    // PFC was set to the WU estimate.
    // If this doesn't reflect the WU size, neither does the PFC estimate 
    // This is a last resort, and can be way off.

// used in the computation of AV scale factors
//
struct RSC_INFO {
    double pfc_sum;
    double pfc_n;
    int nvers_thresh;   // # app versions w/ lots of samples
    int nvers_total;

    RSC_INFO() {
        pfc_sum = 0;
        pfc_n = 0;
        nvers_thresh = 0;
        nvers_total = 0;
    }
    void update(APP_VERSION& av) {
        nvers_total++;
        if (av.pfc.n > MIN_VERSION_SAMPLES) {
            nvers_thresh++;
            pfc_sum += av.pfc.get_avg() * av.pfc.n;
            pfc_n += av.pfc.n;
        }
    }
    double avg() {
        return pfc_sum/pfc_n;
    }
};

// "avg" is the average PFC for this app
// over CPU versions or GPU versions, whichever is lowest.
// Update the pfc_scale of this app's versions in the DB,
// and update app.min_avg_pfc
//
int scale_versions(APP& app, double avg, SCHED_SHMEM* ssp) {
    char buf[256];
    int retval;

    for (int j=0; j<ssp->napp_versions; j++) {
        APP_VERSION& av = ssp->app_versions[j];
        if (av.appid != app.id) continue;
        if (av.pfc.n < MIN_VERSION_SAMPLES) continue;
        av.pfc_scale= avg/av.pfc.get_avg();

        DB_APP_VERSION dav;
        dav.id = av.id;
        sprintf(buf, "pfc_scale=%.15e", av.pfc_scale);
        retval = dav.update_field(buf);
        if (retval) return retval;
        if (config.debug_credit) {
            PLATFORM* p = ssp->lookup_platform_id(av.platformid);
            log_messages.printf(MSG_NORMAL,
                " updating scale factor for %d (%s %s)\n",
                av.id, p->name, av.plan_class
            );
            log_messages.printf(MSG_NORMAL,
                "  n: %g avg PFC: %g new scale: %g\n",
                av.pfc.n, av.pfc.get_avg(), av.pfc_scale
            );
        }
    }
    app.min_avg_pfc = avg;
    DB_APP da;
    da.id = app.id;
    sprintf(buf, "min_avg_pfc=%.15e", avg);
    retval = da.update_field(buf);
    if (retval) return retval;
    return 0;
}

// Update app version scale factors,
// and find the min average PFC for each app.
// Called periodically from the master feeder.
//
int update_av_scales(SCHED_SHMEM* ssp) {
    int i, j, retval;
    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL, "-- updating app version scales --\n");
    }
    for (i=0; i<ssp->napps; i++) {
        APP& app = ssp->apps[i];
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL, "app %s (%d)\n", app.name, app.id);
        }
        RSC_INFO cpu_info, gpu_info;

        // find the average PFC of CPU and GPU versions

        for (j=0; j<ssp->napp_versions; j++) {
            APP_VERSION& avr = ssp->app_versions[j];
            if (avr.appid != app.id) continue;
            DB_APP_VERSION av;
            retval = av.lookup_id(avr.id);
            if (retval) return retval;
            avr = av;       // update shared mem array
            if (strstr(av.plan_class, "cuda") || strstr(av.plan_class, "ati")) {
                if (config.debug_credit) {
                    log_messages.printf(MSG_NORMAL,
                        "add to gpu totals: (%d %s) %g %g\n",
                        av.id, av.plan_class, av.pfc.n, av.pfc.get_avg()
                    );
                }
                gpu_info.update(av);
            } else {
                if (config.debug_credit) {
                    log_messages.printf(MSG_NORMAL,
                        "add to cpu totals: (%d %s) %g %g\n",
                        av.id, av.plan_class, av.pfc.n, av.pfc.get_avg()
                    );
                }
                cpu_info.update(av);
            }
        }

        // If there are only CPU or only GPU versions,
        // and at least 2 are above threshold, normalize to the average
        //
        // If there are both, and at least 1 of each is above threshold,
        // normalize to the min of the averages
        //
        if (cpu_info.nvers_total) {
            if (gpu_info.nvers_total) {
                if (cpu_info.nvers_thresh && gpu_info.nvers_thresh) {
                    if (config.debug_credit) {
                        log_messages.printf(MSG_NORMAL,
                            "CPU avg: %g; GPU avg: %g\n",
                            cpu_info.avg(), gpu_info.avg()
                        );
                    }
                    scale_versions(app,
                        cpu_info.avg()<gpu_info.avg()?cpu_info.avg():gpu_info.avg(),
                        ssp
                    );
                }
            } else {
                if (cpu_info.nvers_thresh > 1) {
                    log_messages.printf(MSG_NORMAL,
                        "CPU avg: %g\n", cpu_info.avg()
                    );
                    scale_versions(app, cpu_info.avg(), ssp);
                }
            }
        } else {
            if (gpu_info.nvers_thresh > 1) {
                log_messages.printf(MSG_NORMAL,
                    "GPU avg: %g\n", gpu_info.avg()
                );
                scale_versions(app, gpu_info.avg(), ssp);
            }
        }


    }
    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL, "-------------\n");
    }
    return 0;
}

// look up HOST_APP_VERSION record; called from validator and transitioner.
// Normally the record will exist; if not create it (transitional case)
//
int hav_lookup(DB_HOST_APP_VERSION& hav, int hostid, int avid) {
    int retval;
    char buf[256];
    sprintf(buf, "where host_id=%d and app_version_id=%d", hostid, avid);
    retval = hav.lookup(buf);
    if (retval == ERR_DB_NOT_FOUND) {
        hav.clear();
        hav.host_id = hostid;
        hav.app_version_id = avid;
        retval = hav.insert();
    }
    return retval;
}

DB_APP_VERSION* av_lookup(int id, vector<DB_APP_VERSION>& app_versions) {
    for (unsigned int i=0; i<app_versions.size(); i++) {
        if (app_versions[i].id == id) {
            return &app_versions[i];
        }
    }
    DB_APP_VERSION av;
    int retval = av.lookup_id(id);
    if (retval) return NULL;
    app_versions.push_back(av);
    return &(app_versions[app_versions.size()-1]);
}

// the estimated PFC for a given WU, in the absence of any other info
//
inline double wu_estimated_pfc(WORKUNIT& wu, DB_APP& app) {
    return wu.rsc_fpops_est*app.min_avg_pfc;
}
inline double wu_estimated_credit(WORKUNIT& wu, DB_APP& app) {
    return wu_estimated_pfc(wu, app)*COBBLESTONE_SCALE;
}

// Compute or estimate "claimed peak FLOP count".
// Possibly update host_app_version records and write to DB.
// Possibly update app_version records in memory and let caller write to DB,
// to merge DB writes
//
int get_pfc(
    RESULT& r, WORKUNIT& wu, DB_APP& app,       // in
    vector<DB_APP_VERSION>&app_versions,        // in/out
    DB_HOST_APP_VERSION& hav,                   // in/out
    double& pfc, int& mode                      // out
) {
    DB_APP_VERSION* avp=0;
    int retval;

    mode = PFC_MODE_APPROX;

    // is result from old scheduler that didn't set r.app_version_id correctly?
    // if so, use WU estimate (this is a transient condition)
    //
    if (r.app_version_id == 0 || r.app_version_id == 1) {
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] missing app_version_id (%d): returning WU default %.2f\n",
                r.id, r.app_version_id, wu_estimated_credit(wu, app)
            );
        }
        mode = PFC_MODE_WU_EST;
        pfc = wu_estimated_pfc(wu, app);
        return 0;
    }

    // temporary kludge for SETI@home:
    // if GPU initialization fails the app falls back to CPU.
    //
    if (strstr(r.stderr_out, "Device Emulation (CPU)")) {
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d][AV#%d] CUDA app fell back to CPU; returning WU default %.2f\n",
                r.id, r.app_version_id, wu.rsc_fpops_est*COBBLESTONE_SCALE
            );
        }
        mode = PFC_MODE_WU_EST;
        pfc = wu_estimated_pfc(wu, app);
        return 0;
    }

    int gavid = generalized_app_version_id(r.app_version_id, r.appid);

    // transition case
    //
    if (!hav.host_id) {
        mode = PFC_MODE_WU_EST;
        pfc = wu_estimated_pfc(wu, app);
        return 0;
    }

    // old clients report CPU time but not elapsed time.
    // Use HOST_APP_VERSION.et to track statistics of CPU time.
    //
    if (!r.elapsed_time) {
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] old client (elapsed time not reported)\n",
                r.id
            );
        }
        hav.et.update_var(
            r.cpu_time/wu.rsc_fpops_est,
            HAV_AVG_THRESH, HAV_AVG_WEIGHT, HAV_AVG_LIMIT
        );
        pfc = wu_estimated_pfc(wu, app);
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] old client: raw credit %.2f\n",
                r.id, pfc*COBBLESTONE_SCALE
            );
        }
        bool do_scale = true;
        if (hav.et.n < MIN_HOST_SAMPLES || (hav.et.get_avg() <= 0)) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] old client: no host scaling - zero or too few samples %f\n",
                    r.id, hav.et.n
                );
            }
        }
        if (do_scale
            && app.host_scale_check
            && hav.consecutive_valid < CONS_VALID_HOST_SCALE
        ) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] old client: no host scaling - cons valid %d\n",
                    r.id, hav.consecutive_valid
                );
            }
        }
        if (do_scale) {
            double s = r.cpu_time / (hav.et.get_avg()*wu.rsc_fpops_est);
            pfc *= s;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] old client: scaling (based on CPU time) by %g, return %.2f\n",
                    r.id, s, pfc*COBBLESTONE_SCALE
                );
            }
        }
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] old client: returning PFC %.2f\n",
                r.id, pfc*COBBLESTONE_SCALE
            );
        }
        return 0;
    }

    // r.flops_estimate shouldn't be zero,
    // but (because of scheduler bug) it can be.
    // At this point we don't have much to go on, so use 1e10.
    //
    if (!r.flops_estimate) {
        r.flops_estimate = 1e10;
    }

    double raw_pfc = (r.elapsed_time * r.flops_estimate);
    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL,
            "[credit] [RESULT#%d] raw credit: %.2f (%.2f sec, %.2f est GFLOPS)\n",
            r.id, raw_pfc*COBBLESTONE_SCALE, r.elapsed_time,
            r.flops_estimate/1e9
        );
    }

    // Sanity check
    //
    if (raw_pfc > wu.rsc_fpops_bound) {
        char query[256], clause[256];
        pfc = wu_estimated_pfc(wu, app);
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] sanity check failed: %.2f>%.2f, return %.2f\n",
                r.id, raw_pfc*COBBLESTONE_SCALE,
                wu.rsc_fpops_bound*COBBLESTONE_SCALE, pfc*COBBLESTONE_SCALE
            );
        }
        sprintf(query, "consecutive_valid=0");
        sprintf(clause, "host_id=%d and app_version_id=%d", r.hostid, gavid);
        retval = hav.update_fields_noid(query, clause);
        return retval;
    }

    if (r.app_version_id < 0) {
        // anon platform
        //
        bool do_scale = true;
        if (hav.pfc.n < MIN_HOST_SAMPLES || hav.pfc.get_avg()<=0) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] anon platform, not scaling, PFC avg zero or too few samples %.0f\n",
                    r.id, hav.pfc.n
                );
            }
        }
        if (do_scale
            && app.host_scale_check
            && hav.consecutive_valid < CONS_VALID_HOST_SCALE
        ) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] anon platform, not scaling, cons valid %d\n",
                    r.id, hav.consecutive_valid
                );
            }
        }
        if (do_scale) {
            double scale = app.min_avg_pfc / hav.pfc.get_avg();
            pfc = raw_pfc * scale;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] anon platform, scaling by %g (%.2f/%.2f)\n",
                    r.id, scale, app.min_avg_pfc, hav.pfc.get_avg()
                );
            }
        } else {
            pfc = wu_estimated_pfc(wu, app);
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] not scaling, using app avg %.2f\n",
                    r.id, pfc*COBBLESTONE_SCALE
                );
            }
        }
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] anon platform, returning %.2f\n",
                r.id, pfc*COBBLESTONE_SCALE
            );
        }
    } else {
        avp = av_lookup(r.app_version_id, app_versions);
        if (!avp) {
            log_messages.printf(MSG_CRITICAL,
                "get_pfc() [RESULT#%d]: No AVP %d!!\n", r.id, r.app_version_id
            );
            return ERR_NOT_FOUND;
        }
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] [AV#%d] normal case. %.0f sec, %.1f GFLOPS.  raw credit: %.2f\n",
                r.id, avp->id, r.elapsed_time, r.flops_estimate/1e9,
                raw_pfc*COBBLESTONE_SCALE
            );
        }

        bool do_scale = true;
        double host_scale = 0;
        if (app.host_scale_check
            && hav.consecutive_valid < CONS_VALID_HOST_SCALE
        ) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] not host scaling - cons valid %d\n",
                    r.id, hav.consecutive_valid
                );
            }
        }
        if (do_scale && (hav.pfc.n < MIN_HOST_SAMPLES || hav.pfc.get_avg()==0)) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] not host scaling - HAV PFC zero or too few samples %.0f\n",
                    r.id, hav.pfc.n
                );
            }
        }
        if (do_scale && avp->pfc.n < MIN_VERSION_SAMPLES) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] not host scaling - app_version PFC too few samples%.0f\n",
                    r.id, avp->pfc.n
                );
            }
        }
        if (do_scale && hav.pfc.get_avg() <= 0) {
            do_scale = false;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] not host scaling - HAV PFC is zero\n",
                    r.id
                );
            }
        }
        if (do_scale) {
            host_scale = avp->pfc.get_avg() / hav.pfc.get_avg();
            if (host_scale > 10) host_scale = 10;
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] host scale: %.2f (%f/%f)\n",
                    r.id, host_scale, avp->pfc.get_avg(), hav.pfc.get_avg()
                );
            }
        }

        pfc = raw_pfc;
        if (avp->pfc_scale) {
            pfc *= avp->pfc_scale;
            if (host_scale) {
                pfc *= host_scale;
                mode = PFC_MODE_NORMAL;
            }
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] applying app version scale %.3f\n",
                    r.id, avp->pfc_scale
                );
            }
        } else {
            if (host_scale) {
                pfc *= host_scale;
            }
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] no app version scale\n",
                    r.id
                );
            }
        }
        if (config.debug_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] [RESULT#%d] [AV#%d] PFC avgs with %g (%g/%g)\n",
                r.id, avp->id,
                raw_pfc/wu.rsc_fpops_est,
                raw_pfc, wu.rsc_fpops_est
            );
        }
        avp->pfc_samples.push_back(raw_pfc/wu.rsc_fpops_est);
    }

    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL,
            "[credit] [RESULT#%d] updating HAV PFC %.2f et %g turnaround %d\n",
            r.id, raw_pfc / wu.rsc_fpops_est,
            r.elapsed_time / wu.rsc_fpops_est,
            (r.received_time - r.sent_time)
        );
    }
                
    hav.pfc.update(
        raw_pfc / wu.rsc_fpops_est,
        HAV_AVG_THRESH, HAV_AVG_WEIGHT, HAV_AVG_LIMIT
    );
    hav.et.update_var(
        r.elapsed_time / wu.rsc_fpops_est,
        HAV_AVG_THRESH, HAV_AVG_WEIGHT, HAV_AVG_LIMIT
    );
    hav.turnaround.update_var(
        (r.received_time - r.sent_time),
        HAV_AVG_THRESH, HAV_AVG_WEIGHT, HAV_AVG_LIMIT
    );

    // keep track of credit per app version
    //
    if (avp) {
        avp->credit_samples.push_back(pfc*COBBLESTONE_SCALE);
        avp->credit_times.push_back(r.sent_time);
    }

    return 0;
}

// compute the average of some numbers,
// where each value is weighted by the sum of the other values.
// (reduces the weight of large outliers)
//
double low_average(vector<double>& v) {
    int i;
    int n = v.size();
    if (n == 1) {
        return v[0];
    }
    double sum=0;
    for (i=0; i<n; i++) {
        sum += v[i];
    }
    double total=0;
    for (i=0; i<n; i++) {
        total += v[i]*(sum-v[i]);
    }
    return total/((n-1)*sum);
}

double vec_min(vector<double>& v) {
    double x = v[0];
    for (unsigned int i=1; i<v.size(); i++) {
        if (v[i] < x) x = v[i];
    }
    return x;
}

// Called by validator when canonical result has been selected.
// Compute credit for valid instances, store in result.granted_credit
// and return as credit
//
int assign_credit_set(
    WORKUNIT& wu, vector<RESULT>& results,
    DB_APP& app,
    vector<DB_APP_VERSION>& app_versions,
    vector<DB_HOST_APP_VERSION>& host_app_versions,
    double max_granted_credit, double& credit
) {
    unsigned int i;
    int mode, retval;
    double pfc;
    vector<double> normal;
    vector<double> approx;

    for (i=0; i<results.size(); i++) {
        RESULT& r = results[i];
        if (r.validate_state != VALIDATE_STATE_VALID) continue;
        DB_HOST_APP_VERSION& hav = host_app_versions[i];
        retval = get_pfc(r, wu, app, app_versions, hav, pfc, mode);
        if (retval) {
            log_messages.printf(MSG_CRITICAL, "get_pfc() error: %d\n", retval);
            return retval;
        } else {
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] [RESULT#%d] get_pfc() returns credit %g mode %s\n",
                    r.id, pfc*COBBLESTONE_SCALE, (mode==PFC_MODE_NORMAL)?"normal":"approx"
                );
            }
        }
        if (max_granted_credit && pfc*COBBLESTONE_SCALE > max_granted_credit) {
            log_messages.printf(MSG_NORMAL,
                "[credit] Credit too high: %f\n", pfc*COBBLESTONE_SCALE
            );
            pfc = wu_estimated_pfc(wu, app);
        }
        if (pfc > wu.rsc_fpops_bound) {
            log_messages.printf(MSG_NORMAL,
                "[credit] PFC too high: %f\n", pfc*COBBLESTONE_SCALE
            );
            pfc = wu_estimated_pfc(wu, app);
        }
        if (mode == PFC_MODE_NORMAL) {
            normal.push_back(pfc);
        } else {
            approx.push_back(pfc);
        }
    }

    // averaging policy: if there is least one normal result,
    // use the "low average" of normal results.
    // Otherwise use the min of all results
    //
    double x;
    if (normal.size()) {
        x = low_average(normal);
    } else {
        x = vec_min(approx);
    }
    x *= COBBLESTONE_SCALE;
    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL,
            "[credit] [WU#%d] assign_credit_set: credit %g\n",
            wu.id, x
        );
    }
    for (i=0; i<results.size(); i++) {
        RESULT& r = results[i];
        if (r.validate_state != VALIDATE_STATE_VALID) continue;
        r.granted_credit = x;
    }
    credit = x;
    return 0;
}

// A job has:
// - errored out (scheduler)
// - timed out (transitioner)
// - failed validation (validator).
// Put (host/app_version) on "host scale probation",
// so that we won't use host scaling for a while.
//
void got_error(DB_HOST_APP_VERSION& hav) {
    if (config.debug_credit) {
        log_messages.printf(MSG_NORMAL,
            "[credit] [HAV#%d] got error, setting error rate to %f\n",
            hav.app_version_id, ERROR_RATE_INIT
        );
    }
}

// carefully write any app_version records that have changed;
// done at the end of every validator scan.
//
int write_modified_app_versions(vector<DB_APP_VERSION>& app_versions) {
    unsigned int i, j;
    int retval = 0;
    double now = dtime();

    if (config.debug_credit && app_versions.size()) {
        log_messages.printf(MSG_NORMAL,
            "[credit] start write_modified_app_versions()\n"
        );
    }
    for (i=0; i<app_versions.size(); i++) {
        DB_APP_VERSION& av = app_versions[i];
        if (av.pfc_samples.empty() && av.credit_samples.empty()) {
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] skipping app version %d - no change\n", av.id
                );
            }
            continue;
        }
        while (1) {
            double pfc_n_orig = av.pfc.n;
            double expavg_credit_orig = av.expavg_credit;

            for (j=0; j<av.pfc_samples.size(); j++) {
                av.pfc.update(
                    av.pfc_samples[j],
                    AV_AVG_THRESH, AV_AVG_WEIGHT, AV_AVG_LIMIT
                );
            }
            for (j=0; j<av.credit_samples.size(); j++) {
                update_average(
                    now,
                    av.credit_times[j], av.credit_samples[j], CREDIT_HALF_LIFE,
                    av.expavg_credit, av.expavg_time
                );
            }
            char query[512], clause[512];
            sprintf(query,
                "pfc_n=%.15e, pfc_avg=%.15e, expavg_credit=%.15e, expavg_time=%f",
                av.pfc.n,
                av.pfc.avg,
                av.expavg_credit,
                av.expavg_time
            );
            if (config.debug_credit) {
                log_messages.printf(MSG_NORMAL,
                    "[credit] updating app version %d:\n", av.id
                );
                log_messages.printf(MSG_NORMAL,
                    "[credit] pfc.n = %f, pfc.avg = %f, expavg_credit = %f, expavg_time=%f\n",
                    av.pfc.n,
                    av.pfc.avg,
                    av.expavg_credit,
                    av.expavg_time
                );
            }
            // if pfc_scale has changed (from feeder) reread it
            //
            sprintf(clause,
                "pfc_n=%.15e and abs(expavg_credit-%.15e)<1e-4 and abs(pfc_scale-%.15e)<1e-6",
                pfc_n_orig, expavg_credit_orig, av.pfc_scale
            );
            retval = av.update_field(query, clause);
            if (retval) break;
            if (boinc_db.affected_rows() == 1) break;
            retval = av.lookup_id(av.id);
            if (retval) break;
        }
        av.pfc_samples.clear();
        av.credit_samples.clear();
        av.credit_times.clear();
        if (retval) return retval;
    }
    return 0;
}
