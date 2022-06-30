<?php
require_once('Device.php');
require_once('JsonUtil.php');

$hg = new \Homegear\Homegear();

$testPackets = [
    "27057902" => "68C4C46808007202790527475F0307EB00000004133BEA4400046D2F09D726426CBF2C4413550F130042EC7EDE060C784426330082016CBF2784011300000000C2016CBF28C401130000000082026CBE2984021300000000C2026CBF2AC40213E8F8020082036CBE2B8403133F450B00C2036CBF2CC40313550F130082046CDF21840413E1AC1B00C2046CDC22C404135E4C230082056CDF23840513055A2C00C2056CDE24C40513FEB9340082066CDF258406133A1D3E00C2066C0000C40613000000000F0100007916",
    "26595731" => "6850506808007231575926EE4D0E04CA00000002FD1700003475D86F0400046D3309D7260406F02601000413B5A51900055B00E95D42055F001E9841053EC679683E052B04CD164603222B15000C78315759261FCE16",
    "26657925" => "685E5E6808007225796526EE4D0E04BA00000002FD170000347573D40300046D3209D7260406D29D03000414FE9B4400841006F002000084101435250600055B00B0DE41055F00510B42053E00000000052B0000000003222915000C78257965261F3416",
    "21450217" => "685B5B6808007217024521B51519020D0000000C78170245210403309A0100040345E90300841003309A01008420030000000084100345E9030084200300000000042BD2FFFFFF04ABFF01E3FFFFFF04ABFF022200000004ABFF03CDFFFFFF4A16",
    "21450216" => "685B5B6808007216024521B5151902AC0000000C78160245210403284904000403FBA604008410032849040084200300000000841003FBA6040084200300000000042BF6FFFFFF04ABFF011A00000004ABFF022900000004ABFF03B2FFFFFF6D16"
];

if ($argc < 2) {
    die('Please specify the file name to import.'.PHP_EOL);
}

$testMode = false;
if ($argc >= 3 && $argv[2] == '1') {
    $testMode = true;
    echo "Test mode enabled.".PHP_EOL;
}

//Load JSON configuration file
$config = JsonUtil::Load($argv[1]);
if ($config === false) die('Could not load config file.'.PHP_EOL);

if (!isset($config['devices'])) die('Configuration file does not contain property "devices".'.PHP_EOL);

//Iterate over every device in config
try {
    foreach ($config['devices'] as $device) {
        //Step one: Precreate the devices with the provided information:
        $peerId = Device::CreateOrReturn($device);

        //Step two: Poll M-Bus device
        if ($testMode) {
            $hg->invokeFamilyMethod(Util::MBUS_FAMILY_ID, 'processPacket', [$testPackets[$device['secondaryAddress']]]);
        } else {
            if (isset($device['primaryAddress'])) {
                $hg->invokeFamilyMethod(Util::MBUS_FAMILY_ID, 'poll', [[$device['primaryAddress']]]);
            } else {
                $hg->invokeFamilyMethod(Util::MBUS_FAMILY_ID, 'poll', [[$device['secondaryAddress']]]);
            }
        }
        //Homegear might need some time to reload the device description files
        for ($i = 0; $i < 10; $i++) {
            $result = $hg->getValue($peerId, 0, 'LAST_PACKET_RECEIVED');
            if (is_int($result) && $result >= time() - 60) {
                break;
            }
            sleep(1);
        }
        if ($hg->getValue($peerId, 0, 'LAST_PACKET_RECEIVED') < time() - 60) {
            print('Error: No response from peer ' . $peerId . "." . PHP_EOL);
            continue;
        }

        //Step three: Apply settings
        Device::ApplySettings($peerId, $device);
    }
} catch (Exception $e) {
    die('Error importing device: ' . $e->getMessage().PHP_EOL);
}
