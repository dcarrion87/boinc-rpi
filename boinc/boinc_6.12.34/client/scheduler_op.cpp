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

#include "cpp.h"

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#endif

#include "str_util.h"
#include "str_replace.h"
#include "util.h"
#include "parse.h"
#include "error_numbers.h"
#include "filesys.h"

#include "client_state.h"
#include "client_types.h"
#include "client_msgs.h"
#include "file_names.h"
#include "log_flags.h"
#include "main.h"
#include "scheduler_op.h"

//#define ENABLE_AUTO_UPDATE

using std::vector;

SCHEDULER_OP::SCHEDULER_OP(HTTP_OP_SET* h) {
    cur_proj = NULL;
    state = SCHEDULER_OP_STATE_IDLE;
    http_op.http_op_state = HTTP_STATE_IDLE;
    http_ops = h;
}

// See if there's a pending master file fetch.
// If so, start it and return true.
//
bool SCHEDULER_OP::check_master_fetch_start() {
    int retval;

    PROJECT* p = gstate.next_project_master_pending();
    if (!p) return false;
    retval = init_master_fetch(p);
    if (retval) {
        msg_printf(p, MSG_INTERNAL_ERROR,
            "Couldn't start download of scheduler list: %s", boincerror(retval)
        );
        p->master_fetch_failures++;
        backoff(p, "scheduler list fetch failed\n");
        return false;
    }
    msg_printf(p, MSG_INFO, "Fetching scheduler list");
    return true;
}

#ifndef SIM

// try to initiate an RPC to the given project.
// If there are multiple schedulers, start with a random one.
// User messages and backoff() is done at this level.
//
int SCHEDULER_OP::init_op_project(PROJECT* p, int r) {
    int retval;
    char err_msg[256];

    reason = r;
    if (log_flags.sched_op_debug) {
        msg_printf(p, MSG_INFO,
            "[sched_op] Starting scheduler request"
        );
    }

    // if project has no schedulers,
    // skip everything else and just get its master file.
    //
    if (p->scheduler_urls.size() == 0) {
        retval = init_master_fetch(p);
        if (retval) {
            sprintf(err_msg,
                "Scheduler list fetch initialization failed: %d\n", retval
            );
            backoff(p, err_msg);
        }
        return retval;
    }

    if (reason == RPC_REASON_INIT) {
        work_fetch.set_initial_work_request();
        if (!gstate.cpu_benchmarks_done()) {
            gstate.cpu_benchmarks_set_defaults();
        }
    }

    url_index = 0;
    retval = gstate.make_scheduler_request(p);
    if (!retval) {
        retval = start_rpc(p);
    }
    if (retval) {
        sprintf(err_msg,
            "scheduler request to %s failed: %s\n",
            p->get_scheduler_url(url_index, url_random), boincerror(retval)
        );
        backoff(p, err_msg);
    } else {
        // RPC started OK, so we must have network connectivity.
        // Now's a good time to check for new BOINC versions
        // and project list
        //
        if (!config.no_info_fetch) {
            gstate.new_version_check();
            gstate.all_projects_list_check();
        }
    }
    return retval;
}

#endif

// One of the following errors occurred:
// - connection failure in fetching master file
// - connection failure in scheduler RPC
// - got master file, but it didn't have any <scheduler> elements
// - tried all schedulers, none responded
// - sent nonzero work request, got a reply with no work
//
// Back off contacting this project's schedulers,
// and output an error msg if needed
//
void SCHEDULER_OP::backoff(PROJECT* p, const char *reason_msg) {
    char buf[1024];

    if (gstate.in_abort_sequence) {
        return;
    }

    if (p->master_fetch_failures >= gstate.master_fetch_retry_cap) {
        sprintf(buf,
            "%d consecutive failures fetching scheduler list",
            p->master_fetch_failures
        );
        p->master_url_fetch_pending = true;
        p->set_min_rpc_time(gstate.now + gstate.master_fetch_interval, buf);
        return;
    }

    // if nrpc failures is master_fetch_period,
    // then set master_url_fetch_pending and initialize again
    //
    if (p->nrpc_failures == gstate.master_fetch_period) {
        p->master_url_fetch_pending = true;
        p->min_rpc_time = 0;
        p->nrpc_failures = 0;
        p->master_fetch_failures++;
    }

    // if network is down, don't count it as RPC failure
    //
    if (!net_status.need_physical_connection) {
        p->nrpc_failures++;
    }
    //msg_printf(p, MSG_INFO, "nrpc_failures %d need_conn %d", p->nrpc_failures, gstate.need_physical_connection);

    int n = p->nrpc_failures;
    if (n > gstate.retry_cap) n = gstate.retry_cap;
    double exp_backoff = calculate_exponential_backoff(
        n, gstate.sched_retry_delay_min, gstate.sched_retry_delay_max
    );
    //msg_printf(p, MSG_INFO, "simulating backoff of %f", exp_backoff);
    p->set_min_rpc_time(gstate.now + exp_backoff, reason_msg);
}


