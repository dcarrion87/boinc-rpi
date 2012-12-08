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

// scheduler component for data archival.
// Called on each scheduler request.

#ifdef _USING_FCGI_
#include "boinc_fcgi.h"
#else
#include <cstdio>
#endif

#include <map>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

#include "filesys.h"

#include "backend_lib.h"

#include "sched_config.h"
#include "sched_msgs.h"
#include "sched_types.h"
#include "sched_util.h"

#include "vda_lib.h"

#include "sched_vda.h"

using std::map;
using std::string;
using std::pair;

static int parse_physical_filename(
    const char* name, int& hostid, char* chunk_name, char* file_name
) {
    char buf[1024];
    strcpy(buf, name);
    if (strstr(buf, "vda_") != buf) {
        return 1;
    }
    char *p = buf + strlen("vda_");
    if (sscanf(p, "%d", &hostid) != 1) {
        return 1;
    }
    p = strchr(p, '_');
    if (!p) return 1;
    p++;
    char* q = strchr(p, '_');
    if (!q) return 1;
    *q = 0;
    strcpy(chunk_name, p);
    strcpy(file_name, q+1);
    return 0;
}

// mark a vda_file for update by vdad
//
static int mark_for_update(int vda_file_id) {
    DB_VDA_FILE f;
    f.id = vda_file_id;
    return f.update_field("need_update=1");
}

typedef map<string, DB_VDA_CHUNK_HOST> CHUNK_LIST;

// get the path to the chunk's directory
//
static void get_chunk_dir(DB_VDA_FILE& vf, const char* chunk_name, char* dir) {
    char chunk_dirs[256];
    strcpy(chunk_dirs, chunk_name);
    while (1) {
        char* p = strchr(chunk_dirs, '.');
        if (!p) break;
        *p = '/';
    }
    sprintf(dir, "%s/%s", vf.dir, chunk_dirs);
}

static void get_chunk_url(DB_VDA_FILE& vf, const char* chunk_name, char* url) {
    char chunk_dirs[256], buf[1024];
    strcpy(chunk_dirs, chunk_name);
    while (1) {
        char* p = strchr(chunk_dirs, '.');
        if (!p) break;
        *p = '/';
    }
    dir_hier_url(
        vf.file_name, config.download_url, config.uldl_dir_fanout, buf
    );
    sprintf(url, "%s/%s/data.vda", buf, chunk_dirs);
}

// read the chunk's MD5 file into a buffer
//
static int get_chunk_md5(char* chunk_dir, char* md5_buf) {
    char md5_path[1024];
    sprintf(md5_path, "%s/md5.txt", chunk_dir);
#ifndef _USING_FCGI_
    FILE* f = fopen(md5_path, "r");
#else
    FCGI_FILE* f = FCGI::fopen(md5_path, "r");
#endif
    if (!f) return ERR_FOPEN;
    char* p = fgets(md5_buf, 64, f);
    fclose(f);
    if (p == NULL) return ERR_GETS;
    strip_whitespace(md5_buf);
    return 0;
}

