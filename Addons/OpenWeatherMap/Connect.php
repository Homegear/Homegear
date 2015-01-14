<?php
/****************************************************/
/****************************************************/
/***** Modify this file according to your needs *****/
/****************************************************/
/****************************************************/

$clientPath = (PHP_OS == "WINNT") ? "HM-XMLRPC-Client\\" : "HM-XMLRPC-Client/";
require_once($clientPath."Client.php");

//No SSL (recommended for "localhost")
$host = "localhost";
$port = 2001;
$ssl = false;
$Client = new \XMLRPC\Client($host, $port, $ssl);

//SSL
/*
$host = "homegear";
$port = 2003;
$ssl = true;
$username = "homegear";
$password = "homegear";

$Client = new \XMLRPC\Client($host, $port, $ssl);
$Client->setSSLVerifyPeer(false);

//!!! You should create a signed certificate and then enable "verify peer" !!!
//$Client->setSSLVerifyPeer(true);
//$Client->setCAFile("/path/to/ca.crt");

$Client->setUsername($username);
$Client->setPassword($password);
 */
?>