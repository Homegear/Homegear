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
	HMSECMDIR =		0x004A,
	HMSECSD = 		0x0042,
	HMRC12SW =		0x004C,
	HMRC19SW =		0x004D,
	HMSENMDIRSM =	0x004F,
	RCH =			0x0054,
	HMSENMDIRO =	0x005D,
	HMPB4DISWM = 	0x0060,
	ATENT =			0x0064,
	HMLCSW1PBUFM =	0x0069,
	ZELSTGRMHS4 =	0x0080,
	HMSECMDIRSCHUECO = 0x0090,
	HMRC42 =		0x00A0,
	HMRCSEC42 =		0x00A5,
	HMRCKEY42 =		0x00A6,
	HMPB6WM55 =		0x00A9
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
