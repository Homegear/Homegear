<?php

class UtilException extends Exception
{
}

class Util
{
    public const MBUS_FAMILY_ID = 23;
    public const MISC_FAMILY_ID = 254;

    public static function GetOrCreateTelemetryCategory(): int
    {
        $telemetryCategory = 0;
        $categories = \Homegear\Homegear::getCategories('en');
        foreach ($categories as $key => $category) {
            if (strtolower($category['NAME']) == 'telemetry') {
                $telemetryCategory = $category['ID'];
                break;
            }
        }
        if ($telemetryCategory == 0) $telemetryCategory = \Homegear\Homegear::createCategory(['en' => 'Telemetry']);
        return $telemetryCategory;
    }

    public static function GetOrCreateBuildingPart(string $locationId): int
    {
        $buildingPartId = 0;
        $buildingParts = \Homegear\Homegear::getBuildingParts('en');
        foreach ($buildingParts as $key => $buildingPart) {
            if (isset($buildingPart['METADATA']) &&
                isset($buildingPart['METADATA']['ibsAu']) &&
                $buildingPart['METADATA']['ibsAu'] == $locationId) {
                $buildingPartId = $buildingPart['ID'];
            }
        }
        if ($buildingPartId == 0) $buildingPartId = \Homegear\Homegear::createBuildingPart(['en' => $locationId], ['ibsAu' => $locationId]);
        return $buildingPartId;
    }

    public static function GetOrCreateTagCategory(string $tag): int
    {
        $tagId = 0;
        $categories = \Homegear\Homegear::getCategories('en');
        foreach ($categories as $key => $category) {
            if (strtolower($category['NAME']) == strtolower($tag)) {
                $tagId = $category['ID'];
                break;
            }
        }
        if ($tagId == 0) $tagId = \Homegear\Homegear::createCategory(['en' => $tag]);
        return $tagId;
    }

    public static function GetOrCreateSectionCategory(string $section): int
    {
        $section = strtolower($section);
        $sectionId = 0;
        $categories = \Homegear\Homegear::getCategories('en');
        foreach ($categories as $key => $category) {
            if (strtolower($category['NAME']) == 'section '.strtolower($section)) {
                $sectionId = $category['ID'];
                break;
            }
        }
        if ($sectionId == 0) $sectionId = \Homegear\Homegear::createCategory(['en' => 'Section '.$section]);
        return $sectionId;
    }

    public static function GetOrCreateServiceLevelCategory(string $serviceLevel): int
    {
        $serviceLevel = strtolower($serviceLevel);
        $serviceLevelId = 0;
        $categories = \Homegear\Homegear::getCategories('en');
        foreach ($categories as $key => $category) {
            if (strtolower($category['NAME']) == 'priority '.strtolower($serviceLevel)) {
                $serviceLevelId = $category['ID'];
                break;
            }
        }
        if ($serviceLevelId == 0) $serviceLevelId = \Homegear\Homegear::createCategory(['en' => 'Priority '.$serviceLevel]);
        return $serviceLevelId;
    }

    /**
     * @throws UtilException
     */
    public static function GetFamilyId(string $name): int
    {
        if ($name == 'mbus' || $name == 'm-bus') return Util::MBUS_FAMILY_ID;
        else if ($name == 'misc') return Util::MISC_FAMILY_ID;

        throw new UtilException('Unknown device family.');
    }
}