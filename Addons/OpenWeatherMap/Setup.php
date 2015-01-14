#!/usr/bin/env php
<?php
include_once("Connect.php");

$Client->send("createDevice", array(254, 256, "OPNWEATHR1", -1, -1));
?>

