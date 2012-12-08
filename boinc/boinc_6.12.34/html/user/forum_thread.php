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


// display the contents of a thread.

require_once('../inc/util.inc');
require_once('../inc/forum.inc');

$threadid = get_int('id');
$sort_style = get_int('sort', true);
$filter = get_str('filter', true);

if ($filter != "false"){
    $filter = true;
} else {
    $filter = false;
}

$logged_in_user = get_logged_in_user(false);
$tokens = "";
if ($logged_in_user) {
    BoincForumPrefs::lookup($logged_in_user);
    $tokens = url_tokens($logged_in_user->authenticator);
}

$thread = BoincThread::lookup_id($threadid);
$forum = BoincForum::lookup_id($thread->forum);

if (!$thread) error_page(tra("No thread with id %1. Please check the link and try again.", $threadid));

if (!is_forum_visible_to_user($forum, $logged_in_user)) {
    if ($logged_in_user) {
        remove_subscriptions_forum($logged_in_user->id, $forum->id);
    }
    error_page(tra("This forum is not visible to you."));
}

if ($thread->hidden) {
    if (!is_moderator($logged_in_user, $forum)) {
        if ($logged_in_user) {
            remove_subscriptions_forum($logged_in_user->id, $thread->id);
        }
        error_page(
            tra("This thread has been hidden by moderators")
        );
    }
}

$title = cleanup_title($thread->title);
if (!$sort_style) {
    // get the sorting style from the user or a cookie
    if ($logged_in_user){
        $sort_style = $logged_in_user->prefs->thread_sorting;
    } else if (array_key_exists('sorting', $_COOKIE)) {
        list($forum_style, $sort_style) = explode("|",$_COOKIE['sorting']);
    }
} else {
    if ($logged_in_user){
        $logged_in_user->prefs->thread_sorting = $sort_style;
        $logged_in_user->prefs->update("thread_sorting=$sort_style");
        $forum_style = 0;   // I guess this is deprecated
    } else if (array_key_exists('sorting', $_COOKIE)) {
        list($forum_style, $old_style) = explode("|", $_COOKIE['sorting']);
    }
    send_cookie('sorting',
        implode("|", array($forum_style, $sort_style)),
        true
    );
}

if ($logged_in_user && $logged_in_user->prefs->jump_to_unread){
    page_head($title, 'jumpToUnread();');
} else {
    page_head($title);
}
echo "<link href=\"forum_forum.php?id=".$forum->id."\" rel=\"up\" title=\"".$forum->title."\">";

$is_subscribed = $logged_in_user && BoincSubscription::lookup($logged_in_user->id, $thread->id);

show_forum_header($logged_in_user);

echo "<p>";
switch ($forum->parent_type) {
case 0:
    $category = BoincCategory::lookup_id($forum->category);
    show_forum_title($category, $forum, $thread);
    break;
case 1:
    show_team_forum_title($forum, $thread);
    break;
}

if ($forum->parent_type == 0) {
    if ($category->is_helpdesk && !$thread->status){
        if ($logged_in_user){
            if ($thread->owner == $logged_in_user->id){
                if ($thread->replies !=0) {
                    // Show a "this question has been answered" to the author
                    echo "<div class=\"helpdesk_note\">
                    <form action=\"forum_thread_status.php\"><input type=\"hidden\" name=\"id\" value=\"".$thread->id."\">
                    <input type=\"submit\" value=\"".tra("My question was answered")."\">
                    </form>
                    " . tra("If your question has been adequately answered please click here to close it!") . "
                    </div>";
                }
            } else {
                // and a "I also got this question" to everyone else if they havent already told so
                echo "<div class=\"helpdesk_note\">
                <form action=\"forum_thread_vote.php\"><input type=\"hidden\" name=\"id\" value=\"".$thread->id."\">
                <input type=\"submit\" value=\"".tra("I've also got this question")."\">
                </form>
                </div>";
            }
        }
    }
}

