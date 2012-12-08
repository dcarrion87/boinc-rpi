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

require_once("../inc/db.inc");
require_once("../inc/util.inc");
require_once("../inc/email.inc");

check_get_args(array());

db_init();
$user = get_logged_in_user();

page_head(tra("Change email address"));

$email_text = "";
if (is_valid_email_addr($user->email_addr)) {
    $email_text = $user->email_addr;
}

echo "<form method=post action=edit_email_action.php>\n";
start_table();
row1(tra("Change the email address of your account"));
row2(tra("New email address").
    "<br><span class=note>".tra("Must be a valid address of the form 'name@domain'")."</span>",
    "<input name=email_addr size=50 type=text value='$email_text'>"
);

// we need the password here not for verification,
// but because we store it salted with email address,
// which is about to change.

row2(
    tra("Password").
    "<br><a href=".SECURE_URL_BASE."/edit_passwd_form.php><span class=note>".tra("No password?")."</span></a>",
    "<input type=password name=passwd>"
);
row2("", "<input type=submit value='".tra("Change email address")."'>");
end_table();
echo "</form>\n";
page_tail();

?>
