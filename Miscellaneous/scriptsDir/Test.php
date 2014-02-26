#!/usr/bin/env php
<?php
require_once("HM-XMLRPC-Client/Client.php");
$Client = new \XMLRPC\Client("localhost", 2001);

//print_r($Client->send("system.methodHelp", array("getLinks")));
//print_r($Client->send("getDeviceDescription", array(23, 1)));
//print_r($Client->send("getLinkInfo", array(23, 1, 37, 3)));
//print_r($Client->send("getMetadata", array("54:2")));
//print_r($Client->send("getParamset", array(12, -1, "MASTER")));
//print_r($Client->send("getParamset", array(23, 1, 37, 3)));
//print_r($Client->send("getParamsetDescription", array(23, 1, "LINK")));
//print_r($Client->send("getParamsetId", array(23, 1, 37, 3)));
//print_r($Client->send("getServiceMessages", array()));
//print_r($Client->send("getLinkPeers", array(27, 1)));
//print_r($Client->send("getLinks", array()));
//print_r($Client->send("getLinks", array(27, 1, 31)));
print_r($Client->send("listDevices", array()));
//print_r($Client->send("putParamset", array(54, 1, "MASTER", array("EVENT_DELAYTIME" => 0.0))));
//print_r($Client->send("putParamset", array(63, 18, "MASTER", array("MESSAGE_SHOW_TIME" => 0.0))));
//print_r($Client->send("setTeam", array(12, 1)));
//print_r($Client->send("setTeam", array(12, 1, 0x80000002, 1)));
//print_r($Client->send("setValue", array(15, 1, "ON_TIME", 787487.3)));
//print_r($Client->send("setValue", array(15, 1, "STATE", true)));
//print_r($Client->send("setValue", array(19, 1, "VALVE_STATE", 0)));
//print_r($Client->send("setValue", array(21, 2, "SETPOINT", 16.0)));
//print_r($Client->send("setValue", array(62, 18, "TEXT", "Hi")));
//print_r($Client->send("setValue", array(63, 18, "SUBMIT", true)));
?>

