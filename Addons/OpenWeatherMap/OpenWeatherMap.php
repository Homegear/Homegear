#!/usr/bin/env php
<?php
function logError($errno, $errstr, $errfile, $errline)
{  
    switch($errno)
    {
        case E_USER_ERROR:
            error_log("Error: $errstr \n Fatal error on line $errline in file $errfile \n", 3, "/var/log/homegear/openweathermap.log");
            break;
        case E_USER_WARNING:
            error_log("Warning: $errstr \n in $errfile on line $errline \n", 3, "/var/log/homegear/openweathermap.log");
            break;
        case E_USER_NOTICE:
            error_log("Notice: $errstr \n in $errfile on line $errline \n", 3, "/var/log/homegear/openweathermap.log");
            break;
        default:
            error_log("Unknown error [#$errno]: $errstr \n in $errfile on line $errline \n", 3, "/var/log/homegear/openweathermap.log");
            break;
    }
    return true;
}

set_error_handler("logError");
set_time_limit (0);

if($argc < 2) die("Please provide a peer id as first parameter.\n");
$PeerID = (integer)$argv[1];

include_once("Connect.php");

while(true)
{
    $Config = $Client->send("getParamset", array($PeerID, 0, "MASTER"));
    if(!array_key_exists("LANGUAGE_CODE", $Config) || !array_key_exists("COUNTRY_CODE", $Config) || !array_key_exists("CITY", $Config)) die("Peer does not seem to be an OpenWeatherMap device.");
    $Interval = $Config["INTERVAL"];
    if($Interval < 60) $Interval = 60;

    $URL = "http://api.openweathermap.org/data/2.5/weather?q=".$Config["CITY"].",".$Config["COUNTRY_CODE"]."&units=metric&cnt=7&lang=".$Config["LANGUAGE_CODE"];
    $JSON = file_get_contents($URL);
    $Data = json_decode($JSON, true);

    $Client->send("setValue", array($PeerID, 1, "CITY_LONGITUDE", (double)$Data["coord"]["lon"]));
    $Client->send("setValue", array($PeerID, 1, "CITY_LATITUDE", (double)$Data["coord"]["lat"]));
    $Client->send("setValue", array($PeerID, 1, "SUNRISE", (integer)$Data["sys"]["sunrise"]));
    $Client->send("setValue", array($PeerID, 1, "SUNSET", (integer)$Data["sys"]["sunset"]));
    if(array_key_exists("weather", $Data) && count($Data["weather"]) > 0)
    {
        $Client->send("setValue", array($PeerID, 1, "WEATHER", (string)$Data["weather"][0]["main"]));
        $Client->send("setValue", array($PeerID, 1, "WEATHER_DESCRIPTION", (string)$Data["weather"][0]["description"]));
        $Client->send("setValue", array($PeerID, 1, "WEATHER_ICON", (string)$Data["weather"][0]["icon"]));
        $Client->send("setValue", array($PeerID, 1, "WEATHER_ICON_URL", "http://openweathermap.org/img/w/".$Data["weather"][0]["icon"].".png"));
    }
    $Client->send("setValue", array($PeerID, 1, "TEMPERATURE", (double)$Data["main"]["temp"]));
    $Client->send("setValue", array($PeerID, 1, "HUMIDITY", (integer)$Data["main"]["humidity"]));
    $Client->send("setValue", array($PeerID, 1, "PRESSURE", (double)$Data["main"]["pressure"]));
    $Client->send("setValue", array($PeerID, 1, "WIND_SPEED", (double)$Data["wind"]["speed"]));
    $Client->send("setValue", array($PeerID, 1, "WIND_GUST", array_key_exists("gust", $Data["wind"]) ? (double)$Data["wind"]["gust"] : 0.0));
    $Client->send("setValue", array($PeerID, 1, "WIND_DIRECTION", (double)$Data["wind"]["deg"]));
    $Client->send("setValue", array($PeerID, 1, "RAIN_3H", array_key_exists("rain", $Data) && array_key_exists("3h", $Data["rain"]) ? (double)$Data["rain"]["3h"] : 0.0));
    $Client->send("setValue", array($PeerID, 1, "SNOW_3H", array_key_exists("snow", $Data) && array_key_exists("3h", $Data["snow"]) ? (double)$Data["snow"]["3h"] : 0.0));
    $Client->send("setValue", array($PeerID, 1, "CLOUD_COVERAGE", (integer)$Data["clouds"]["all"]));

    sleep($Interval);
}
?>
