<?php
require_once("user.php");

if(!$_SERVER['WEBSOCKET_ENABLED']) die('WebSockets are not enabled on this server in "rpcservers.conf".');
if($_SERVER['WEBSOCKET_AUTH_TYPE'] != 'session') die('WebSocket authorization type is not set to "session" in "rpcservers.conf".');

if (!\Homegear\Homegear::nodeBlueIsReady()) {
	header("Location: starting.html");
	die();
}

$user = new User();

//{{{ Check if anonymous login is enabled
if (!empty($_SERVER['ANONYMOUS_NODE_BLUE_PATHS'])) {
	$paths = explode(',', $_SERVER['ANONYMOUS_NODE_BLUE_PATHS']);
	foreach ($paths as $path) {
		$path = trim($path);
		if (substr($_SERVER['REQUEST_URI'], 10, strlen($path)) === $path) {
			if (\Homegear\Homegear::userExists('ui')) $_SESSION['user'] = 'anonymous';
			else if (\Homegear\Homegear::userExists('ui')) $_SESSION['user'] = 'ui';
			else if (\Homegear\Homegear::userExists('homegear')) $_SESSION['user'] = 'homegear';
			else die('Path '.$_SERVER['REQUEST_URI'].' can be accessed anonymously, but a user to access this path is still required. Please create a user with the name "anonymous", "ui" or "homegear" for anonymous access (checked in this order).'); //Unknown user
			
			hg_set_user_privileges($_SESSION['user']);
            if(\Homegear\Homegear::checkServiceAccess("node-blue") !== true) die("Unauthorized.");

			$_SESSION['authorized'] = true;

			if(!array_key_exists('locale', $_SESSION)) {
	            $settings = hg_get_user_metadata($_SESSION['user']);
	            if (array_key_exists('locale', $settings)) $_SESSION['locale'] = array($settings['locale']);
	        }

			break;
		}
	}
}
//}}}

if (!$_SESSION["authorized"]) {
	if(!$user->checkAuth(true)) die("Unauthorized.");
}
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
<link rel="stylesheet" href="vendor/vendor.css">
<link rel="stylesheet" href="red/style.min.css">
</head>
<body spellcheck="false">
<div id="red-ui-editor"></div>
<script>
<?php
if(isset($_SESSION['locale']) && is_array($_SESSION['locale']) && count($_SESSION['locale']) > 0)
{
    if(array_search('en', $_SESSION['locale']) === false) array_push($_SESSION['locale'], 'en');
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
<script src="red/red.min.js"></script>
<script src="red/main.min.js"></script>
</body>
</html>