echo "
    <p>
    <form action=\"forum_thread.php\">
    <table width=\"100%\" cellspacing=0 cellpadding=0>
    <tr class=\"forum_toplinks\">
    <td><ul class=\"actionlist\">
";

$reply_url = "";
if (can_reply($thread, $forum, $logged_in_user)) {        
    $reply_url = "forum_reply.php?thread=".$thread->id."#input";
    show_actionlist_button($reply_url, tra("Post to thread"), tra("Add a new message to this thread"));
}

if ($is_subscribed) {
    $type = NOTIFY_SUBSCRIBED_POST;
    BoincNotify::delete_aux("userid=$logged_in_user->id and type=$type and opaque=$thread->id");
    $url = "forum_subscribe.php?action=unsubscribe&thread=".$thread->id."$tokens";
    show_actionlist_button($url, tra("Unsubscribe"), tra("You are subscribed to this thread.  Click here to unsubscribe."));
} else {
    $url = "forum_subscribe.php?action=subscribe&thread=".$thread->id."$tokens";
    show_actionlist_button($url, tra("Subscribe"), tra("Click to get email when there are new posts in this thread"));
}

//If the logged in user is moderator enable some extra features
//
if (is_moderator($logged_in_user, $forum)) {
    if ($thread->hidden){
        show_actionlist_button("forum_moderate_thread_action.php?action=unhide&thread=".$thread->id."$tokens", tra("Unhide"), tra("Unhide this thread"));
    } else {
        show_actionlist_button("forum_moderate_thread.php?action=hide&thread=".$thread->id, tra("Hide"), tra("Hide this thread"));
    }
    if ($thread->sticky){
        show_actionlist_button("forum_moderate_thread_action.php?action=desticky&thread=".$thread->id."$tokens", tra("Make unsticky"), tra("Make this thread not sticky"));
    } else {
        show_actionlist_button("forum_moderate_thread_action.php?action=sticky&thread=".$thread->id."$tokens", tra("Make sticky"), tra("Make this thread sticky"));
    }
    if ($thread->locked) {
        show_actionlist_button("forum_moderate_thread_action.php?action=unlock&amp;thread=".$thread->id."$tokens", tra("Unlock"), tra("Unlock this thread"));
    } else {
        show_actionlist_button("forum_moderate_thread.php?action=lock&thread=".$thread->id."$tokens", tra("Lock"), tra("Lock this thread"));
    }
    if ($forum->parent_type == 0) {
        show_actionlist_button("forum_moderate_thread.php?action=move&thread=".$thread->id."$tokens", tra("Move"), tra("Move this thread to a different forum"));
    }
    show_actionlist_button("forum_moderate_thread.php?action=title&thread=".$thread->id."$tokens", tra("Edit title"), tra("Edit thread title"));
}
echo "</ul>"; //End of action list

// Display a box that allows the user to select sorting of the posts
echo "</td><td align=\"right\">
    <input type=\"hidden\" name=\"id\" value=\"", $thread->id, "\">" .
    tra("Sort");
echo select_from_array("sort", $thread_sort_styles, $sort_style);
echo "<input type=\"submit\" value=\"Sort\">
    </td></tr></table></form>
";

// Here is where the actual thread begins.
$headings = array(array(tra("Author"),"authorcol"), array(tra("Message"),""));

start_forum_table($headings, "id=\"thread\" width=100%");
show_posts($thread, $forum, $sort_style, $filter, $logged_in_user, true);
end_table();

if ($reply_url) {
    show_button($reply_url, tra("Post to thread"), tra("Add a new message to this thread"));
}

echo "<p>";
switch ($forum->parent_type) {
case 0:
    show_forum_title($category, $forum, $thread);
    break;
case 1:
    show_team_forum_title($forum, $thread);
    break;
}

$thread->update("views=views+1");

page_tail();
$cvs_version_tracker[]="\$Id: forum_thread.php 21003 2010-03-26 05:34:14Z boincadm $";
?>
