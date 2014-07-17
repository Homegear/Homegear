#!/usr/bin/env php
<?
require_once("HM-XMLRPC-Client/Client.php");
$Client = new \XMLRPC\Client("localhost", 2001);

if(count($argv) < 2) die("Please specify the devices serial number.\n");
$Serial = htmlentities($argv[1]);
if(strlen($Serial) > 100) die("Serial number is too long\n");
if(gettype($Serial) != "string" || mb_detect_encoding($Serial, "ASCII", true) != "ASCII") die("Serial is not a valid ASCII string.\n");

$ID = $Client->send("getPeerId", array($Serial));
if(is_array($ID))
{
	if(count($ID) == 2 && isset($ID["faultCode"]))
	{
		if($ID["faultCode"] == -2) die("No device with this serial number found.\n");
		else die($ID["faultString"]."\n");
	}
	$ID = $ID[0];
}
if(gettype($ID) == "integer") { print "ID: ".$ID."\n"; }
else die("Error getting serial number.\n");
?>
