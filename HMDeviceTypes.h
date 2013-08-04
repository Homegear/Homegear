#ifndef HMDEVICETYPES_H_
#define HMDEVICETYPES_H_

#include <iostream>

enum class HMDeviceTypes : uint32_t
{
	HMUNKNOWN = 	0xFFFFFFFF,
	HMSD = 			0xFFFFFFFE,
	HMCENTRAL = 	0xFFFFFFFD,
	HMRCV50 = 		0x0000,
	HMMLCSW1FM = 	0x0004,
	HMRC4 = 		0x0008,
	HMRCP1 =		0x001A,
	HMRCSEC3 = 		0x001B,
	HMRCSEC3B = 	0x001C,
	HMRCKEY3 =		0x001D,
	HMRCKEY3B =		0x001E,
	HMSC = 			0x002F,
	HMPB4WM =		0x0035,
	HMPB2WM =		0x0036,
	HMCCTC = 		0x0039,
	HMCCVD = 		0x003A,
	HMRC4B =		0x003B,
	HMSECSD = 		0x0042,
	HMLCSW1FM = 	0x0004,
	HMLCSW2FM =		0x0009,
	HMPB4DISWM = 	0x0060,
	HMLCSW1PBUFM =	0x0069
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
