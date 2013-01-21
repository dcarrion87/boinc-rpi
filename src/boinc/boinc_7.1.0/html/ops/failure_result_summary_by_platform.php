<?php
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

require_once("../inc/util_ops.inc");

db_init();
admin_page_head("Failure summary by (app version, error)");

$query_appid = $_GET['appid'];
$query_received_time = time() - $_GET['nsecs'];

$q = new SqlQueryString();
$q->process_form_items();

$main_query = "
SELECT
    app_version_id,
    app_version.plan_class,
    case
        when INSTR(host.os_name, 'Darwin') then 'Darwin'
        when INSTR(host.os_name, 'Linux') then 'Linux'
        when INSTR(host.os_name, 'Windows') then 'Windows'
        when INSTR(host.os_name, 'SunOS') then 'SunOS'
        when INSTR(host.os_name, 'Solaris') then 'Solaris'
        when INSTR(host.os_name, 'Mac') then 'Mac'
        else 'Unknown'
    end AS OS_Name,
    exit_status,
    COUNT(*) AS error_count
FROM   result
        left join host on result.hostid = host.id
        left join app_version on result.app_version_id = app_version.id
WHERE
    result.appid = '$query_appid' and
    server_state = '5' and
    outcome = '3' and
    received_time > '$query_received_time'
GROUP BY
    app_version_id,
    exit_status
order by error_count desc
";

$urlquery = $q->urlquery;
$result = mysql_query($main_query);

start_table();
table_header(
    "App version", "Exit Status", "Error Count"
);

while ($res = mysql_fetch_object($result)) {
    $exit_status_condition = "exit_status=$res->exit_status";
    $av = BoincAppVersion::lookup_id($res->app_version_id);
    $p = BoincPlatform::lookup_id($av->platformid);
    table_row(
        sprintf("%.2f", $av->version_num/100)." $p->name [$av->plan_class]",
        link_results(
            exit_status_string($res), $urlquery, "$exit_status_condition", ""
        ),
        $res->error_count
    );
}
mysql_free_result($result);

end_table();

admin_page_tail();

$cvs_version_tracker[]="\$Id$";  //Generated automatically - do not edit
?>
