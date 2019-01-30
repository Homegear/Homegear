<?php
require_once("user.php");

$loginResult = -3;
if(isset($_GET["logout"]))
{
  $user = new User();
  $user->logout();
}
else
{
  if(isset($_POST["username"]) && isset($_POST["password"]) && $_POST["username"] && $_POST["password"])
  {
    $user = new User();
    $loginResult = $user->login($_POST["username"], urldecode($_POST["password"]));
    if($loginResult === 0)
    {
      header("Location: index.php");
      die();
    }
  }
}

$locales = explode(',', explode(';', $_SERVER['HTTP_ACCEPT_LANGUAGE'])[0]);
$i18n = array();
if(file_exists('static/locales/en-US/signin')) $i18n = json_decode(file_get_contents('static/locales/en-US/signin'), true);

foreach($locales as $locale)
{
  if(file_exists('static/locales/'.$locale.'/signin'))
  {
    $i18n2 = json_decode(file_get_contents('static/locales/'.$locale.'/signin'), true);
    $i18n = array_replace_recursive($i18n, $i18n2);
    break;
  }
}

$i18n = json_decode(json_encode($i18n), false);
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="author" content="Homegear UG">

  <title>Node-BLUE</title>

  <link href="vendor/bootstrap/css/bootstrap.min.css" rel="stylesheet" media="screen">
  <style>
body {
  width: 100%;
  padding-top: 40px;
  padding-bottom: 40px;
  background-color: #eee;
}

input {
  width: 230px;
  border: none;
  font-weight: 300;
  font-size: 20px;
  padding: 11px 10px 9px;
  box-sizing: content-box;
  background: #fff;
  color: #555;
  cursor: text;
  outline: none;
  border-radius: 3px;
}

input.inputtop {
  border-bottom-left-radius: 0 !important;
  border-bottom-right-radius: 0 !important;
  margin-top: 5px;
  margin-bottom: 0 !important;
}

input.inputbottom {
  border-top-left-radius: 0 !important;
  border-top-right-radius: 0 !important;
  margin-top: 0 !important;
  margin-bottom: 5px;
}

.alert {
  margin-bottom: 5px;
}

button {
  padding: 10px 20px;
  width: 250px;
  font-size: 20px;
  border: 1px solid #3fa9f5;
  background-color: #3fa9f5;
  color: #fff;
  font-weight: 600;
  cursor: pointer;
  outline: none;
  border-radius: 3px;
}

p.footer {
  color: #1b1464;
  font-weight: 600;
  text-decoration: none;
  outline: none;
  text-align: center;
  margin-top: 40px
}
  </style>
</head>

<body>
  <div style="text-align: center; margin-bottom: 20px;"><img style="width: 200px; margin-left: auto; margin-right: auto;" src="red/images/node-blue.svg" /></div>
  <div style="position: relative; margin-left: auto; margin-right: auto; width: 250px;">
    <form role="form" action="<?PHP print $_SERVER["PHP_SELF"]; ?>" method="post">
      <input type="hidden" name="url" value="<?php if(isset($_GET['url'])) print $_GET['url']; ?>" />
      <input type="user" id="inputUser" name="username" class="inputtop" placeholder="<?php print($i18n->common->username); ?>" required autofocus />
      <input type="password" id="inputPassword" name="password" class="inputbottom" placeholder="<?php print($i18n->common->password); ?>" required />
      <?php
        if($loginResult === -1) print('<div class="alert alert-danger" role="alert">'.$i18n->common->error->wrongpassword.'</div>'); 
        else if($loginResult === -2) print('<div class="alert alert-danger" role="alert">'.$i18n->common->error->noaccess.'</div>');
      ?>
      <button type="submit"><?php print($i18n->common->signin); ?></button>
    </form>
    <p class="footer">Node-BLUE</p>
  </div>
</body>
</html>
