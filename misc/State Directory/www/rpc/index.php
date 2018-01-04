<?php

$hgDevices = [];
$hgVersion = '-';
$hgError = false;

try {

    $hg = new \Homegear\Homegear();

    $hgVersion = explode(' ', $hg->getVersion())[1];

    $families = $hg->listFamilies();

    $familyNames = [];
    foreach ($families as $value) {
        $familyNames[$value["ID"]] = $value["NAME"];
    }

    $rawDevices = $hg->listDevices(false, ["FAMILY", "ID", "ADDRESS", "TYPE", "FIRMWARE"]);

    foreach ($rawDevices as $item) {

        $info = $hg->getDeviceInfo($item["ID"], ["NAME", "INTERFACE"]);

        $hgDevices[] = [
            'name' => $info["NAME"],
            'family' => $familyNames[$item["FAMILY"]],
            'id' => ($item["ID"] > 999999 ? "0x" . dechex($item["ID"]) : $item["ID"]),
            'address' => $item["ADDRESS"],
            'type' => $item["TYPE"],
            'interface' => (array_key_exists("INTERFACE", $info) ? $info["INTERFACE"] : "&nbsp;"),
            'firmware' => $item["FIRMWARE"]
        ];
    }


} catch (\Exception $e) {
    $hgError = $e->getMessage();
}


?>
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
    <link href="assets/bootstrap/css/bootstrap.min.css" rel="stylesheet"/>
    <link href="assets/css/main.css" rel="stylesheet"/>
</head>
<body>
<div class="jumbotron">
    <div class="container">
        <small class="pull-right">
            <b>Version:</b> <?= $hgVersion ?>
        </small>
        <img style="float: left; margin-top: 13px; margin-right: 40px" src="assets/images/Logo.png"/>
        <h2>Your</h2>
        <h1>Homegear RPC Server</h1>
        <h2>welcomes you!</h2>
    </div>
</div>
<div class="container marketing">
    <div class="row">
        <div style="height: 350px" class="col-lg-3">
            <span class="icon-nodes" style="font-size: 140px"></span>
            <h2>Node-BLUE</h2>
            <p style="height: 40px">To make your home smart use Homegear's logic engine.</p>
            <p><a class="btn btn-default" href="/flows" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-3 -->
        <div style="height: 350px" class="col-lg-3">
            <span class="glyphicon glyphicon-education" style="font-size: 140px"></span>
            <h2>Documentation</h2>
            <p style="height: 40px">For help and tutorials visit the Homegear documentation.</p>
            <p><a class="btn btn-default" href="https://doc.homegear.eu" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-3 -->
        <div style="height: 350px" class="col-lg-3">
            <span class="glyphicon glyphicon-wrench" style="font-size: 140px"></span>
            <h2>Reference</h2>
            <p style="height: 40px">Visit the Homegear reference for information about using this RPC server.</p>
            <p><a class="btn btn-default" href="https://ref.homegear.eu" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-3 -->
        <div style="height: 350px" class="col-lg-3">
            <span class="glyphicon glyphicon-comment" style="font-size: 140px"></span>
            <h2>Forum</h2>
            <p style="height: 40px">If you still don't know, what to do, get help in our forum.</p>
            <p><a class="btn btn-default" href="https://forum.homegear.eu" role="button">Go there &raquo;</a></p>
        </div><!-- /.col-lg-3 -->
    </div><!-- /.row -->
    <div class="row">
        <?php

        if ($hgError === false) {

            ?>
            <div class="panel panel-default">
                <div class="panel-heading">
                    Device Overview
                </div>
                <?php

                if (count($hgDevices)) {
                    ?>
                    <div class="panel-body">
                        <p>Here's a list of all devices known to Homegear.</p>
                    </div>

                    <table class="table">
                        <thead>
                        <tr>
                            <th>Name</th>
                            <th>Family</th>
                            <th>ID</th>
                            <th>Serial Number</th>
                            <th>Type</th>
                            <th>Interface</th>
                            <th>Firmware</th>
                        </tr>
                        </thead>
                        <tbody>
                        <?php
                        foreach ($hgDevices as $item) {
                            echo '<tr>' . PHP_EOL;
                            foreach ($item AS $value) {
                                echo '<td>' . htmlspecialchars($value) . '</td>' . PHP_EOL;
                            }
                            echo '</tr>' . PHP_EOL;
                        }
                        ?>
                        </tbody>
                    </table>


                    <?php
                } else {
                    ?>
                    <div class="panel-body">
                        <div class="alert alert-info">
                            No devices known to Homegear yet.
                        </div>
                    </div>
                    <?php
                }
                ?>
            </div>
            <?php

        } else {
            ?>
            <div class="alert alert-danger">
                Can't connect to the Homegear service.<br>
                <b>Error: </b>
                <?= htmlspecialchars($hgError) ?>
            </div>
            <?php
        }

        ?>
    </div>
    <footer>
        <p class="pull-right"><a href="#">Back to top</a></p>
        <p>&copy; 2014-<?= date('Y') ?> Homegear UG (haftungsbeschränkt)</p>
    </footer>
</div>
</body>
</html>
