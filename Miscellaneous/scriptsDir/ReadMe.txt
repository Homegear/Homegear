Usage:

Require the file "Client.php" somewhere at the top of your PHP file:

require_once("HM-XMLRPC-Client/Client.php");

Create a new instance of the client with:

$Client = new \XMLRPC\Client(HOSTNAME, PORT);

And then invoke XML RPC methods with:

$Client->send("METHODNAME", array(PARAMETERS));

See Test.php for an example.