// process a completed upload:
// if vda_chunk_host found
//      verify md5 of upload
//      move it from upload dir to vda_file dir
//      mark vda_file for update
//      clear transfer_in_progress flag in vda_chunk_host
// else
//      delete from upload dir
//
static int process_completed_upload(char* phys_filename, CHUNK_LIST& chunks) {
    char path[1024], buf[256];
    char chunk_name[1024], file_name[1024];
    int retval, hostid;

    retval = parse_physical_filename(
        phys_filename, hostid, chunk_name, file_name
    );
    if (retval) {
        log_messages.printf(MSG_NORMAL,
            "[vda] bad upload filename: %s\n", phys_filename
        );
        return retval;
    }
    dir_hier_path(
        phys_filename, config.upload_dir, config.uldl_dir_fanout, path, false
    );

    // if we don't have a DB record for this chunk, delete the file
    // TODO: maybe we should create a DB record instead
    //
    CHUNK_LIST::iterator i2 = chunks.find(string(phys_filename));
    if (i2 == chunks.end()) {
        if (config.debug_vda) {
            log_messages.printf(MSG_NORMAL,
                "[vda] chunk_host not found for %s\n", chunk_name
            );
        }
        boinc_delete_file(path);
        return 0;
    }

    char client_md5[64], server_md5[64];
    char chunk_dir[1024];
    DB_VDA_CHUNK_HOST& ch = i2->second;
    DB_VDA_FILE vf;
    double size;

    retval = vf.lookup_id(ch.vda_file_id);
    get_chunk_dir(vf, chunk_name, chunk_dir);

    // if file already exists on server, delete the upload file
    // Otherwise move the file from upload dir to data dir
    //
    sprintf(buf, "%s/data.vda", chunk_dir);
    if (boinc_file_exists(buf)) {
        boinc_delete_file(path);
    } else {
        retval = get_chunk_md5(chunk_dir, server_md5);
        if (retval) return retval;
        retval = md5_file(path, client_md5, size);
        if (retval) return retval;
        if (strcmp(client_md5, server_md5)) {
            if (config.debug_vda) {
                log_messages.printf(MSG_NORMAL,
                    "[vda] MD5 mismatch %s %s\n", client_md5, server_md5
                );
            }
            boinc_delete_file(path);
        } else {
            char dst_path[1024];
            sprintf(buf, "%s/data.vda", chunk_dir);
            ssize_t n = readlink(buf, dst_path, sizeof(dst_path)-1);
            if (n < 0) {
                log_messages.printf(MSG_CRITICAL,
                    "[vda] readlink() failed\n"
                );
            } else {
                dst_path[n] = 0;
                sprintf(buf, "mv %s %s; chmod g+rw %s", path, dst_path, dst_path);
                retval = system(buf);
                if (retval == -1 || WEXITSTATUS(retval)) {
                    log_messages.printf(MSG_NORMAL,
                        "[vda] command failed: %s\n", buf
                    );
                } else {
                    log_messages.printf(MSG_NORMAL,
                        "[vda] file move succeeded: %s\n", buf
                    );
                }
            }
            retval = vf.update_field("need_update=1");
        }
    }
    sprintf(buf, "host_id=%d and physical_file_name='%s'",
        ch.host_id,
        ch.physical_file_name
    );
    ch.transfer_in_progress = 0;
    retval = ch.update_fields_noid("transfer_in_progress=0", buf);
    if (retval) return retval;
    return 0;
}

// Process a file that's present on client.
// A VDA_CHUNK_HOST record may not be in the DB,
// e.g. because this host hasn't communicated in a while
// and we deleted the VDA_CHUNK_HOST record
// So:
// - create a vda_chunk_host record if needed
// - set present_on_host flag in vda_chunk_host
// - mark our in-memory vda_chunk_host record as "found"
// - mark vda_file for update
//
static void process_chunk_present_on_client(FILE_INFO& fi, CHUNK_LIST& chunks) {
    char fname[256], chunk_name[256], buf[1024];
    int hostid, retval;
    retval = parse_physical_filename(fi.name, hostid, chunk_name, fname);
    if (retval) {
        log_messages.printf(MSG_CRITICAL,
            "Can't parse VDA filename %s\n", fi.name
        );
        return;
    }

    DB_VDA_FILE vf;
    sprintf(buf, "where file_name='%s'", fname);
    retval = vf.lookup(buf);
    if (retval) {
        log_messages.printf(MSG_CRITICAL,
            "No VDA file for %s, deleting\n", fi.name
        );
        delete_file_xml(fi.name, buf);
        g_reply->file_transfer_requests.push_back(string(buf));
        return;
    }

    if (fi.nbytes != vf.chunk_size) {
        log_messages.printf(MSG_CRITICAL,
            "wrong chunk size for %s: %.0f != %.0f, deleting\n",
            fi.name, fi.nbytes, vf.chunk_size
        );
        delete_file_xml(fi.name, buf);
        g_reply->file_transfer_requests.push_back(string(buf));
        return;
    }

    CHUNK_LIST::iterator cli = chunks.find(string(fi.name));
    if (cli == chunks.end()) {
        // we don't have a record of this chunk on this host; make one
        //
        DB_VDA_CHUNK_HOST ch;
        ch.create_time = dtime();
        ch.vda_file_id = vf.id;
        ch.host_id = g_reply->host.id;
        strcpy(ch.physical_file_name, fi.name);
        ch.present_on_host = true;
        ch.transfer_in_progress = false;
        ch.transfer_wait = false;
        ch.transfer_request_time = 0;
        ch.transfer_send_time = 0;
        retval = ch.insert();
        if (retval) {
            log_messages.printf(MSG_CRITICAL, "ch.insert() failed\n");
            return;
        }
        mark_for_update(vf.id);
    } else {
        // we already have a DB record.
        // If needed, update it and mark file for update
        //
        DB_VDA_CHUNK_HOST* chp = &(cli->second);
        chp->found = true;

        // if file wasn't previously on host, update the main file
        //
        if (!chp->present_on_host) {
            mark_for_update(vf.id);

            chp->transfer_in_progress = false;
            chp->transfer_wait = false;
            chp->present_on_host = true;
            sprintf(buf,
                "host_id=%d and physical_file_name='%s'",
                chp->host_id, chp->physical_file_name
            );
            chp->update_fields_noid(
                "transfer_in_progress=0, transfer_wait=0, present_on_host=1",
                buf
            );
        }
    }
}

