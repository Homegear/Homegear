/* Copyright 2013 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef HMDEVICETYPES_H_
#define HMDEVICETYPES_H_

#include <iostream>

enum class DeviceTypes : uint32_t
{
	UNKNOWN = 		0xFFFFFFFF,
	HMSD = 			0xFFFFFFFE,
	HMCENTRAL = 	0xFFFFFFFD,
	HMRCV50 = 		0xFF000000,
	HMLCSW1PLOM54 =	0xFF000001,
	HMLCSW1SM =		0xFF000002,
	HMLCSW4SM =		0xFF000003,
	HMLCSW1FM = 	0xFF000004,
	HMRC4 = 		0xFF000008,
	HMLCSW2FM =		0xFF000009,
	HMLCSW2SM =		0xFF00000A,
	HMLCSW1PL =		0xFF000011,
	HMLCSW1SMATMEGA168 = 0xFF000014,
	HMLCSW4SMATMEGA168 = 0xFF000015,
	HMRCP1 =		0xFF00001A,
	HMRCSEC3 = 		0xFF00001B,
	HMRCSEC3B = 	0xFF00001C,
	HMRCKEY3 =		0xFF00001D,
	HMRCKEY3B =		0xFF00001E,
	HMRC12 =		0xFF000029,
	HMRC12B =		0xFF00002A,
	HMLCSW4PCB =	0xFF00002D,
	HMSC = 			0xFF00002F,
	HMPB4WM =		0xFF000035,
	HMPB2WM =		0xFF000036,
	HMRC19 = 		0xFF000037,
	HMRC19B =		0xFF000038,
	HMCCTC = 		0xFF000039,
	HMCCVD = 		0xFF00003A,
	HMRC4B =		0xFF00003B,
	HMSWI3FM =		0xFF000046,
	HMSECMDIR =		0xFF00004A,
	HMSECSD = 		0xFF000042,
	HMRC12SW =		0xFF00004C,
	HMRC19SW =		0xFF00004D,
	HMSENMDIRSM =	0xFF00004F,
	HMLCSW1PBFM =	0xFF000051,
	HMLCSW2PBFM =	0xFF000052,
	RCH =			0xFF000054,
	HMLCDIM1TPL =	0xFF000057,
	HMLCDIM1TCV =	0xFF000058,
	HMLCDIM1TFM =	0xFF000059,
	HMLCDIM2TSM =	0xFF00005A,
	HMSENMDIRO =	0xFF00005D,
	HMPB4DISWM = 	0xFF000060,
	HMLCSW4DR =		0xFF000061,
	HMLCSW2DR =		0xFF000062,
	ATENT =			0xFF000064,
	HMLCSW4WM =		0xFF000066,
	HMLCDIM1PWMCV =	0xFF000067,
	HMLCDIM1TPBUFM = 0xFF000068,
	HMLCSW1PBUFM =	0xFF000069,
	HMLCSW1BAPCB =	0xFF00006C,
	HMLCDIM1LCV644 = 0xFF00006E,
	HMLCDIM1LPL644 = 0xFF00006F,
	HMLCDIM2LSM644 = 0xFF000070,
	HMLCDIM1TPL644 = 0xFF000071,
	HMLCDIM1TCV644 = 0xFF000072,
	HMLCDIM1TFM644 = 0xFF000073,
	HMLCDIM2TSM644 = 0xFF000074,
	ZELSTGRMHS4 =	0xFF000080,
	HMSWI3FMROTO =	0xFF000083,
	HMLCDIMSCHUECO = 0xFF000089,
	HMLCDIMSCHUECO2 = 0xFF00008A,
	HMLCSWSCHUECO = 0xFF00008B,
	HMLCSWSCHUECO2 = 0xFF00008C,
	HMSECMDIRSCHUECO = 0xFF000090,
	HMSWI3FMSCHUECO = 0xFF000092,
	HMRC42 =		0xFF0000A0,
	HMLCSW1PL2 =	0xFF0000A1,
	HMLCDIM1TPL2 =	0xFF0000A4,
	HMRCSEC42 =		0xFF0000A5,
	HMRCKEY42 =		0xFF0000A6,
	HMPB6WM55 =		0xFF0000A9,
	HMLCSW4BAPCB =	0xFF0000AB,
};

class LogicalDeviceType
{
public:
	static int32_t getRawType(DeviceTypes type)
	{
		return ((int32_t)type) & 0xFFFF;
	}

	static DeviceTypes fromRawType(int32_t prefix, int32_t rawType)
	{
		return (DeviceTypes)((prefix << 16) + rawType);
	}

	static std::string getString(DeviceTypes type)
	{
		switch(type)
		{
		case DeviceTypes::HMRCV50:
			return "HM-RCV-50";
		default:
			return "";
		}
		return "";
	}

	static bool isSwitch(DeviceTypes type)
	{
		switch(type)
		{
		case DeviceTypes::HMLCSW1PL:
			return true;
		case DeviceTypes::HMLCSW1PL2:
			return true;
		case DeviceTypes::HMLCSW1SM:
			return true;
		case DeviceTypes::HMLCSW2SM:
			return true;
		case DeviceTypes::HMLCSW4SM:
			return true;
		case DeviceTypes::HMLCSW4PCB:
			return true;
		case DeviceTypes::HMLCSW4WM:
			return true;
		case DeviceTypes::HMLCSW1FM:
			return true;
		case DeviceTypes::HMLCSWSCHUECO:
			return true;
		case DeviceTypes::HMLCSWSCHUECO2:
			return true;
		case DeviceTypes::HMLCSW2FM:
			return true;
		case DeviceTypes::HMLCSW1PBFM:
			return true;
		case DeviceTypes::HMLCSW2PBFM:
			return true;
		case DeviceTypes::HMLCSW4DR:
			return true;
		case DeviceTypes::HMLCSW2DR:
			return true;
		case DeviceTypes::HMLCSW1PBUFM:
			return true;
		case DeviceTypes::HMLCSW4BAPCB:
			return true;
		case DeviceTypes::HMLCSW1BAPCB:
			return true;
		case DeviceTypes::HMLCSW1PLOM54:
			return true;
		case DeviceTypes::HMLCSW1SMATMEGA168:
			return true;
		case DeviceTypes::HMLCSW4SMATMEGA168:
			return true;
		default:
			return false;
		}
		return false;
	}

	static bool isDimmer(DeviceTypes type)
	{
		switch(type)
		{
		case DeviceTypes::HMLCDIM1TPL:
			return true;
		case DeviceTypes::HMLCDIM1TPL2:
			return true;
		case DeviceTypes::HMLCDIM1TCV:
			return true;
		case DeviceTypes::HMLCDIMSCHUECO:
			return true;
		case DeviceTypes::HMLCDIMSCHUECO2:
			return true;
		case DeviceTypes::HMLCDIM2TSM:
			return true;
		case DeviceTypes::HMLCDIM1TFM:
			return true;
		case DeviceTypes::HMLCDIM1LPL644:
			return true;
		case DeviceTypes::HMLCDIM1LCV644:
			return true;
		case DeviceTypes::HMLCDIM1PWMCV:
			return true;
		case DeviceTypes::HMLCDIM1TPL644:
			return true;
		case DeviceTypes::HMLCDIM1TCV644:
			return true;
		case DeviceTypes::HMLCDIM1TFM644:
			return true;
		case DeviceTypes::HMLCDIM1TPBUFM:
			return true;
		case DeviceTypes::HMLCDIM2LSM644:
			return true;
		case DeviceTypes::HMLCDIM2TSM644:
			return true;
		default:
			return false;
		}
		return false;
	}
};

#endif
