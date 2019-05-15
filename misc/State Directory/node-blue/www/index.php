<?php
require_once("user.php");

if(!$_SERVER['WEBSOCKET_ENABLED']) die('WebSockets are not enabled on this server in "rpcservers.conf".');
if($_SERVER['WEBSOCKET_AUTH_TYPE'] != 'session') die('WebSocket authorization type is not set to "session" in "rpcservers.conf".');

$user = new User();
if(!$user->checkAuth(true)) die();
?>
<!DOCTYPE html>
<html>
<meta charset="utf-8">
<meta http-equiv="X-UA-Compatible" content="IE=edge" />
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0"/>
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<head>
<title>Node-BLUE</title>
<link rel="icon" type="image/png" href="favicon.ico">
<link rel="mask-icon" href="red/images/node-red-icon-black.svg" color="#8f0000">
<link href="vendor/bootstrap/css/bootstrap.min.css" rel="stylesheet" media="screen">
<link href="vendor/jquery/css/smoothness/jquery-ui-1.10.3.custom.min.css" rel="stylesheet" media="screen">
<link rel="stylesheet" href="vendor/font-awesome/css/font-awesome.min.css">
<link rel="stylesheet" href="red/style.min.css">
</head>
<body spellcheck="false">
<div id="header">
    <span class="logo"><img src="red/images/node-blue.png"> <span>Node-BLUE</span></span>
    <ul class="header-toolbar hide">
        <li><a id="btn-sidemenu" class="button" data-toggle="dropdown" href="#"><i class="fa fa-bars"></i></a></li>
    </ul>
    <div id="header-shade" class="hide"></div>
</div>
<div id="main-container" class="sidebar-closed hide">
    <div id="workspace">
        <ul id="workspace-tabs"></ul>
        <div id="chart" tabindex="1"></div>
        <div id="workspace-toolbar"></div>
        <div id="workspace-footer">
            <a class="workspace-footer-button" id="btn-zoom-out" href="#"><i class="fa fa-minus"></i></a>
            <a class="workspace-footer-button" id="btn-zoom-zero" href="#"><i class="fa fa-circle-o"></i></a>
            <a class="workspace-footer-button" id="btn-zoom-in" href="#"><i class="fa fa-plus"></i></a>
            <a class="workspace-footer-button-toggle single" id="btn-navigate" href="#"><i class="fa fa-map-o"></i></a>
        </div>
        <div id="editor-shade" class="hide"></div>
    </div>
    <div id="editor-stack"></div>
    <div id="palette">
        <img src="red/images/spin.svg" class="palette-spinner hide"/>
        <div id="palette-search" class="palette-search hide">
            <input type="text" data-i18n="[placeholder]palette.filter"></input>
        </div>
        <div id="palette-container" class="palette-scroll hide"></div>
        <div id="palette-footer">
            <a class="palette-button" id="palette-collapse-all" href="#"><i class="fa fa-angle-double-up"></i></a>
            <a class="palette-button" id="palette-expand-all" href="#"><i class="fa fa-angle-double-down"></i></a>
        </div>
        <div id="palette-shade" class="hide"></div>
    </div><!-- /palette -->
    <div id="sidebar">
        <ul id="sidebar-tabs"></ul>
        <div id="sidebar-content"></div>
        <div id="sidebar-footer"></div>
        <div id="sidebar-shade" class="hide"></div>
    </div>

    <div id="sidebar-separator"></div>

</div>
<div id="full-shade" class="hide"></div>

<div id="notifications"></div>
<div id="dropTarget"><div data-i18n="[append]workspace.dropFlowHere"><br/><i class="fa fa-download"></i></div></div>

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
<script src="vendor/ace/ace.js"></script>
<script src="vendor/ace/ext-language_tools.js"></script>
<script src="red/red.min.js"></script>
<script src="red/main.min.js"></script>

</body>
</html>