// for each vda_chunk_host not in file list:
// - delete from DB
// - mark vda_file for update
//
static int process_chunks_missing_on_client(CHUNK_LIST& chunks) {
    CHUNK_LIST::iterator it;
    for (it = chunks.begin(); it != chunks.end(); it++) {
        DB_VDA_CHUNK_HOST& ch = it->second;
        if (!ch.present_on_host && ch.transfer_in_progress) continue;
        if (!ch.found) {
            if (config.debug_vda) {
                log_messages.printf(MSG_NORMAL,
                    "[vda] in DB but not on client: %s\n", ch.physical_file_name
                );
            }
            char buf[256];
            sprintf(buf,
                "host_id=%d and vda_file_id=%d and physical_file_name='%s'",
                ch.host_id, ch.vda_file_id, ch.physical_file_name
            );
            int retval = ch.delete_from_db_multi(buf);
            if (retval) {
                log_messages.printf(MSG_CRITICAL,
                    "VDA: failed to delete %s\n", buf
                );
            }
            ch.transfer_in_progress = false;
            mark_for_update(ch.vda_file_id);
        }
    }
    return 0;
}

// if project is using more than its share of disk space,
// remove some chunks and mark vda_files for update
//
static int enforce_quota(CHUNK_LIST& chunks) {
    if (!g_request->host.d_boinc_max) return 0;

    double x = g_request->host.d_boinc_used_project;
    if (config.debug_vda) {
        log_messages.printf(MSG_NORMAL,
            "[vda] share: %f used: %f\n",
            g_request->host.d_boinc_max, x
        );
    }
    CHUNK_LIST::iterator it = chunks.begin();
    while (x > g_request->host.d_boinc_max && it != chunks.end()) {
        DB_VDA_CHUNK_HOST& ch = it->second;
        if (!ch.found) continue;
        FILE_INFO fi;
        strcpy(fi.name, ch.physical_file_name);
        if (config.debug_vda) {
            log_messages.printf(MSG_NORMAL,
                "[vda] deleting: %s\n", ch.physical_file_name
            );
        }
        DB_VDA_FILE vf;
        vf.lookup_id(ch.vda_file_id);
        x -= vf.chunk_size;
        g_reply->file_deletes.push_back(fi);
        it++;
    }
    return 0;
}

// does the host already have a result of the given name?
//
static bool result_already_on_host(const char* name) {
    for (unsigned int i=0; i<g_request->other_results.size(); i++) {
        OTHER_RESULT& r = g_request->other_results[i];
        if (!strcmp(r.name, name)) {
            return true;
        }
    }
    return false;
}