// RPC failed, either on startup or later.
// If RPC was requested by project or acct mgr, or init,
// keep trying (subject to backoff); otherwise give up.
// The other cases (results_dur, need_work, project req, and trickle_up)
// will be retriggered automatically
//
void SCHEDULER_OP::rpc_failed(const char* msg) {
    backoff(cur_proj, msg);
    switch (cur_proj->sched_rpc_pending) {
    case RPC_REASON_INIT:
    case RPC_REASON_ACCT_MGR_REQ:
        break;
    default:
        cur_proj->sched_rpc_pending = 0;
    }
    cur_proj = 0;
}

static void request_string(char* buf) {
    bool first = true;
    strcpy(buf, "");
    if (cpu_work_fetch.req_secs) {
        strcpy(buf, "CPU");
        first = false;
    }
    if (cuda_work_fetch.req_secs) {
        if (!first) strcat(buf, " and ");
        strcat(buf, "NVIDIA GPU");
        first = false;
    }
    if (ati_work_fetch.req_secs) {
        if (!first) strcat(buf, " and ");
        strcat(buf, "ATI GPU");
        first = false;
    }
}

// low-level routine to initiate an RPC
// If successful, creates an HTTP_OP that must be polled
// PRECONDITION: the request file has been created
//
int SCHEDULER_OP::start_rpc(PROJECT* p) {
    int retval;
    char request_file[1024], reply_file[1024], buf[256];
    char *trickle_up_msg;

    safe_strcpy(scheduler_url, p->get_scheduler_url(url_index, url_random));
    if (log_flags.sched_ops) {
        msg_printf(p, MSG_INFO,
            "Sending scheduler request: %s.", rpc_reason_string(reason)
        );
        if (p->trickle_up_pending && reason != RPC_REASON_TRICKLE_UP) {
            trickle_up_msg = ", sending trickle-up message";
        } else {
            trickle_up_msg = "";
        }
        request_string(buf);
        if (strlen(buf)) {
            if (p->nresults_returned) {
                msg_printf(p, MSG_INFO,
                    "Reporting %d completed tasks, requesting new tasks for %s%s",
                    p->nresults_returned, buf, trickle_up_msg
                );
            } else {
                msg_printf(p, MSG_INFO, "Requesting new tasks for %s%s", buf, trickle_up_msg);
            }
        } else {
            if (p->nresults_returned) {
                msg_printf(p, MSG_INFO,
                    "Reporting %d completed tasks, not requesting new tasks%s",
                    p->nresults_returned, trickle_up_msg
                );
            } else {
                msg_printf(p, MSG_INFO, "Not reporting or requesting tasks%s", trickle_up_msg);
            }
        }
    }
    if (log_flags.sched_op_debug) {
        msg_printf(p, MSG_INFO,
            "[sched_op] CPU work request: %.2f seconds; %.2f CPUs",
            cpu_work_fetch.req_secs, cpu_work_fetch.req_instances
        );
        if (gstate.host_info.have_cuda()) {
            msg_printf(p, MSG_INFO,
                "[sched_op] NVIDIA GPU work request: %.2f seconds; %.2f GPUs",
                cuda_work_fetch.req_secs, cuda_work_fetch.req_instances
            );
        }
        if (gstate.host_info.have_ati()) {
            msg_printf(p, MSG_INFO,
                "[sched_op] ATI GPU work request: %.2f seconds; %.2f GPUs",
                ati_work_fetch.req_secs, ati_work_fetch.req_instances
            );
        }
    }

    get_sched_request_filename(*p, request_file, sizeof(request_file));
    get_sched_reply_filename(*p, reply_file, sizeof(reply_file));

    cur_proj = p;
    retval = http_op.init_post(scheduler_url, request_file, reply_file);
    if (retval) {
        if (log_flags.sched_ops) {
            msg_printf(p, MSG_INFO,
                "Scheduler request initialization failed: %s", boincerror(retval)
            );
        }
        rpc_failed("Scheduler request initialization failed");
        return retval;
    }
    http_ops->insert(&http_op);
    p->rpc_seqno++;
    state = SCHEDULER_OP_STATE_RPC;
    return 0;
}

