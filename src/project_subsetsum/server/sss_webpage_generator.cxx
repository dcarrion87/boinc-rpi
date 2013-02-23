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

// sample_work_generator.cpp: an example BOINC work generator.
// This work generator has the following properties
// (you may need to change some or all of these):
//
// - Runs as a daemon, and creates an unbounded supply of work.
//   It attempts to maintain a "cushion" of 100 unsent job instances.
//   (your app may not work this way; e.g. you might create work in batches)
// - Creates work for the application "example_app".
// - Creates a new input file for each job;
//   the file (and the workunit names) contain a timestamp
//   and sequence number, so they're unique.

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cmath>

#include "boinc_db.h"
#include "error_numbers.h"
#include "backend_lib.h"
#include "parse.h"
#include "util.h"
#include "svn_version.h"

#include "sched_config.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "str_util.h"

#include "mysql.h"

#include "../common/bit_logic.hpp"
#include "../common/generate_subsets.hpp"
#include "../common/binary_output.hpp"
#include "../common/n_choose_k.hpp"

const char* app_name = "subset_sum";

char* in_template;
DB_APP app;
int start_time;
int seqno;

double max_digits;                  //extern
double max_set_digits;              //extern

unsigned long int max_sums_length;  //extern
uint32_t *sums;                 //extern
uint32_t *new_sums;             //extern


using namespace std;

