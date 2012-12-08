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



// search for posts or a thread.
// Takes input from forum_search.php
 
require_once('../inc/time.inc');
require_once('../inc/text_transform.inc');
require_once('../inc/forum.inc');

// Searches for the keywords in the $keyword_list array in thread titles.
// Optionally filters by forum, user, time, or hidden if specified.
//
function search_thread_titles(
    $keyword_list, $forum="", $user="", $time="", $limit=200,
    $sort_style=CREATE_TIME_NEW, $show_hidden = false
){
    $search_string="%";
    foreach ($keyword_list as $key => $word) {
        $search_string.=mysql_escape_string($word)."%";
    }        
    $query = "title like '".$search_string."'";
    if ($forum && $forum != "all") {
        $query .= " and forum = $forum->id";
    }
    if ($user && $user != "all") {
        $query .= " and owner = $user->id";
    }
    if ($time && $user != "all") {
        $query .= " and timestamp > $time";
    }
    if (!$show_hidden) {
        $query .= " and thread.hidden = 0";
    }
    switch($sort_style) {
    case MODIFIED_NEW:
        $query .= ' ORDER BY timestamp DESC';
        break;
    case VIEWS_MOST:
        $query .= ' ORDER BY views DESC';
        break;
    case REPLIES_MOST:
        $query .= ' ORDER BY replies DESC';
        break;
    case CREATE_TIME_NEW:
        $query .= ' ORDER by create_time desc';
        break;
    case CREATE_TIME_OLD:
        $query .= ' ORDER by create_time asc';
        break;
    case 'score':
        $query .= ' ORDER by score desc';
        break;
    default:
        $query .= ' ORDER BY timestamp DESC';
        break;
    }

    $query .= " limit $limit";
    return BoincThread::enum($query);
}

// Searches for the keywords in the $keyword_list array in post bodies.
// optionally filters by forum, time, hidden, or user if specified.
//
function search_post_content(
    $keyword_list, $forum, $user, $time, $limit, $sort_style, $show_hidden
){
    $search_string="%";
    foreach ($keyword_list as $key => $word){
        $search_string.=mysql_escape_string($word)."%";
    }
    $optional_join = "";
    // if looking in a single forum, need to join w/ thread table
    // because that's where the link to forum is
    //
    if ($forum) {
        $optional_join = " LEFT JOIN DBNAME.thread ON post.thread = thread.id";
    }
    $query = "select post.* from DBNAME.post".$optional_join." where content like '".$search_string."'";
    if ($forum) {
        $query.=" and forum = $forum->id";
    }
    if ($user) {
        $query.=" and post.user = $user->id ";
    }
    if ($time) {
        $query.=" and post.timestamp > $time";
    }
    if (!$show_hidden) {
        $query .= " AND post.hidden = 0";
    }
    switch($sort_style) {
    case VIEWS_MOST:
        $query.= ' ORDER BY views DESC';
        break;
    case CREATE_TIME_NEW:
        $query .= ' ORDER by post.timestamp desc';
        break;
    case CREATE_TIME_OLD:
        $query .= ' ORDER by post.timestamp asc';
        break;
    case POST_SCORE:
        $query .= ' ORDER by post.score desc';
        break;
    default:
        $query .= ' ORDER BY post.timestamp DESC';
        break;
    }
    $query.= " limit $limit";
    return BoincPost::enum_general($query);
}

$logged_in_user = get_logged_in_user(false);
BoincForumPrefs::lookup($logged_in_user);
if ($logged_in_user && $logged_in_user->prefs->privilege(S_MODERATOR)){
    $show_hidden_posts = true;
} else {
    $show_hidden_posts = false;
}

page_head(tra("Forum search results"));

$search_keywords = post_str("search_keywords", true);
$search_author = post_int("search_author", true);
$search_max_time = post_int("search_max_time");
$search_forum = post_int("search_forum");
$search_sort = post_int("search_sort");
$search_list = explode(" ",$search_keywords);
$min_timestamp = time() - ($search_max_time*3600*24);
$limit = 100;

if ($search_forum==-1){
    $forum = null;
} else if ($search_forum) {
    $forum = BoincForum::lookup_id($search_forum);
}

$user = null;
if ($search_author) {
    $user = BoincUser::lookup_id($search_author);
}

// First search thread titles; if we get a hit there it's probably relevant
//
$threads = search_thread_titles($search_list, $forum, $user, $min_timestamp, round($limit/7), $search_sort, $show_hidden_posts);

// Display the threads while we search for posts
if (count($threads)){
    echo "<span class=title>Thread titles matching your query:</span>";
    show_thread_and_context_header();
    $i = 0;
    foreach ($threads as $thread){
        if ($thread->hidden) continue;
        show_thread_and_context($thread, $logged_in_user, $i++);
    }
    end_table();
    echo "<br /><br />";
}


// Look in a post content as well
//
$posts = search_post_content(
    $search_list, $forum, $user, $min_timestamp, $limit, $search_sort,
    $show_hidden_posts
);

if (count($posts)){
    echo "<span class=title>Messages matching your query:</span>";
    start_table();
    $n = 1;
    $options = get_output_options($logged_in_user);
    $options->setHighlightTerms($search_list);
    foreach ($posts as $post) {
        $thread = BoincThread::lookup_id($post->thread);
        if (!$thread) continue;
        $forum = BoincForum::lookup_id($thread->forum);
        if (!$forum) continue;
        if (!is_forum_visible_to_user($forum, $logged_in_user)) continue;

        if (!$show_hidden_posts) {
            if ($thread->hidden) continue;
            if ($post->hidden) continue;
        }
        show_post_and_context($post, $thread, $forum, $options, $n);
        $n++;
    }
    end_table();
}

if (!count($thread) && !count($posts)){
    echo "<p>Sorry, couldn't find anything matching your search query. You 
    can try to broaden your search by using less words (or less specific words).</p>
    <p>You can also 
    <a href=\"http://www.google.com/search?domains=".URL_BASE."&sitesearch=".URL_BASE."/forum_thread.php&q=".htmlentities($search_keywords)."\">
    try the same search on Google.</a></p>";
}
echo "<p><a href=\"forum_search.php\">Perform another search</a></p>";
page_tail();
exit;

$cvs_version_tracker[]="\$Id: forum_search_action.php 15982 2008-09-09 14:59:50Z Rytis $";  //Generated automatically - do not edit
?>
