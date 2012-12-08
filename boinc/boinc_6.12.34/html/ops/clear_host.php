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
$hostid = $_GET["hostid"];

if (!$hostid) {
    error_page("no host ID\n");
}

mysql_query("update host set rpc_time=0 where id='$hostid'");
echo "Host RPC time cleared for host ID: $hostid\n";

admin_page_tail();
$cvs_version_tracker[]="\$Id: clear_host.php 20745 2010-02-26 21:34:20Z davea $";  //Generated automatically - do not edit
?>