// initiate a fetch of a project's master URL file
//
int SCHEDULER_OP::init_master_fetch(PROJECT* p) {
    int retval;
    char master_filename[256];

    get_master_filename(*p, master_filename, sizeof(master_filename));

    if (log_flags.sched_op_debug) {
        msg_printf(p, MSG_INFO, "[sched_op] Fetching master file");
    }
    cur_proj = p;
    retval = http_op.init_get(p->master_url, master_filename, true);
    if (retval) {
        if (log_flags.sched_ops) {
            msg_printf(p, MSG_INFO,
                "Master file fetch failed: %s", boincerror(retval)
            );
        }
        rpc_failed("Master file fetch initialization failed");
        return retval;
    }
    http_ops->insert(&http_op);
    state = SCHEDULER_OP_STATE_GET_MASTER;
    return 0;
}

// parse a master file.
//
int SCHEDULER_OP::parse_master_file(PROJECT* p, vector<std::string> &urls) {
    char buf[256], buf2[256];
    char master_filename[256];
    std::string str;
    FILE* f;
    int n;

    get_master_filename(*p, master_filename, sizeof(master_filename));
    f = boinc_fopen(master_filename, "r");
    if (!f) {
        msg_printf(p, MSG_INTERNAL_ERROR, "Can't open scheduler list file");
        return ERR_FOPEN;
    }
    p->scheduler_urls.clear();
    while (fgets(buf, 256, f)) {

        // allow for the possibility of > 1 tag per line here
        // (UMTS may collapse lines)
        //
        char* q = buf;
        while (q && parse_str(q, "<scheduler>", str)) {
            push_unique(str, urls);
            q = strstr(q, "</scheduler>");
            if (q) q += strlen("</scheduler>");
        }

        // check for new syntax: <link ...>
        //
        q = buf;
        while (q) {
            n = sscanf(q, "<link rel=\"boinc_scheduler\" href=\"%s", buf2);
            if (n == 1) {
                char* q2 = strchr(buf2, '"');
                if (q2) *q2 = 0;
                strip_whitespace(buf2);
                str = string(buf2);
                push_unique(str, urls);
            }
            q = strchr(q, '>');
            if (q) q = strchr(q, '<');
        }
    }
    fclose(f);
    if (log_flags.sched_op_debug) {
        msg_printf(p, MSG_INFO,
            "[sched_op] Found %d scheduler URLs in master file\n",
            (int)urls.size()
        );
    }

    // couldn't find any scheduler URLs in the master file?
    //
    if ((int) urls.size() == 0) {
        p->sched_rpc_pending = 0;
        return ERR_XML_PARSE;
    }

    return 0;
}

// A master file has just been read.
// transfer scheduler URLs to project.
// Return true if any of them is new
//
bool SCHEDULER_OP::update_urls(PROJECT* p, vector<std::string> &urls) {
    unsigned int i, j;
    bool found, any_new;

    any_new = false;
    for (i=0; i<urls.size(); i++) {
        found = false;
        for (j=0; j<p->scheduler_urls.size(); j++) {
            if (urls[i] == p->scheduler_urls[j]) {
                found = true;
                break;
            }
        }
        if (!found) any_new = true;
    }

    p->scheduler_urls.clear();
    for (i=0; i<urls.size(); i++) {
        p->scheduler_urls.push_back(urls[i]);
    }

    return any_new;
}

