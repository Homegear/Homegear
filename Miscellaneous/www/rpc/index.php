<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">

    <title>Homegear RPC Server</title>

    <!-- Bootstrap core CSS -->
    <link href="bootstrap/css/bootstrap.min.css" rel="stylesheet">
  </head>
  <body>
  <div class="jumbotron">
      <div class="container">
          <img style="float: left; margin-top: 13px; margin-right: 40px" src="Logo.png" />
          <h2>Your</h2>
          <h1>Homegear RPC Server</h1>
          <h2>welcomes you!</h2>
      </div>
  </div>
  <div class="container marketing">
      <div class="row">
        <div style="height: 350px" class="col-lg-4">
          <span class="glyphicon glyphicon-education" style="font-size: 140px"></span>
          <h2>Wiki</h2>
          <p style="height: 40px">For help and tutorials visit the Homegear Wiki.</p>
          <p><a class="btn btn-default" href="https://homegear.eu" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-4 -->
        <div style="height: 350px" class="col-lg-4">
            <span class="glyphicon glyphicon-wrench" style="font-size: 140px"></span>
          <h2>Reference</h2>
          <p style="height: 40px">Visit the Homegear reference for information about using this RPC server.</p>
          <p><a class="btn btn-default" href="https://www.homegear.eu/index.php/Homegear_Reference" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-4 -->
        <div style="height: 350px" class="col-lg-4">
            <span class="glyphicon glyphicon-comment" style="font-size: 140px"></span>
          <h2>Forum</h2>
          <p style="height: 40px">If you still don't know, what to do, get help in our forum.</p>
          <p><a class="btn btn-default" href="https://forum.homegear.eu" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-4 -->
      </div><!-- /.row -->
      <div class="row">
        <div class="panel panel-default">
          <div class="panel-heading">Device Overview</div>
          <div class="panel-body"><p>Here's a list of all devices known to Homegear.</p></div>
            <table class="table">
                <thead><tr><th>Name</th><th>Family</th><th>ID</th><th>Serial Number</th><th>Type</th><th>Interface</th><th>Firmware</th></tr></thead>
<?php
$families = hg_invoke("listFamilies");
$familyNames = array();
foreach($families as $value)
{
  $familyNames[$value["ID"]] = $value["NAME"];
}

$devices = hg_invoke("listDevices", false, array("FAMILY", "ID", "ADDRESS", "TYPE", "FIRMWARE"));
foreach($devices as $value)
{
  $info = hg_invoke("getDeviceInfo", $value["ID"], array("NAME", "INTERFACE"));
  echo "<tr>";
  echo "<td>".$info["NAME"]."</td>";
  echo "<td>".$familyNames[$value["FAMILY"]]."</td>";
  echo "<td>".$value["ID"]."</td>";
  echo "<td>".$value["ADDRESS"]."</td>";
  echo "<td>".$value["TYPE"]."</td>";
  echo "<td>".$info["INTERFACE"]."</td>";
  echo "<td>".$value["FIRMWARE"]."</td>";
  echo "</tr>";
}
?>
            </table>
          </div>
        </div>
      <footer>
        <p class="pull-right"><a href="#">Back to top</a></p>
        <p>&copy; 2014-2015 Homegear UG</p>
      </footer>
    </div>
  </body>
</html>