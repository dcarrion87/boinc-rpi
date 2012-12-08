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

// Let the founder assign foundership to someone else,
// or decline a foundership change request

require_once("../inc/util.inc");
require_once("../inc/team.inc");
require_once("../inc/boinc_db.inc");

$user = get_logged_in_user();

$teamid = get_int("teamid");
$team = BoincTeam::lookup_id($teamid);
if (!$team) {
    error_page(tra("no such team"));
}
require_founder_login($user, $team);

page_head(tra("Change founder of %1", $team->name));

if ($team->ping_user != 0) {
    if ($team->ping_user < 0) {
        $ping_user = BoincUser::lookup_id(-$team->ping_user);
        $x = date_str($team->ping_time);
        echo "<p>".tra("Team member %1 requested this team's foundership on %2, but left the team, thus canceling the request.", user_links($ping_user), $x)
        ."</p>";
        $team->update("ping_user=0, ping_time=0");
    } else {
        $ping_user = BoincUser::lookup_id($team->ping_user);
        $x = date_str(transfer_ok_time($team));
        echo "<p>".tra("Team member %1 has requested this team's foundership. This may be because you left the team or haven't had contact with the team for a long time.", user_links($ping_user))
        ."</p>";
        echo "<p>
            <form method=\"post\" action=\"team_founder_transfer_action.php\">
            <input type=\"hidden\" name=\"action\" value=\"decline\">
            <input type=\"hidden\" name=\"teamid\" value=\"".$team->id."\">
            <input type=\"submit\" value=\"".tra("decline request")."\">
            </form>
            <p>
            ".tra("If you don't decline the request by %1, %2 will have the option of assuming team foundership.<br /><br />
                  To accept the request, assign foundership to %3 using the form below.", $x, $ping_user->name, $ping_user->name)."
            </p>
            <hr>
            <p>
        ";
    }
} else {
    echo tra("No transfer request is pending.")."<br /><br />";
}

echo tra("To assign foundership of this team to another member, check the box next to member name and click <strong>Change founder</strong> below.")
    ."<form method=post action=team_change_founder_action.php>
    <input type=hidden name=teamid value=$team->id>";
echo form_tokens($user->authenticator);

start_table();
echo "<tr>
    <th>".tra("New founder?")."</th>
    <th>".tra("Name")."</th>
    <th>".tra("Total credit")."</th>
    <th>".tra("Recent average credit")."</th>
    </tr>
";

$users = BoincUser::enum("teamid=$team->id");

$navailable_users = 0;
foreach ($users as $user) {
    if ($user->id != $team->userid) {       //don't show current founder
        $user_total_credit = format_credit($user->total_credit);
        $user_expavg_credit = format_credit($user->expavg_credit);
        $selected = ($user->id == $team->ping_user)?"selected":"";
        echo '
            <tr>
            <td align="center"><input type="radio" name="userid" value="'.$user->id.'">
            <td>'.$user->name.'</td>
            <td align=right>'.$user_total_credit.'</td>
            <td align=right>'.$user_expavg_credit.'</td>
            </tr>
        ';
        $navailable_users++;
    }
}
if ($navailable_users > 0) {
    echo "<input type=hidden name=navailable_users value=$navailable_users>";
    end_table();
    echo "<input type=submit value=\"".tra("Change founder")."\">";
} else {
    echo '<tr>
        <td colspan="4">".tra("There are no users to transfer team to.")."</td>
        </tr>
    ';
    end_table();
}
echo "</form>";
page_tail();

$cvs_version_tracker[]="\$Id: team_change_founder_form.php 20582 2010-02-16 01:06:03Z davea $";  //Generated automatically - do not edit
?>