#ifndef SIM

// poll routine.  If an operation is in progress, check for completion
//
bool SCHEDULER_OP::poll() {
    int retval;
    vector<std::string> urls;
    bool changed;

    switch(state) {
    case SCHEDULER_OP_STATE_GET_MASTER:
        // here we're fetching the master file for a project
        //
        if (http_op.http_op_state == HTTP_STATE_DONE) {
            state = SCHEDULER_OP_STATE_IDLE;
            cur_proj->master_url_fetch_pending = false;
            http_ops->remove(&http_op);
            if (http_op.http_op_retval == 0) {
                if (log_flags.sched_op_debug) {
                    msg_printf(cur_proj, MSG_INFO,
                        "[sched_op] Got master file; parsing"
                    );
                }
                retval = parse_master_file(cur_proj, urls);
                if (retval || (urls.size()==0)) {
                    // master file parse failed.
                    //
                    cur_proj->master_fetch_failures++;
                    rpc_failed("Couldn't parse scheduler list");
                } else {
                    // parse succeeded
                    //
                    msg_printf(cur_proj, MSG_INFO, "Master file download succeeded");
                    cur_proj->master_fetch_failures = 0;
                    changed = update_urls(cur_proj, urls);
                    
                    // reenable scheduler RPCs if have new URLs
                    //
                    if (changed) {
                        cur_proj->min_rpc_time = 0;
                        cur_proj->nrpc_failures = 0;
                    }
                }
            } else {
                // master file fetch failed.
                //
                char buf[256];
                sprintf(buf, "Scheduler list fetch failed: %s",
                    boincerror(http_op.http_op_retval)
                );
                cur_proj->master_fetch_failures++;
                rpc_failed("Master file request failed");
            }
            gstate.set_client_state_dirty("Master fetch complete");
            gstate.request_work_fetch("Master fetch complete");
            cur_proj = NULL;
            return true;
        }
        break;
    case SCHEDULER_OP_STATE_RPC:

        // here we're doing a scheduler RPC
        //
        if (http_op.http_op_state == HTTP_STATE_DONE) {
            state = SCHEDULER_OP_STATE_IDLE;
            http_ops->remove(&http_op);
            if (http_op.http_op_retval) {
                if (log_flags.sched_ops) {
                    msg_printf(cur_proj, MSG_INFO,
                        "Scheduler request failed: %s", http_op.error_msg
                    );
                }

                // scheduler RPC failed.  Try another scheduler if one exists
                //
                while (1) {
                    url_index++;
                    if (url_index == (int)cur_proj->scheduler_urls.size()) {
                        break;
                    }
                    retval = start_rpc(cur_proj);
                    if (!retval) return true;
                }
                if (url_index == (int) cur_proj->scheduler_urls.size()) {
                    rpc_failed("Scheduler request failed");
                }
            } else {
                retval = gstate.handle_scheduler_reply(cur_proj, scheduler_url);
                switch (retval) {
                case 0:
                    break;
                case ERR_PROJECT_DOWN:
                    backoff(cur_proj, "project is down");
                    break;
                default:
                    backoff(cur_proj, "can't parse scheduler reply");
                    break;
                }
				cur_proj->sched_rpc_pending = 0;
                    // do this after handle_scheduler_reply()
            }
            cur_proj = NULL;
            gstate.set_client_state_dirty("RPC complete");
            gstate.request_work_fetch("RPC complete");
            return true;
        }
    }
    return false;
}

#endif

void SCHEDULER_OP::abort(PROJECT* p) {
    if (state != SCHEDULER_OP_STATE_IDLE && cur_proj == p) {
        gstate.http_ops->remove(&http_op);
        state = SCHEDULER_OP_STATE_IDLE;
        cur_proj = NULL;
    }
}

void SCHEDULER_REPLY::clear() {
    hostid = 0;
    request_delay = 0;
    next_rpc_delay = 0;
    global_prefs_xml = 0;
    project_prefs_xml = 0;
    code_sign_key = 0;
    code_sign_key_signature = 0;
    strcpy(master_url, "");
    code_sign_key = 0;
    code_sign_key_signature = 0;
    message_ack = false;
    project_is_down = false;
    send_file_list = false;
    send_full_workload = false;
    send_time_stats_log = 0;
    send_job_log = 0;
    messages.clear();
    scheduler_version = 0;
    cpu_backoff = 0;
    cuda_backoff = 0;
    ati_backoff = 0;
    got_rss_feeds = false;
}

