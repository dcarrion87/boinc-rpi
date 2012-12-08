<?php

require_once("../inc/util_ops.inc");

$user = get_logged_in_user_ops();
if ($user) {
    clear_cookie('auth', true);
    admin_page_head("Logged out");
    admin_page_tail();
} else {
    admin_error_page("not logged in");
}

?>
