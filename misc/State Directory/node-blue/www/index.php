<?php
require_once("user.php");

if(!$_SERVER['WEBSOCKET_ENABLED']) die('WebSockets are not enabled on this server in "rpcservers.conf".');
if($_SERVER['WEBSOCKET_AUTH_TYPE'] != 'session') die('WebSocket authorization type is not set to "session" in "rpcservers.conf".');

$user = new User();
if(!$user->checkAuth(true)) die();
?>
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta http-equiv="X-UA-Compatible" content="IE=edge" />
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0"/>
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<title>Node-BLUE</title>
<link rel="icon" type="image/png" href="favicon.ico">
<link rel="mask-icon" href="red/images/node-red-icon-black.svg" color="#8f0000">
<link rel="stylesheet" href="vendor/jquery/css/base/jquery-ui.min.css">
<link rel="stylesheet" href="vendor/font-awesome/css/font-awesome.min.css">
<link rel="stylesheet" href="red/style.min.css">
</head>
<body spellcheck="false">
<div id="red-ui-editor"></div>
<script>
<?php
if(isset($_SESSION['locale']) && is_array($_SESSION['locale']) && count($_SESSION['locale']) > 0)
{
    if(array_search('en-US', $_SESSION['locale']) === false) array_push($_SESSION['locale'], 'en-US');
    print('var locale = '.json_encode($_SESSION['locale']).';');
}
else
{
    print('var locale = null;');
}
?>
</script>
<script src="vendor/vendor.js"></script>
<script src="red/red.js"></script>
<script src="red/main.min.js"></script>
</body>
</html>
