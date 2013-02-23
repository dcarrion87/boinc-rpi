<?php
	
	echo "<table class=\"recentfeed\">";
	echo "<tr><td><div class=\"tablehead\">Max Value (M)</div></td><td><div class=\"tablehead\">Subset Size (N)</div></td><td><div class=\"tablehead\">Workunits</div></td><td><div class=\"tablehead\">Completed</div></td><td><div class=\"tablehead\">Errored</div></td><td><div class=\"tablehead\">Failed Sets</div></td><td><div class=\"tablehead\">Details</div></td></tr>";

	while($row = mysql_fetch_array($result))
	{
		echo mysql_error();
		echo "<tr class=\"d" . ($row['max_value'] & 1) . "\">";
#        echo "<tr>";

        echo "<td><div class=\"max_value\">" . $row['max_value'] . "</div></td>";
        echo "<td><div class=\"subset_size\">" . $row['subset_size'] . "</div></td>";
        echo "<td><div class=\"slices\">". $row['slices'] . "</div></td>";
        echo "<td><div class=\"completed\">" . $row['completed'] . "</div></td>";
        echo "<td><div class=\"errors\">" . $row['errors'] . "</div></td>";
        echo "<td><div class=\"failed_set_count\">" . $row['failed_set_count'] . "</div></td>";
        if ($row['webpage_generated'] == true) {
            echo "<td><div class=\"details\"><a href=\"../download/set_" . $row['max_value'] . "c" . $row['subset_size'] . ".html\">details</a></div></td>";
        } else {
            echo "<td><div class=\"details\"></div</td>";
        }

		echo "</tr>";
	}
	echo "</table>";

?>