SCHEDULER_REPLY::SCHEDULER_REPLY() {
    clear();
}

SCHEDULER_REPLY::~SCHEDULER_REPLY() {
    if (global_prefs_xml) free(global_prefs_xml);
    if (project_prefs_xml) free(project_prefs_xml);
    if (code_sign_key) free(code_sign_key);
    if (code_sign_key_signature) free(code_sign_key_signature);
}

#ifndef SIM

// parse a scheduler reply.
// Some of the items go into the SCHEDULER_REPLY object.
// Others are copied straight to the PROJECT
//
int SCHEDULER_REPLY::parse(FILE* in, PROJECT* project) {
    char buf[256], msg_buf[1024], pri_buf[256];
    int retval;
    MIOFILE mf;
    std::string delete_file_name;
    mf.init_file(in);
    bool found_start_tag = false, btemp;
    double cpid_time = 0;

    clear();
    strcpy(host_venue, project->host_venue);
        // the project won't send us a venue if it's doing maintenance
        // or doesn't check the DB because no work.
        // Don't overwrite the host venue in that case.
    sr_feeds.clear();

    // First line should either be tag (HTTP 1.0) or
    // hex length of response (HTTP 1.1)
    //
    while (fgets(buf, 256, in)) {
        if (!found_start_tag) {
            if (match_tag(buf, "<scheduler_reply")) {
                found_start_tag = true;
            }
            continue;
        }
        if (match_tag(buf, "</scheduler_reply>")) {

            // update statistics after parsing the scheduler reply
            // add new record if vector is empty or we have a new day
            //
            if (project->statistics.empty() || project->statistics.back().day!=dday()) {
                double cutoff = dday() - config.save_stats_days*86400;
                // delete old stats; fill in the gaps if some days missing
                //
                while (!project->statistics.empty()) {
                    DAILY_STATS& ds = project->statistics[0];
                    if (ds.day >= cutoff) {
                        break;
                    }
                    if (project->statistics.size() > 1) {
                        DAILY_STATS& ds2 = project->statistics[1];
                        if (ds2.day <= cutoff) {
                            project->statistics.erase(project->statistics.begin());
                        } else {
                            ds.day = cutoff;
                            break;
                        }
                    } else {
                        ds.day = cutoff;
                        break;
                    }
                }

                DAILY_STATS nds;
                project->statistics.push_back(nds);
            }
            DAILY_STATS& ds = project->statistics.back();
            ds.day=dday();
            ds.user_total_credit=project->user_total_credit;
            ds.user_expavg_credit=project->user_expavg_credit;
            ds.host_total_credit=project->host_total_credit;
            ds.host_expavg_credit=project->host_expavg_credit;

            project->write_statistics_file();

            if (cpid_time) {
                project->cpid_time = cpid_time;
            } else {
                project->cpid_time = project->user_create_time;
            }
            return 0;
        }
        else if (parse_str(buf, "<project_name>", project->project_name, sizeof(project->project_name))) {
            continue;
        }
        else if (parse_str(buf, "<master_url>", master_url, sizeof(master_url))) {
            continue;
        }
        else if (parse_str(buf, "<symstore>", project->symstore, sizeof(project->symstore))) continue;
        else if (parse_str(buf, "<user_name>", project->user_name, sizeof(project->user_name))) continue;
        else if (parse_double(buf, "<user_total_credit>", project->user_total_credit)) continue;
        else if (parse_double(buf, "<user_expavg_credit>", project->user_expavg_credit)) continue;
        else if (parse_double(buf, "<user_create_time>", project->user_create_time)) continue;
        else if (parse_double(buf, "<cpid_time>", cpid_time)) continue;
        else if (parse_str(buf, "<team_name>", project->team_name, sizeof(project->team_name))) continue;
        else if (parse_int(buf, "<hostid>", hostid)) continue;
        else if (parse_double(buf, "<host_total_credit>", project->host_total_credit)) continue;
        else if (parse_double(buf, "<host_expavg_credit>", project->host_expavg_credit)) continue;
        else if (parse_str(buf, "<host_venue>", host_venue, sizeof(host_venue))) continue;
        else if (parse_double(buf, "<host_create_time>", project->host_create_time)) continue;
        else if (parse_double(buf, "<request_delay>", request_delay)) continue;
        else if (parse_double(buf, "<next_rpc_delay>", next_rpc_delay)) continue;
        else if (match_tag(buf, "<global_preferences>")) {
            retval = dup_element_contents(
                in,
                "</global_preferences>",
                &global_prefs_xml
            );
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse global prefs in scheduler reply: %s",
                    boincerror(retval)
                );
                return retval;
            }
        } else if (match_tag(buf, "<project_preferences>")) {
            retval = dup_element_contents(
                in,
                "</project_preferences>",
                &project_prefs_xml
            );
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse project prefs in scheduler reply: %s",
                    boincerror(retval)
                );
                return retval;
            }
        } else if (match_tag(buf, "<gui_urls>")) {
            std::string foo;
            retval = copy_element_contents(in, "</gui_urls>", foo);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse GUI URLs in scheduler reply: %s",
                    boincerror(retval)
                );
                return retval;
            }
            project->gui_urls = "<gui_urls>\n"+foo+"</gui_urls>\n";
            continue;
        } else if (match_tag(buf, "<code_sign_key>")) {
            retval = dup_element_contents(
                in,
                "</code_sign_key>",
                &code_sign_key
            );
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse code sign key in scheduler reply: %s",
                    boincerror(retval)
                );
                return ERR_XML_PARSE;
            }
        } else if (match_tag(buf, "<code_sign_key_signature>")) {
            retval = dup_element_contents(
                in,
                "</code_sign_key_signature>",
                &code_sign_key_signature
            );
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse code sign key signature in scheduler reply: %s",
                    boincerror(retval)
                );
                return ERR_XML_PARSE;
            }
        } else if (match_tag(buf, "<app>")) {
            APP app;
            retval = app.parse(mf);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse application in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                apps.push_back(app);
            }
        } else if (match_tag(buf, "<file_info>")) {
            FILE_INFO file_info;
            retval = file_info.parse(mf, true);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse file info in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                file_infos.push_back(file_info);
            }
        } else if (match_tag(buf, "<app_version>")) {
            APP_VERSION av;
            retval = av.parse(mf);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse application version in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                app_versions.push_back(av);
            }
        } else if (match_tag(buf, "<workunit>")) {
            WORKUNIT wu;
            retval = wu.parse(mf);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse workunit in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                workunits.push_back(wu);
            }
        } else if (match_tag(buf, "<result>")) {
            RESULT result;      // make sure this is here so constructor
                                // gets called each time
            retval = result.parse_server(mf);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse task in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                results.push_back(result);
            }
        } else if (match_tag(buf, "<result_ack>")) {
            RESULT result;
            retval = result.parse_name(in, "</result_ack>");
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse ack in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                result_acks.push_back(result);
            }
        } else if (match_tag(buf, "<result_abort>")) {
            RESULT result;
            retval = result.parse_name(in, "</result_abort>");
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse result abort in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                result_abort.push_back(result);
            }
        } else if (match_tag(buf, "<result_abort_if_not_started>")) {
            RESULT result;
            retval = result.parse_name(in, "</result_abort_if_not_started>");
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't parse result abort-if-not-started in scheduler reply: %s",
                    boincerror(retval)
                );
            } else {
                result_abort_if_not_started.push_back(result);
            }
        } else if (parse_str(buf, "<delete_file_info>", delete_file_name)) {
            file_deletes.push_back(delete_file_name);
        } else if (parse_str(buf, "<message", msg_buf, sizeof(msg_buf))) {
            parse_attr(buf, "priority", pri_buf, sizeof(pri_buf));
            USER_MESSAGE um(msg_buf, pri_buf);
            messages.push_back(um);
            continue;
        } else if (parse_bool(buf, "message_ack", message_ack)) {
            continue;
        } else if (parse_bool(buf, "project_is_down", project_is_down)) {
            continue;
        } else if (parse_str(buf, "<email_hash>", project->email_hash, sizeof(project->email_hash))) {
            continue;
        } else if (parse_str(buf, "<cross_project_id>", project->cross_project_id, sizeof(project->cross_project_id))) {
            continue;
        } else if (match_tag(buf, "<trickle_down>")) {
            retval = gstate.handle_trickle_down(project, in);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "handle_trickle_down failed: %s", boincerror(retval)
                );
            }
            continue;
        } else if (parse_bool(buf, "non_cpu_intensive", project->non_cpu_intensive)) {
            continue;
        } else if (parse_bool(buf, "ended", project->ended)) {
            continue;
        } else if (parse_bool(buf, "no_cpu_apps", btemp)) {
            if (!project->anonymous_platform) {
                project->no_cpu_apps = btemp;
            }
            continue;
        } else if (parse_bool(buf, "no_cuda_apps", btemp)) {
            if (!project->anonymous_platform) {
                project->no_cuda_apps = btemp;
            }
            continue;
        } else if (parse_bool(buf, "no_ati_apps", btemp)) {
            if (!project->anonymous_platform) {
                project->no_ati_apps = btemp;
            }
            continue;
        } else if (parse_bool(buf, "verify_files_on_app_start", project->verify_files_on_app_start)) {
            continue;
        } else if (parse_bool(buf, "request_file_list", send_file_list)) {
            continue;
        } else if (parse_bool(buf, "send_full_workload", send_full_workload)) {
            continue;
        } else if (parse_int(buf, "<send_time_stats_log>", send_time_stats_log)){
            continue;
        } else if (parse_int(buf, "<send_job_log>", send_job_log)) {
            continue;
        } else if (parse_int(buf, "<scheduler_version>", scheduler_version)) {
            continue;
        } else if (match_tag(buf, "<project_files>")) {
            retval = project->parse_project_files(mf, true);
#ifdef ENABLE_AUTO_UPDATE
        } else if (match_tag(buf, "<auto_update>")) {
            retval = auto_update.parse(mf);
            if (!retval) auto_update.present = true;
#endif
        } else if (parse_double(buf, "<cpu_backoff>", cpu_backoff)) {
            if (cpu_backoff > 28*SECONDS_PER_DAY) cpu_backoff = 28*SECONDS_PER_DAY;
            if (cpu_backoff < 0) cpu_backoff = 0;
            continue;
        } else if (parse_double(buf, "<cuda_backoff>", cuda_backoff)) {
            if (cuda_backoff > 28*SECONDS_PER_DAY) cuda_backoff = 28*SECONDS_PER_DAY;
            if (cuda_backoff < 0) cuda_backoff = 0;
            continue;
        } else if (parse_double(buf, "<ati_backoff>", ati_backoff)) {
            if (ati_backoff > 28*SECONDS_PER_DAY) ati_backoff = 28*SECONDS_PER_DAY;
            if (ati_backoff < 0) ati_backoff = 0;
            continue;
        } else if (match_tag(buf, "<rss_feeds>")) {
            got_rss_feeds = true;
            parse_rss_feed_descs(mf, sr_feeds);
            continue;
        } else if (parse_int(buf, "<userid>", project->userid)) {
            continue;
        } else if (parse_int(buf, "<teamid>", project->teamid)) {
            continue;
        } else if (match_tag(buf, "<!--")) {
            continue;
        } else if (strlen(buf)>1){
            if (log_flags.unparsed_xml) {
                msg_printf(project, MSG_INFO,
                    "[unparsed_xml] SCHEDULER_REPLY::parse(): unrecognized %s\n", buf
                );
            }
        }
    }
    if (found_start_tag) {
        msg_printf(project, MSG_INTERNAL_ERROR, "No close tag in scheduler reply");
    } else {
        msg_printf(project, MSG_INTERNAL_ERROR, "No start tag in scheduler reply");
    }

    return ERR_XML_PARSE;
}
#endif

USER_MESSAGE::USER_MESSAGE(char* m, char* p) {
    message = m;
    priority = p;
}

