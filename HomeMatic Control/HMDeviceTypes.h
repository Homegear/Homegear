#ifndef HMDEVICETYPES_H_
#define HMDEVICETYPES_H_

#include <iostream>

enum class HMDeviceTypes : uint32_t { HMUNKNOWN = 0xFFFFFFFF, HMSD = 0xFFFFFFFE, HMCENTRAL = 0xFFFFFFFD, HMRCV50 = 0x0000, HMMLCSW1FM = 0x0004, HMCCTC = 0x0039, HMCCVD = 0x003A, HMSECSD = 0x0042 };

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
