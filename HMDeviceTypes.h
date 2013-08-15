#ifndef HMDEVICETYPES_H_
#define HMDEVICETYPES_H_

#include <iostream>

enum class HMDeviceTypes : uint32_t
{
	HMUNKNOWN = 	0xFFFFFFFF,
	HMSD = 			0xFFFFFFFE,
	HMCENTRAL = 	0xFFFFFFFD,
	HMRCV50 = 		0x0000,
	HMLCSW1FM = 	0x0004,
	HMRC4 = 		0x0008,
	HMLCSW2FM =		0x0009,
	HMRCP1 =		0x001A,
	HMRCSEC3 = 		0x001B,
	HMRCSEC3B = 	0x001C,
	HMRCKEY3 =		0x001D,
	HMRCKEY3B =		0x001E,
	HMRC12 =		0x0029,
	HMRC12B =		0x002A,
	HMSC = 			0x002F,
	HMPB4WM =		0x0035,
	HMPB2WM =		0x0036,
	HMRC19 = 		0x0037,
	HMRC19B =		0x0038,
	HMCCTC = 		0x0039,
	HMCCVD = 		0x003A,
	HMRC4B =		0x003B,
	HMSECSD = 		0x0042,
	HMRC12SW =		0x004C,
	HMRC19SW =		0x004D,
	RCH =			0x0054,
	HMPB4DISWM = 	0x0060,
	ATENT =			0x0064,
	HMLCSW1PBUFM =	0x0069,
	ZELSTGRMHS4 =	0x0080
};

class HMDeviceType
{
public:
	static std::string getString(HMDeviceTypes type)
	{
		switch(type)
		{
		case HMDeviceTypes::HMRCV50:
			return "HM-RCV-50";
		default:
			return "";
		}
		return "";
	}
};

#endif
