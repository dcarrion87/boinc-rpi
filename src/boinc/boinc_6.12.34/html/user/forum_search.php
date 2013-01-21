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


// Allows users to search for a post or thread.
// Sends to forum_search_action.php for action and display.

require_once('../inc/forum.inc');

page_head(tra("Forum search"));

start_table();
echo "<form action=\"forum_search_action.php\" method=\"post\">";
row1("Search query");
row2("Search for keywords:<br />
    <font size=-1>Posts that contain all the specified words will be displayed</font>",
    '<input type="text" style="width: 290px" name="search_keywords" size="30" /><br />
    <font size=-1>For example: "screensaver freeze"</font>');
row2("Search for author ID:<br />
    <font size=-1>Only posts by this author will be displayed</font>",
    '<input type="text" style="width: 150px" name="search_author" size="10" /><br />
    <font size=-1>For example: "43214"</font>');

row1("Search options");
row2("Search limits<br />
    <font size=-1>Search at most this many days back in time</font>",
    '<select name="search_max_time">
    <option value="1">1 day</option>
    <option value="3">3 days</option>
    <option value="7">7 days</option>
    <option value="15">15 days</option>
    <option value="30" selected>30 days</option>
    <option value="90">3 months</option>
    <option value="180">6 months</option>
    <option value="360">1 year</option>
    </select>');

$forumid = null;
if (get_str("forumid",true)){
    $forumid = get_str("forumid");
}
$forumlist="<option value=\"-1\">All</option>";
$categories = BoincCategory::enum();
foreach ($categories as $category) {
    $forums = BoincForum::enum("parent_type=0 and category=$category->id");
    foreach ($forums as $forum) {
        if ($forum->id==$forumid){
            $forumlist.="<option selected value=\"".$forum->id."\">".$forum->title."</option>";
        } else {
            $forumlist.="<option value=\"".$forum->id."\">".$forum->title."</option>";
        }
    }
}
row2("Forum<br />
    <font size=-1>Only display posts from this forum</font>",
    '<select name="search_forum">'.$forumlist.'</select');

$sortlist = null;
foreach ($thread_sort_styles as $id => $style){
    if ($id == CREATE_TIME_NEW){
        $sortlist.="<option selected value=\"".$id."\">".$style."</option>";
    } else {
        $sortlist.="<option value=\"".$id."\">".$style."</option>";
    }
}
row2("Sort by", 
    '<select name="search_sort">'.$sortlist.'</select');

row1("&nbsp;");
row2("","<input type=\"submit\" value=\"Start the search\">");
echo "</form>";
end_table();

page_tail();
exit;

$cvs_version_tracker[]="\$Id: forum_search.php 15758 2008-08-05 22:43:14Z davea $";  //Generated automatically - do not edit
?>
