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
        if (!isset($device['family'])) throw new DeviceException('Device does not have a "family" property');
        $familyId = Util::GetFamilyId($device['family']);
        if ($familyId == Util::MBUS_FAMILY_ID) {
            if (!isset($device['secondaryAddress']) || intval($device['secondaryAddress']) == 0) throw new DeviceException('Device has an invalid "secondaryAddress" property');
            $primaryAddress = -1;
            if (isset($device['primaryAddress'])) $primaryAddress = intval($device['primaryAddress']);
            $peerIds = \Homegear\Homegear::getPeerId(2, '0x' . $device['secondaryAddress']);
            $deviceExists = count($peerIds) > 0;
            if ($deviceExists) {
                $peerId = $peerIds[0];
                \Homegear\Homegear::invokeFamilyMethod(Util::MBUS_FAMILY_ID, "setPrimaryAddress", [$peerId, $primaryAddress]);
            } else {
                $peerId = \Homegear\Homegear::createDevice($familyId, -1, $device['secondaryAddress'], $primaryAddress, -1, $device['interface'] ?? '');
                if (is_array($peerId)) throw new DeviceException('Could not create device: '.$peerId['faultString']);
            }
        } else if ($familyId == Util::MISC_FAMILY_ID) {
            if (!isset($device['id'])) throw new DeviceException('Device does not have a "id" property');
            if (!isset($device['role'])) throw new DeviceException('Device does not have a "role" property');
            $role = intval($device['role']);
            $peerIds = \Homegear\Homegear::getPeerId(1, $device['id']);
            $deviceExists = count($peerIds) > 0;
            if ($deviceExists) {
                $peerId = $peerIds[0];
            } else {
                $roleTypeIdMap = [
                    900100 => 0xF01A,
                    900200 => 0xF01B,
                    900300 => 0xF01C,
                    900400 => 0xF01D,
                    900500 => 0xF01E,
                    900600 => 0xF01A,
                    900700 => 0xF01A,
                    900800 => 0xF01A
                ];
                if (!isset($roleTypeIdMap[$role])) throw new DeviceException('Unknown role.');
                $peerId = \Homegear\Homegear::createDevice($familyId, $roleTypeIdMap[$role], $device['id'], -1, -1, '');
            }
            if ($role == 900101 || $role == 900601 || $role == 900701 || $role == 900801) {
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900100);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900101);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900600);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900601);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900700);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900701);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900800);
                \Homegear\Homegear::removeRoleFromVariable($peerId, 1, "VOLUME", 900801);
                \Homegear\Homegear::addRoleToVariable($peerId, 1, "VOLUME", $role);
            }
            //All other meter roles are defined by their XML file
        } else throw new DeviceException('Unknown family.');
        return $peerId;
    }

    /**
     * @throws DeviceException
     */
    public static function ApplySettings(int $peerId, array $device, string &$error)
    {
        if (!isset($device['family'])) throw new DeviceException('Device does not have a "family" property');
        $familyId = Util::GetFamilyId($device['family']);

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
                $variableDescription = \Homegear\Homegear::getVariableDescription($peerId, $channel, $variable, ["CATEGORIES", "BUILDING_PART", "ROLES"]);
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

        //Remove from all virtual meters
        $types = ['0xF01A', '0xF01B', '0xF01C', '0xF01D', '0xF01E'];
        foreach ($types as $type) {
            $peerIds = \Homegear\Homegear::getPeerId(3, $type);
            foreach ($peerIds as $virtualPeerId) {
                $config = \Homegear\Homegear::getParamset($virtualPeerId, 0, "MASTER");
                if (!isset($config['PEERS']) || !is_array($config['PEERS'])) continue;
                if (isset($config['PEERS'][$peerId])) {
                    unset($config['PEERS'][$peerId]);
                    \Homegear\Homegear::putParamset($virtualPeerId, 0, "MASTER", $config);
                }
            }
        }

        //Add to virtual meter
        if (isset($device['meterGroup']) && $familyId != Util::MISC_FAMILY_ID) {
            if (!isset($device['meterGroup']['virtualMeterId'])) throw new DeviceException('Property meterGroup\virtualMeterId is missing.');
            $peerIds = \Homegear\Homegear::getPeerId(1, $device['meterGroup']['virtualMeterId']);
            if (count($peerIds) == 0) throw new DeviceException('Virtual meter ID ' . $device['meterGroup']['virtualMeterId'] . ' is unknown.');
            $virtualPeerId = $peerIds[0];
            $config = \Homegear\Homegear::getParamset($virtualPeerId, 0, "MASTER");
            if (!isset($config['PEERS'])) throw new DeviceException('Virtual meter is missing configuration parameter "PEERS".');
            if (isset($device['meterGroup']['operation']) && $device['meterGroup']['operation'] != 'add' && $device['meterGroup']['operation'] != 'subtract') {
                throw new DeviceException('Property meterGroup\operation has invalid value.');
            }
            if (!is_array($config['PEERS'])) $config['PEERS'] = [];
            $direction = 1;
            if (isset($device['meterGroup']['operation']) && $device['meterGroup']['operation'] == 'subtract') $direction = 0;
            $config['PEERS'][$peerId] = ['direction' => $direction];
            \Homegear\Homegear::putParamset($virtualPeerId, 0, "MASTER", $config);
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
        if (isset($device['parent']) && strlen($device['parent']) > 0 && isset($device['parentDirection'])) {
        	$parent = $device['parent'];
        	$parts = explode('_', $parent, 2);
        	if (count($parts) > 1 && $parts[1][0] == '%') {
        		$peerIds = \Homegear\Homegear::getPeerId(1, substr($parts[1], 1, -1));
        		if (count($peerIds) > 0) {
        			$parent = $parts[0].'_'.$peerIds[0];
        		}
        	}
            \Homegear\Homegear::setMetadata($peerId, 'parent', $parent);
            \Homegear\Homegear::setMetadata($peerId, 'parentDirection', $device['parentDirection']);
        }

        //Loop through all data points/variables
        $telemetryCategoryId = Util::GetOrCreateTelemetryCategory();
        if (isset($device['roles'])) {
            foreach ($device['roles'] as $key => $roleSettings) {
                $role = intval($key);
                $roleChannels = \Homegear\Homegear::getVariablesInRole($role, $peerId);
                if (count($roleChannels) == 0) {
                	if ($roleSettings['alternativeRoles'] && is_array($roleSettings['alternativeRoles'])) {
						foreach ($roleSettings['alternativeRoles'] as $alternativeRole) {
							$role = intval($alternativeRole);
							$roleChannels = \Homegear\Homegear::getVariablesInRole($role, $peerId);
							if (count($roleChannels) > 0) break;
						}
                	}
                	if (count($roleChannels) == 0) {
                		if (strlen($error == 0)) $error = "Could not find role $role";
                		else $error .= ", could not find role $role";
                		continue;
                	}
                }
                foreach ($roleChannels as $channel => $roleVariables) {
                    foreach ($roleVariables as $variable => $variableSettings) {
                        //Add role to telemetry
                        \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $telemetryCategoryId);

                        //Set location
                        if (isset($device['location'])) {
                            $buildingPartId = Util::GetOrCreateBuildingPart($device['location']);
                            \Homegear\Homegear::addVariableToBuildingPart($peerId, $channel, $variable, $buildingPartId);
                        }
                        if (isset($roleSettings['location'])) {
                            $buildingPartId = Util::GetOrCreateBuildingPart($roleSettings['location']);
                            \Homegear\Homegear::addVariableToBuildingPart($peerId, $channel, $variable, $buildingPartId);
                        }

                        //Set section
                        if (isset($device['section'])) {
                            $sectionCategoryId = Util::GetOrCreateSectionCategory($device['section']);
                            \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $sectionCategoryId);
                        }

                        //Set data point tags
                        if (isset($roleSettings['tags'])) {
                            foreach ($roleSettings['tags'] as $tag) {
                                $tagCategoryId = Util::GetOrCreateTagCategory($tag);
                                \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $tagCategoryId);
                            }
                        }

                        //Set service level
                        if (isset($roleSettings['serviceLevel'])) {
                            if ($role < 800000 || $role >= 810000) {
                                throw new DeviceException('serviceLevel can only be set to data points with service roles.');
                            }
                            $serviceLevelCategoryId = Util::GetOrCreateServiceLevelCategory($roleSettings['serviceLevel']);
                            \Homegear\Homegear::addCategoryToVariable($peerId, $channel, $variable, $serviceLevelCategoryId);
                        }
                    }
                }
            }
        }
    }
}