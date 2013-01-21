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

// This file allows people to rate posts in a thread

require_once('../inc/forum.inc');
require_once('../inc/util.inc');

$config = get_config();
if (parse_bool($config, "no_forum_rating")) {
    page_head("Rating offline");
    echo "This function is turned off by the project";
    page_tail();
    exit(0);
}

if (!empty($_GET['post'])) {
    $postId = get_int('post');
    $choice = post_str('submit', true);
    $rating = post_int('rating', true);
    if (!$choice) $choice = get_str('choice', true);
    
    if ($choice == SOLUTION or $choice=="p") {
        $rating = 1;
    } else {
        $rating = -1;
    }

    $user = get_logged_in_user();

    if ($choice == null && ($rating == null || $rating > 2 || $rating < -2)) {
        show_result_page(false, NULL, NULL, $choice);
    }

    $post = BoincPost::lookup_id($postId);
    $thread = BoincThread::lookup_id($post->thread);
    $forum = BoincForum::lookup_id($thread->forum);

    // Make sure the user has the forum's minimum amount of RAC and total credit
    // before allowing them to rate a post.
    //
    if ($user->total_credit<$forum->rate_min_total_credit || $user->expavg_credit<$forum->rate_min_expavg_credit) {
        error_page("You need more average or total credit to rate a post.");
    }
    
    if (BoincPostRating::lookup($user->id, $post->id)) {
        error_page("You have already rated this post once.<br /><br /><a href=\"forum_thread.php?nowrap=true&id=".$thread->id."#".$post->id."\">Return to thread</a>");
    } else {
        $success = BoincPostRating::replace($user->id, $post->id, $rating);
        show_result_page($success, $post, $thread, $choice);
    }
}

function show_result_page($success, $post, $thread, $choice) {
    if ($success) {
        if ($choice) {
            page_head('Input Recorded');
                echo "<p>Your input has been successfully recorded.  Thank you for your help.</p>";
        } else {
            page_head('Vote Registered');
        echo "<span class=\"title\">Vote Registered</span>";
        echo "<p>Your rating has been successfully recorded.  Thank you for your input.</p>";
        }
        echo "<a href=\"forum_thread.php?nowrap=true&id=", $thread->id, "#", $post->id, "\">Return to thread</a>";
    } else {
        page_head('Vote Submission Problem');    
        echo "<span class=\"title\">Vote submission failed</span>";
        if ($post) {
            echo "<p>There was a problem recording your vote in our database.  Please try again later.</p>";
            echo "<a href=\"forum_thread.php?id=", $thread->id, "#", $post->id, "\">Return to thread</a>";
        } else {
            echo "<p>There post you specified does not exist, or your rating was invalid.</p>";
        }
    }
    page_tail();
    exit;
}

$cvs_version_tracker[]="\$Id: forum_rate.php 15758 2008-08-05 22:43:14Z davea $";  //Generated automatically - do not edit
?>
