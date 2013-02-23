<html>
<head>
<title>UND - SubsetSum@Home Progress</title>
<link rel="stylesheet" type="text/css" href="style.css" />
</head>
<body>
<div class="title">SubsetSum@Home Progress</div>
<br />
<div class="navigationmain">
<div class="header">Navigation</div>
<?php
require_once("navigation.php");
require_once("db.inc"); //contains the $user and $pass variables to connect to the database, DO NOT COMMIT THIS TO GITHUB (make sure it stays ignored)
?>
</div>
<table class="description">
<tr>
<td width="1008px">
<p>Here is the progress we have so far of evaluated sets for given M (maximum set value) and N (set size). The Workunits, Completed and Errored columns specify how many workunits were generated for a given M and N, how many have been completed successfully and validated by the server, and how many are currently errored out, respectively.  For M &lt; 36, these were calculated offline so no workunits were generated, completed or errored. Failed sets specifies how many subsets failed the conjectue (and in the case of incomplete runs for M and N, how many currently are known to have failed). When all workunits have been completed for a given M and N, a webpage with details about which sets failed is generated and a link is provided in the details column.</p>

<p>The format of the details is as follows. The first number is the iteration of the set. The numbers between the square brackets [] are the elements of the set. The program calculates the sets using arrays of binary numbers, and they go from right to left (not left to right). If there is a 0, then the set cannot generate that sum, and if there is a 1 it can. </p>

<p>So lets take 8 choose 5. In this case, a 0 on the far left means that any combination of the numbers in the set cannot generate the sum 32 (where the sums gets larger there will be longer bit strings). So the output says the set on the first line, [ 1 2 3 4 8], can generate every sum from 18 to 1. For the conjecture, we're interested in the sums from 10 to 8 (inclusive), and these are highlighted in green. Sets 9, 19 and 23 cannot generate all those central sums -- and the missing sums are highlighted in red. For set 9, it cannot generate the sum 12, set 19 cannot generate the sums 10 and 17, and set 23 cannot generate the sum 12. Ie., no combination of the sets elements can be added together to get that number.</p>

<p>As the number of sets evaluated increases exponentially as M and N increase, we'll only be printing out the failed sets (those are the ones we're interested anyways as we hope more information on these will help us prove the conjecture).</p>

<p>Feel free to discuss the progress on the following forum thread: <a href="http://volunteer.cs.und.edu/subset_sum/forum_thread.php?id=2">Set Analysis Progress</a>.
</td>
</tr>
</table>

<table class="layout">
<tr>
<td width="1000px">
<div class="recent">
<div class="header">Set Analysis Progress</div>
<?php
mysql_connect("localhost", $user, $pass);
mysql_select_db("subset_sum");
$query = "SELECT max_value, subset_size, slices, completed, errors, failed_set_count, webpage_generated FROM sss_runs ORDER BY max_value, subset_size"; 
$result = mysql_query($query);
require_once("need.php");
?>
</div>
</td>
</tr>
</table>
</body>
</html>
