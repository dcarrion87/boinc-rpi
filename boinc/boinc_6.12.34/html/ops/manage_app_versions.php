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



/***********************************************************************\
 *  Display and Manage BOINC Application Versions
 * 
 * This page presents a form with information about application versions.
 * Some of the fields can be changed.   An application version can be deleted
 * by entering the word "DELETE" (all caps required) in the provided field.   
 * It is better to deprecate a version first than to delete it, but it is also 
 * good to remove old versions after they have been unused for a while,
 * lest you over-fill the feeder (which results in new versions not being
 * used by clients).
 *
 * Eric Myers <myers@spy-hill.net>  - 4 June 2006
 * @(#) $Id: manage_app_versions.php 20745 2010-02-26 21:34:20Z davea $
\***********************************************************************/

// TODO: rewrite this using the new DB interface

require_once('../inc/util_ops.inc');

db_init();

// Platform and application labels (are better than numbers)

$result = mysql_query("SELECT * FROM platform");
$Nplatform =  mysql_num_rows($result);
for($i=0;$i<=$Nplatform;$i++){
    $item=mysql_fetch_object($result);
    $id=$item->id;
    $plat_off[$id]=$item->deprecated; 
    $platform[$id]=$item->user_friendly_name;
}
mysql_free_result($result);


$result = mysql_query("SELECT * FROM app");
$Napp =  mysql_num_rows($result);
for($i=0;$i<=$Napp;$i++){
    $item=mysql_fetch_object($result);
    $id=$item->id;
    $app_off[$id]=$item->deprecated; 
    $app[$id]=$item->name;
}
mysql_free_result($result);

$commands="";

/***************************************************\
 *  Action: process form input for changes
 \***************************************************/

if( !empty($_POST) ) {

    $result = mysql_query("SELECT * FROM app_version");
    $Nrow=mysql_num_rows($result);

    for($j=1;$j<=$Nrow;$j++){  // test/update each row in DB
        $item=mysql_fetch_object($result);
        $id=$item->id;

        /* Delete this entry? */
        $field="delete_".$id; 
        if ($_POST[$field]=='DELETE' ) {
            $cmd =  "DELETE FROM app_version WHERE id=$id";
            $commands .= "<P><pre>$cmd</pre>\n";
            mysql_query($cmd);
            continue;  // next row, this one is gone
        }

        /* Change deprecated status? */
        $field="deprecated_".$id;
        $new_v= ($_POST[$field]=='on') ? 1 : 0;
        $old_v=$item->deprecated;
        if ($new_v != $old_v ) {
            $cmd =  "UPDATE app_version SET deprecated=$new_v WHERE id=$id";
            $commands .= "<P><pre>$cmd</pre>\n";
            mysql_query($cmd);
        }

        /* Minimum core version limit */
        $field="min_core_version_".$id;
        $new_v= $_POST[$field];
        $old_v=$item->min_core_version;
        if ($new_v != $old_v ) {
            $cmd =  "UPDATE app_version SET min_core_version=$new_v WHERE id=$id";
            $commands .= "<P><pre>$cmd</pre>\n";
            mysql_query($cmd);
        }

        /* Maximum core version limit */
        $field="max_core_version_".$id;
        $new_v= $_POST[$field];
        $old_v=$item->max_core_version;
        if($new_v != $old_v ) {
            $cmd =  "UPDATE app_version SET max_core_version=$new_v WHERE id=$id";
            $commands .= "<P><pre>$cmd</pre>\n";
            mysql_query($cmd);
        }
    }
    mysql_free_result($result);
    touch("../../reread_db");
}


/***************************************************\
 * Display the DB contents in a form
 \***************************************************/

admin_page_head("Manage Application Versions");

if($commands) echo $commands;   // show the last DB commands given

$self=$_SERVER['PHP_SELF'];
echo "<form action='$self' method='POST'>\n";


// Application Version table:

start_table("align='center'");

echo "<TR><TH>ID #</TH>
      <TH>Application</TH>
      <TH>Version</TH>
      <TH>Platform</TH>
      <TH>Plan Class</TH>
      <TH>minimum<br>core version</TH>
      <TH>maximum<br>core version</TH>
      <TH>deprecated?</TH>
      <TH>DELETE?<sup>*</sup>
    </TH>
       </TR>\n";

$q="SELECT * FROM app_version ORDER BY appid, version_num, platformid";
$result = mysql_query($q);
$Nrow=mysql_num_rows($result);
for($j=1;$j<=$Nrow;$j++){
    $item=mysql_fetch_object($result);
    $id=$item->id;

    // grey-out deprecated versions 
    $f1=$f2='';
    if($item->deprecated==1) {
        $f1="<font color='GREY'>";
        $f2="</font>";
    }

    echo "<tr> ";
    echo "  <TD align='center'>$f1 $id $f2</TD>\n";

    $i=$item->appid;
    echo "  <TD align='center'>$f1 $app[$i] $f2</TD>\n";

    echo "  <TD align='center'>$f1 $item->version_num $f2</TD>\n";

    $i=$item->platformid;
    echo "  <TD align='center'>$f1 $platform[$i] $f2</TD>\n";

    echo "  <td align=center>$f1 $item->plan_class $f2</td>\n";

    $field="min_core_version_".$id;
    $v=$item->min_core_version;
    echo "  <TD align='center'>
    <input type='text' size='4' name='$field' value='$v'></TD>\n";

    $field="max_core_version_".$id;
    $v=$item->max_core_version;
    echo "  <TD align='center'>
    <input type='text' size='4' name='$field' value='$v'></TD>\n";

    $field="deprecated_".$id;
    $v='';
    if($item->deprecated) $v=' CHECKED ';
    echo "  <TD align='center'>
    <input name='$field' type='checkbox' $v></TD>\n";

    $field="delete_".$id; 
    echo "  <TD align='center'>
    <input type='text' size='6' name='$field' value=''></TD>\n";
    echo "</tr> "; 
}
mysql_free_result($result);


echo "<tr><td colspan=7><font color='RED'><sup>*</sup>
    To delete an entry you must enter 'DELETE' in this field.
    </font></td>
    <td align='center' colspan=2 bgcolor='#FFFF88'>
    <input type='submit' value='Update'></td>
    </tr>
";

end_table();


echo "</form><P>\n";
admin_page_tail();

//Generated automatically - do not edit
$cvs_version_tracker[]="\$Id: manage_app_versions.php 20745 2010-02-26 21:34:20Z davea $"; 
?>