// issue upload and download commands
//
static int issue_transfer_commands(CHUNK_LIST& chunks) {
    char xml_buf[8192], chunk_name[256], file_name[1024];
    int retval;
    char url[1024], buf[1024];

    CHUNK_LIST::iterator it;
    for (it = chunks.begin(); it != chunks.end(); it++) {
        vector<const char*> urls;
        DB_VDA_CHUNK_HOST& ch = it->second;
        if (!ch.transfer_in_progress) continue;
        if (!ch.transfer_wait) continue;
        DB_VDA_FILE vf;
        retval = vf.lookup_id(ch.vda_file_id);
        if (retval) return retval;
        if (ch.present_on_host) {
            // upload
            //
            sprintf(buf, "upload_%s", ch.physical_file_name);
            if (result_already_on_host(buf)) {
                if (config.debug_vda) {
                    log_messages.printf(MSG_NORMAL,
                        "[vda] upload of %s already in progress\n",
                        ch.physical_file_name
                    );
                }
                continue;
            }
            if (config.debug_vda) {
                log_messages.printf(MSG_NORMAL,
                    "[vda] sending command to upload %s\n",
                    ch.physical_file_name
                );
            }
            urls.push_back(config.upload_url);
            R_RSA_PRIVATE_KEY key;
            retval = get_file_xml(
                ch.physical_file_name,
                urls,
                vf.chunk_size,
                dtime() + VDA_HOST_TIMEOUT,
                false,
                key,
                xml_buf
            );
        } else {
            // download
            //
            sprintf(buf, "download_%s", ch.physical_file_name);
            if (result_already_on_host(buf)) {
                if (config.debug_vda) {
                    log_messages.printf(MSG_NORMAL,
                        "[vda] download of %s already in progress\n",
                        ch.physical_file_name
                    );
                }
                continue;
            }
            char md5[64], chunk_dir[1024];
            int hostid;
            if (config.debug_vda) {
                log_messages.printf(MSG_NORMAL,
                    "[vda] sending command to download %s\n",
                    ch.physical_file_name
                );
            }
            parse_physical_filename(
                ch.physical_file_name,
                hostid,
                chunk_name,
                file_name
            );

            get_chunk_url(vf, chunk_name, url);
            urls.push_back(url);
            get_chunk_dir(vf, chunk_name, chunk_dir);
            retval = get_chunk_md5(chunk_dir, md5);
            if (retval) return retval;
            retval = put_file_xml(
                ch.physical_file_name,
                urls,
                md5,
                vf.chunk_size,
                dtime() + VDA_HOST_TIMEOUT,
                xml_buf
            );
        }
        g_reply->file_transfer_requests.push_back(string(xml_buf));
    }
    return 0;
}

// handle a scheduler request:
//
// - handle completed uploads
// - handle set of files present on client
//   (update or create VDA_CHUNK_HOST record)
// - handle files expected but not present
// - issue delete commands if needed to enforce share
// - issue upload or download commands to client
//
// relevant fields of SCHEDULER_REQUEST
// file_infos: list of sticky files
// file_xfer_results: list of completed file xfers
//
void handle_vda() {
    int retval;
    unsigned int i;
    CHUNK_LIST chunks;
        // chunks that are supposed to be on this host

    // if client is outdated, mark as dead
    //
    if (outdated_client(g_reply->host)) {
        g_reply->host.cpu_efficiency = 1;
        return;
    }

    // otherwise mark it as alive
    //
    g_reply->host.cpu_efficiency = 0;

    // enumerate the vda_chunk_host records for this host from DB
    //
    DB_VDA_CHUNK_HOST ch;
    char buf[256];
    sprintf(buf, "where host_id=%d", g_reply->host.id);
    while (1) {
        retval = ch.enumerate(buf);
        if (retval == ERR_DB_NOT_FOUND) break;
        if (retval) {
            // if we didn't get a complete enumeration,
            // give up rather than continuing with partial info
            //
            log_messages.printf(MSG_NORMAL,
                "[vda]: ch.enumerate() failed %d\n", retval
            );
            return;
        }
        if (config.debug_vda) {
            log_messages.printf(MSG_NORMAL,
                "[vda] DB: has chunk %s, file %d\n",
                ch.physical_file_name, ch.vda_file_id
            );
        }
        chunks.insert(
            pair<string, DB_VDA_CHUNK_HOST>(string(ch.physical_file_name), ch)
        );
    }

    // process completed uploads
    // (completed downloads are handled below)
    //
    for (i=0; i<g_request->file_xfer_results.size(); i++) {
        RESULT& r = g_request->file_xfer_results[i];
        if (strstr(r.name, "upload_vda_")) {
            char* phys_file_name = r.name + strlen("upload_");
            if (config.debug_vda) {
                log_messages.printf(MSG_NORMAL,
                    "[vda] completed upload of %s\n", phys_file_name
                );
            }
            retval = process_completed_upload(phys_file_name, chunks);
            if (retval) {
                log_messages.printf(MSG_CRITICAL,
                    "[vda] process_completed_upload(): %d\n", retval
                );
                return;
            }
        }
    }

    // process files present on client
    //
    for (i=0; i<g_request->file_infos.size(); i++) {
        FILE_INFO& fi = g_request->file_infos[i];
        if (!starts_with(fi.name, "vda_")) {
            continue;
        }
        if (config.debug_vda) {
            log_messages.printf(MSG_NORMAL,
                "[vda] request: client has file %s, status %d\n",
                fi.name, fi.status
            );
        }
        if (fi.status != FILE_PRESENT) {
            continue;
        }
        process_chunk_present_on_client(fi, chunks);
    }

    process_chunks_missing_on_client(chunks);

    enforce_quota(chunks);

    issue_transfer_commands(chunks);
}
