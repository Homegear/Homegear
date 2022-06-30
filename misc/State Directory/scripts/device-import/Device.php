<?php
require_once('Util.php');

class DeviceException extends Exception
{
}

class Device
{
    /**
     * @throws UtilException
     * @throws DeviceException
     */
    public static function CreateOrReturn(array $device): int
    {
        $peerId = 0;
        if (!isset($device['family'])) throw new DeviceException('At least one device does not have a "family" property');
        $familyId = Util::GetFamilyId($device['family']);
        if ($familyId == Util::MBUS_FAMILY_ID) {
            if (!isset($device['secondaryAddress'])) throw new DeviceException('At least one device does not have a "secondaryAddress" property');
            $primaryAddress = -1;
            if (isset($device['primaryAddress'])) $primaryAddress = intval($device['primaryAddress']);
            $peerIds = \Homegear\Homegear::getPeerId(2, '0x' . $device['secondaryAddress']);
            $deviceExists = count($peerIds) > 0;
            if ($deviceExists) {
                $peerId = $peerIds[0];
                \Homegear\Homegear::invokeFamilyMethod(Util::MBUS_FAMILY_ID, "setPrimaryAddress", [$peerId, $primaryAddress]);
            } else $peerId = \Homegear\Homegear::createDevice($familyId, -1, $device['secondaryAddress'], $primaryAddress, -1, "");
        } else throw new DeviceException('Unknown family.');
        return $peerId;
    }

    /**
     * @throws DeviceException
     */
    public static function ApplySettings(int $peerId, array $device)
    {
        //Delete all device categories
        $deviceDescription = \Homegear\Homegear::getDeviceDescription($peerId, -1, ['CATEGORIES', 'CHANNELS']);
        if (isset($deviceDescription['CATEGORIES'])) {
            foreach ($deviceDescription['CATEGORIES'] as $categoryId) {
                \Homegear\Homegear::removeCategoryFromDevice($peerId, $categoryId);
            }
        }

        //Delete all variable categories and building parts
        foreach ($deviceDescription['CHANNELS'] as $channel) {
            $variables = \Homegear\Homegear::getParamset($peerId, $channel, "VALUES");
            foreach ($variables as $variable => $value) {
                $variableDescription = \Homegear\Homegear::getVariableDescription($peerId, $channel, $variable, ["CATEGORIES", "BUILDING_PART"]);
                if (isset($variableDescription['CATEGORIES'])) {
                    foreach ($variableDescription['CATEGORIES'] as $categoryId) {
                        \Homegear\Homegear::removeCategoryFromVariable($peerId, $channel, $variable, $categoryId);
                    }
                }
                if (isset($variableDescription['BUILDING_PART'])) {
                    \Homegear\Homegear::removeVariableFromBuildingPart($peerId, $channel, $variable, $variableDescription['BUILDING_PART']);
                }
            }
        }

        //Set equipment ID
        \Homegear\Homegear::deleteMetadata($peerId, 'equipmentId');
        if (isset($device['equipmentId'])) \Homegear\Homegear::setMetadata($peerId, 'equipmentId', $device['equipmentId']);

        //Set name
        if (isset($device['name'])) \Homegear\Homegear::setName($peerId, $device['name']);

        //Set device tags
        if (isset($device['tags'])) {
            foreach ($device['tags'] as $tag) {
                $tagCategoryId = Util::GetOrCreateTagCategory($tag);
                \Homegear\Homegear::addCategoryToDevice($peerId, $tagCategoryId);
            }
        }

        //Set parent device
        \Homegear\Homegear::deleteMetadata($peerId, 'parent');
        \Homegear\Homegear::deleteMetadata($peerId, 'parentDirection');
        if (isset($device['parent']) && isset($device['parentDirection'])) {
            \Homegear\Homegear::setMetadata($peerId, 'parent', $device['parent']);
            \Homegear\Homegear::setMetadata($peerId, 'parentDirection', $device['parentDirection']);
        }

        //Loop through all data points/variables
        $telemetryCategoryId = Util::GetOrCreateTelemetryCategory();
        if (isset($device['roles'])) {
            foreach ($device['roles'] as $key => $roleSettings) {
                $role = intval($key);
                $roleChannels = \Homegear\Homegear::getVariablesInRole($role, $peerId);
                if (count($roleChannels) == 0) throw new DeviceException("Could not find role $role in peer $peerId.");
                foreach ($roleChannels as $channel => $roleVariables) {
                    foreach ($roleVariables as $variable => $variableSettings) {
                        //Add role to telemetry
                        \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $telemetryCategoryId);

                        //Set location
                        if (isset($device['location'])) {
                            $buildingPartId = Util::GetOrCreateBuildingPart($device['location']);
                            \Homegear\Homegear::addVariableToBuildingPart($peerId, $channel, $variable, $buildingPartId);
                        }
                        if (isset($variableSettings['location'])) {
                            $buildingPartId = Util::GetOrCreateBuildingPart($variableSettings['location']);
                            \Homegear\Homegear::addVariableToBuildingPart($peerId, $channel, $variable, $buildingPartId);
                        }

                        //Set section
                        if (isset($device['section'])) {
                            $sectionCategoryId = Util::GetOrCreateSectionCategory($device['section']);
                            \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $sectionCategoryId);
                        }

                        //Set data point tags
                        if (isset($variableSettings['tags'])) {
                            foreach ($variableSettings['tags'] as $tag) {
                                $tagCategoryId = Util::GetOrCreateTagCategory($tag);
                                \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $tagCategoryId);
                            }
                        }

                        //Set service level
                        if (isset($variableSettings['serviceLevel'])) {
                            if ($role < 800000 || $role >= 810000) {
                                throw new DeviceException('serviceLevel can only be set to data points with service roles.');
                            }
                            $serviceLevelCategoryId = Util::GetOrCreateServiceLevelCategory($device['serviceLevel']);
                            \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $serviceLevelCategoryId);
                        }
                    }
                }
            }
        }
    }
}