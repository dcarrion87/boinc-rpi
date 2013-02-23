/*
 * Copyright 2012, 2009 Travis Desell and the University of North Dakota.
 *
 * This file is part of the Toolkit for Asynchronous Optimization (TAO).
 *
 * TAO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TAO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TAO.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <vector>
#include <cstdlib>
#include <string>

#include "config.h"
#include "util.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "md5_file.h"
#include "error_numbers.h"
#include "validate_util.h"

#include "stdint.h"
#include "mysql.h"
#include "boinc_db.h"

#include "undvc_common/file_io.hxx"
#include "undvc_common/parse_xml.hxx"

#include <boost/algorithm/string.hpp>

using namespace std;

//returns 0 on sucess
int assimilate_handler(WORKUNIT& wu, vector<RESULT>& /*results*/, RESULT& canonical_result) {
    int retval;
    vector<OUTPUT_FILE_INFO> files;
    
    MYSQL *conn = boinc_db.mysql;

    //Parse the max_value, subset_size and starting_subset values from the workunit name
    uint32_t max_value, subset_size;
    uint64_t starting_subset;

    vector<std::string> split_string;
    boost::split(split_string, wu.name, boost::is_any_of("_"));

    max_value       = atol( split_string[2].c_str() );
    subset_size     = atol( split_string[3].c_str() );
    starting_subset = atol( split_string[4].c_str() );

    log_messages.printf(MSG_NORMAL, "parsed max_value: %u, subset_size: %u, and starting_subset %lu\n", max_value, subset_size, starting_subset);

    uint32_t id;        //get the run id from the max_value and subset size

    ostringstream query;

    query.str("");
    query.clear();
    query << "SELECT id FROM sss_runs WHERE max_value = " << max_value << " AND subset_size = " << subset_size << endl;

    log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
    mysql_query(conn, query.str().c_str());
    MYSQL_RES *result = mysql_store_result(conn);

    if (mysql_errno(conn) != 0) {
        log_messages.printf(MSG_CRITICAL, "ERROR: getting id from sss_runs: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
        exit(1);
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (mysql_errno(conn) != 0) {
        log_messages.printf(MSG_CRITICAL, "ERROR: getting id from sss_runs: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
    } else if (row == NULL) {
        log_messages.printf(MSG_CRITICAL, "ERROR: getting id from sss_runs: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
        log_messages.printf(MSG_CRITICAL, "returned NULL for rows.\n");
        exit(1);
    }

    id = atoi(row[0]);
    mysql_free_result(result);

//    cout << "id = " << id << endl;

    if (wu.error_mask > 0) {
        log_messages.printf(MSG_CRITICAL, "[RESULT#%d %s] assimilate_handler: WORKUNIT ERRORED OUT\n", canonical_result.id, canonical_result.name);

        query.str("");
        query.clear();
        query << "INSERT INTO sss_errors SET "
            << "id = " << id << ", "
            << "starting_subset = " << starting_subset;

        log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
        mysql_query(conn, query.str().c_str());

        if (mysql_errno(conn) != 0) {
            log_messages.printf(MSG_CRITICAL, "ERROR: could not insert into sss_errors with query: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
            exit(1);
        }

        query.str("");
        query.clear();
        query << "UPDATE sss_runs SET errors = errors + 1 WHERE id = " << id;

        log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
        mysql_query(conn, query.str().c_str());
        if (mysql_errno(conn) != 0) {
            log_messages.printf(MSG_CRITICAL, "ERROR: could not update sss_runs with query: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
            exit(1);
        }

    } else if (wu.canonical_resultid == 0) {
        log_messages.printf(MSG_CRITICAL, "[RESULT#%d %s] assimilate_handler: error mask not set and canonical result id == 0, should never happen\n", canonical_result.id, canonical_result.name);
        exit(1);

    } else {

        retval = get_output_file_infos(canonical_result, files);
        if (retval) {
            log_messages.printf(MSG_CRITICAL, "[RESULT#%d %s] assimilate_handler: can't get output filenames\n", canonical_result.id, canonical_result.name);
            return retval;
        }

        if (files.size() > 1) {
            log_messages.printf(MSG_CRITICAL, "[RESULT#%d %s] had more than one output file: %u\n", canonical_result.id, canonical_result.name, files.size());
            for (uint32_t i = 0; i < files.size(); i++) {
                log_messages.printf(MSG_CRITICAL, "    %s\n", files[i].name.c_str());
            }
            exit(1);
        }

        OUTPUT_FILE_INFO& fi = files[0];

        string file_contents;
        try {
            file_contents = get_file_as_string(fi.path);
        } catch (int err) {
            log_messages.printf(MSG_CRITICAL, "[RESULT#%d %s] assimilate_handler: could not open file for result\n", canonical_result.id, canonical_result.name);
            log_messages.printf(MSG_CRITICAL, "     file path: %s\n", fi.path.c_str());
            return ERR_FOPEN;
        }

        uint32_t checksum;
        vector<uint64_t> failed_sets;

        try {
            checksum = parse_xml<uint32_t>(file_contents, "checksum");
            parse_xml_vector<uint64_t>(file_contents, "failed_subsets", failed_sets);
        } catch (string err_msg) {
            log_messages.printf(MSG_CRITICAL, "Error parsing file contents:\n%s\n\n", file_contents.c_str());
            log_messages.printf(MSG_CRITICAL, "Threw exception:\n%s\n", err_msg.c_str());
            exit(1);
        }

        /**
         *  Insert each failed subset into the database so they can be used to generate a webpage of which
         *  ones failed and why later.
         */
        for (uint64_t i = 0; i < failed_sets.size(); i++) {
            query.str("");
            query.clear();
            query << "INSERT INTO sss_results SET "
                << "id = " << id << ", "
                << "failed_set = " << failed_sets[i];

            log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
            mysql_query(conn, query.str().c_str());

            if (mysql_errno(conn) != 0) {
                log_messages.printf(MSG_CRITICAL, "ERROR: could not insert into sss_results with query: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
                exit(1);
            }
        }

        /**
         *  Update the sss_runs table because another slice was completed
         */
        query.str("");
        query.clear();
        query << "UPDATE sss_runs SET completed = completed + 1, failed_set_count = failed_set_count + " << failed_sets.size() << " WHERE id = " << id;

        log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
        mysql_query(conn, query.str().c_str());
        if (mysql_errno(conn) != 0) {
            log_messages.printf(MSG_CRITICAL, "ERROR: could not update sss_runs with query: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
            exit(1);
        }
    }

    //Don't need to do anything, when the result is validated it gets inserted into the database directly
    return 0;
}