void main_loop(MYSQL *conn) {
    /**
     *  Get max_set_value and subset_size from sss_progress table
     */
    uint32_t id, max_set_value, subset_size;
    uint64_t slices, failed_set_count, failed_set;

    while (1) {
        check_stop_daemons();

        /**
         *  Get the completed runs from the database
         */
        ostringstream query;
        query << "SELECT id, max_value, subset_size, slices, failed_set_count FROM sss_runs WHERE sss_runs.completed = sss_runs.slices AND webpage_generated = FALSE";

        log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
        mysql_query(conn, query.str().c_str());
        MYSQL_RES *result = mysql_store_result(conn);

        if (mysql_errno(conn) != 0) {
            log_messages.printf(MSG_CRITICAL, "ERROR: getting completed sss_runs: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
            exit(1);
        }   

        /**
         *  Generate a webpage for each completed run.
         */
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != NULL) {
//            && mysql_errno(conn) != 0) {
            log_messages.printf(MSG_DEBUG, "inside first while loop\n");
            id = atoi(row[0]);
            max_set_value = atoi(row[1]);
            subset_size = atoi(row[2]);
            slices = atol(row[3]);
            failed_set_count = atol(row[4]);

            query.str("");
            query.clear();
            query << "SELECT failed_set FROM sss_results WHERE id = " << id << " ORDER BY failed_set";

            log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
            mysql_query(conn, query.str().c_str());
            MYSQL_RES *failed_sets_result = mysql_store_result(conn);

            if (mysql_errno(conn) != 0) {
                log_messages.printf(MSG_CRITICAL, "ERROR: getting failed sets: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
                exit(1);
            }   


            /**
             *  Calculate the maximum set length (in bits) and some other values so we can use this for printing out the values cleanly.
             */
            uint32_t *max_set = new uint32_t[subset_size];
            for (uint32_t i = 0; i < subset_size; i++) max_set[subset_size - i - 1] = max_set_value - i;
            for (uint32_t i = 0; i < subset_size; i++) max_sums_length += max_set[i];

            //    sums_length /= 2;
            max_sums_length /= ELEMENT_SIZE;
            max_sums_length++;

            delete [] max_set;

            uint64_t expected_total = n_choose_k(max_set_value - 1, subset_size - 1);
            max_digits = ceil(log10(expected_total));

            sums = new uint32_t[max_sums_length];
            new_sums = new uint32_t[max_sums_length];
            uint32_t *subset = new uint32_t[subset_size];


            /**
             *  Open the webpage to write to
             */
            ostringstream webpage_name;
            webpage_name << "/projects/subset_sum/download/set_" << max_set_value << "c" << subset_size << ".html";
            ofstream *output_target = new ofstream(webpage_name.str().c_str());

            log_messages.printf(MSG_DEBUG, "writing to file: '%s'\n", webpage_name.str().c_str());

            /**
             * Make the HTML header
             */
            *output_target << "<!DOCTYPE html PUBLIC \"-//w3c//dtd html 4.0 transitional//en\">" << endl;
            *output_target << "<html>" << endl;
            *output_target << "<head>" << endl;
            *output_target << "  <meta http-equiv=\"Content-Type\"" << endl;
            *output_target << " content=\"text/html; charset=iso-8859-1\">" << endl;
            *output_target << "  <meta name=\"GENERATOR\"" << endl;
            *output_target << " content=\"Mozilla/4.76 [en] (X11; U; Linux 2.4.2-2 i686) [Netscape]\">" << endl;
            *output_target << "  <title>" << max_set_value << " choose " << subset_size << "</title>" << endl;
            *output_target << "" << endl;
            *output_target << "<style type=\"text/css\">" << endl;
            *output_target << "    .courier_green {" << endl;
            *output_target << "        color: #008000;" << endl;
            *output_target << "    }   " << endl;
            *output_target << "</style>" << endl;
            *output_target << "<style type=\"text/css\">" << endl;
            *output_target << "    .courier_red {" << endl;
            *output_target << "        color: #FF0000;" << endl;
            *output_target << "    }   " << endl;
            *output_target << "</style>" << endl;
            *output_target << "" << endl;
            *output_target << "</head><body>" << endl;
            *output_target << "<h1>" << max_set_value << " choose " << subset_size << "</h1>" << endl;
            *output_target << "<hr width=\"100%%\">" << endl;
            *output_target << "" << endl;
            *output_target << "<br>" << endl;
            *output_target << "<tt>" << endl;

            *output_target << "max_set_value: " << max_set_value << ", subset_size: " << subset_size << "<br>" << endl;

            MYSQL_ROW failed_sets_row;
            uint64_t fail = 0;
            while ((failed_sets_row = mysql_fetch_row(failed_sets_result)) != NULL) {
                failed_set = atol(failed_sets_row[0]);

                /**
                 *  Write the line for that failed set to the html file.
                 */
                generate_ith_subset(failed_set, subset, subset_size, max_set_value);
                print_subset_calculation(output_target, failed_set, subset, subset_size, false);
                fail++;
            }
            uint64_t pass = expected_total - fail;

            if (mysql_errno(conn) != 0) {
                log_messages.printf(MSG_CRITICAL, "ERROR: getting failed_sets from sss_results row: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
                exit(1);
            }   

            delete subset;
            delete sums;
            delete new_sums;
            mysql_free_result(failed_sets_result);

            *output_target << expected_total << " total sets, " << pass << " sets passed, " << fail << " sets failed, " << ((double)pass/ ((double)pass + (double)fail)) << " success rate." << endl;
            *output_target << "</tt>" << endl;
            *output_target << "<br>" << endl << endl;
            *output_target << "<hr width=\"100%%\">" << endl;
            *output_target << "Copyright &copy; Travis Desell, Tom O'Neil and the University of North Dakota, 2012" << endl;
            *output_target << "</body>" << endl;
            *output_target << "</html>" << endl;

            output_target->flush();
            output_target->close();

//            if (fail > 0) {
//                cerr << "[url=http://volunteer.cs.und.edu/subset_sum/download/set_" << max_set_value << "c" << subset_size << ".html]" << max_set_value << " choose " << subset_size << "[/url] -- " << fail << " failures" << endl;
//            } else {
//                cerr << "[url=http://volunteer.cs.und.edu/subset_sum/download/set_" << max_set_value << "c" << subset_size << ".html]" << max_set_value << " choose " << subset_size << "[/url] -- pass" << endl;
//            }

            /**
             *  Update the sss_run setting the webpage_generated column to true.
             */
            query.str("");
            query.clear();
            query << "UPDATE sss_runs SET webpage_generated = true WHERE id = " << id;

            log_messages.printf(MSG_NORMAL, "%s\n", query.str().c_str());
            mysql_query(conn, query.str().c_str()); 

            if (mysql_errno(conn) != 0) {
                log_messages.printf(MSG_CRITICAL, "ERROR: could not update sss_runs with query: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
                exit(1);
            }   
        }

        if (mysql_errno(conn) != 0) {
            log_messages.printf(MSG_CRITICAL, "ERROR: getting completed sss_runs row: '%s'. Error: %d -- '%s'. Thrown on %s:%d\n", query.str().c_str(), mysql_errno(conn), mysql_error(conn), __FILE__, __LINE__);
            exit(1);
        }   

        mysql_free_result(result);

        sleep(30);
    }
}


void usage(char *name) {
    fprintf(stderr, "This generates the webpages for the subset_sum progress report.\n"
        "Usage: %s [OPTION]...\n\n"
        "Options:\n"
        "  --app X                      Application name (default: example_app)\n"
        "  [ -d X ]                     Sets debug level to X.\n"
        "  [ -h | --help ]              Shows this help text.\n"
        "  [ -v | --version ]           Shows version information.\n",
        name
    );
}

int main(int argc, char** argv) {
    int i, retval;
    char buf[256];

//Aaron Comment: command line flags are explained in descriptions above.
    for (i=1; i<argc; i++) {
        if (is_arg(argv[i], "d")) {
            if (!argv[++i]) {
                log_messages.printf(MSG_CRITICAL, "%s requires an argument\n\n", argv[--i]);
                usage(argv[0]);
                exit(1);
            }
            int dl = atoi(argv[i]);
            log_messages.set_debug_level(dl);
            if (dl == 4) g_print_queries = true;
        } else if (!strcmp(argv[i], "--app")) {
            app_name = argv[++i];
        } else if (is_arg(argv[i], "h") || is_arg(argv[i], "help")) {
            usage(argv[0]);
            exit(0);
        } else if (is_arg(argv[i], "v") || is_arg(argv[i], "version")) {
            printf("%s\n", SVN_VERSION);
            exit(0);

        } else {
            log_messages.printf(MSG_CRITICAL, "unknown command line argument: %s\n\n", argv[i]);
            usage(argv[0]);
            exit(1);
        }
    }

//Aaron Comment: if at any time the retval value is greater than 0, then the program
//has failed in some manner, and the program then exits.

//Aaron Comment: processing project's config file.
    retval = config.parse_file();
    if (retval) {
        log_messages.printf(MSG_CRITICAL,
            "Can't parse config.xml: %s\n", boincerror(retval)
        );
        exit(1);
    }

//Aaron Comment: opening connection to database.
    retval = boinc_db.open(
        config.db_name, config.db_host, config.db_user, config.db_passwd
    );
    if (retval) {
        log_messages.printf(MSG_CRITICAL, "can't open db\n");
        exit(1);
    }

//Aaron Comment: looks for applicaiton to be run. If not found, program exits.
    sprintf(buf, "where name='%s'", app_name);
    if (app.lookup(buf)) {
        log_messages.printf(MSG_CRITICAL, "can't find app %s\n", app_name);
        exit(1);
    }

//Aaron Comment: if work generator passes all startup tests, the main work gneration
//loop is called.
    start_time = time(0);
    seqno = 0;

    log_messages.printf(MSG_NORMAL, "Starting\n");

    main_loop(boinc_db.mysql);
}
