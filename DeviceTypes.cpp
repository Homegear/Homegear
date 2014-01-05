/* Copyright 2013-2014 Sathya Laufer
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

#include "DeviceTypes.h"
#include "HelperFunctions.h"
#include "GD.h"

LogicalDeviceType::LogicalDeviceType(DeviceID id, DeviceFamily family, uint32_t type, std::string name)
{
	_id = id;
	_family = family;
	_type = type;
	_name = name;
}

bool LogicalDeviceType::operator==(LogicalDeviceType& other)
{
	if(other.family() == _family && other.type() == _type) return true;
	return false;
}

bool LogicalDeviceType::operator!=(LogicalDeviceType& other)
{
	if(other.family() != _family || other.type() != _type) return true;
	return false;
}

bool LogicalDeviceType::isSwitch()
{
	if(_family == DeviceFamily::HomeMaticBidCoS)
	{
		switch(_id)
		{
		case DeviceID::HMLCSW1PL:
			return true;
		case DeviceID::HMLCSW1PL2:
			return true;
		case DeviceID::HMLCSW1SM:
			return true;
		case DeviceID::HMLCSW2SM:
			return true;
		case DeviceID::HMLCSW4SM:
			return true;
		case DeviceID::HMLCSW4PCB:
			return true;
		case DeviceID::HMLCSW4WM:
			return true;
		case DeviceID::HMLCSW1FM:
			return true;
		case DeviceID::HMLCSWSCHUECO:
			return true;
		case DeviceID::HMLCSWSCHUECO2:
			return true;
		case DeviceID::HMLCSW2FM:
			return true;
		case DeviceID::HMLCSW1PBFM:
			return true;
		case DeviceID::HMLCSW2PBFM:
			return true;
		case DeviceID::HMLCSW4DR:
			return true;
		case DeviceID::HMLCSW2DR:
			return true;
		case DeviceID::HMLCSW1PBUFM:
			return true;
		case DeviceID::HMLCSW4BAPCB:
			return true;
		case DeviceID::HMLCSW1BAPCB:
			return true;
		case DeviceID::HMLCSW1PLOM54:
			return true;
		case DeviceID::HMLCSW1SMATMEGA168:
			return true;
		case DeviceID::HMLCSW4SMATMEGA168:
			return true;
		default:
			return false;
		}
	}
	return false;
}

bool LogicalDeviceType::isDimmer()
{
	if(_family == DeviceFamily::HomeMaticBidCoS)
	{
		switch(_id)
		{
		case DeviceID::HMLCDIM1TPL:
			return true;
		case DeviceID::HMLCDIM1TPL2:
			return true;
		case DeviceID::HMLCDIM1TCV:
			return true;
		case DeviceID::HMLCDIMSCHUECO:
			return true;
		case DeviceID::HMLCDIMSCHUECO2:
			return true;
		case DeviceID::HMLCDIM2TSM:
			return true;
		case DeviceID::HMLCDIM1TFM:
			return true;
		case DeviceID::HMLCDIM1LPL644:
			return true;
		case DeviceID::HMLCDIM1LCV644:
			return true;
		case DeviceID::HMLCDIM1PWMCV:
			return true;
		case DeviceID::HMLCDIM1TPL644:
			return true;
		case DeviceID::HMLCDIM1TCV644:
			return true;
		case DeviceID::HMLCDIM1TFM644:
			return true;
		case DeviceID::HMLCDIM1TPBUFM:
			return true;
		case DeviceID::HMLCDIM2LSM644:
			return true;
		case DeviceID::HMLCDIM2TSM644:
			return true;
		default:
			return false;
		}
	}
	return false;
}

DeviceTypes::DeviceTypes()
{
	_types[DeviceID::UNKNOWN] = LogicalDeviceType(DeviceID::UNKNOWN, DeviceFamily::none, 0xFFFFFFFF, "UNKNOWN");
	_types[DeviceID::HMSD] = LogicalDeviceType(DeviceID::HMSD, DeviceFamily::HomeMaticBidCoS, 0xFFFFFFFE, "HM-SD");
	_types[DeviceID::HMCENTRAL] = LogicalDeviceType(DeviceID::HMCENTRAL, DeviceFamily::HomeMaticBidCoS, 0xFFFFFFFD, "HM-CENTRAL");
	_types[DeviceID::HMRCV50] = LogicalDeviceType(DeviceID::HMRCV50, DeviceFamily::HomeMaticBidCoS, 0x0000, "HM-RCV-50");
	_types[DeviceID::HMLCSW1PLOM54] = LogicalDeviceType(DeviceID::HMLCSW1PLOM54, DeviceFamily::HomeMaticBidCoS, 0x0001, "HM-LC-Sw1-Pl-OM54");
	_types[DeviceID::HMLCSW1SM] = LogicalDeviceType(DeviceID::HMLCSW1SM, DeviceFamily::HomeMaticBidCoS, 0x0002, "HM-LC-Sw1-SM");
	_types[DeviceID::HMLCSW4SM] = LogicalDeviceType(DeviceID::HMLCSW4SM, DeviceFamily::HomeMaticBidCoS, 0x0003, "HM-LC-Sw4-SM");
	_types[DeviceID::HMLCSW1FM] = LogicalDeviceType(DeviceID::HMLCSW1FM, DeviceFamily::HomeMaticBidCoS, 0x0004, "HM-LC-Sw1-FM");
	_types[DeviceID::HMRC4] = LogicalDeviceType(DeviceID::HMRC4, DeviceFamily::HomeMaticBidCoS, 0x0008, "HM-RC-4");
	_types[DeviceID::HMLCSW2FM] = LogicalDeviceType(DeviceID::HMLCSW2FM, DeviceFamily::HomeMaticBidCoS, 0x0009, "HM-LC-Sw2-FM");
	_types[DeviceID::HMLCSW2SM] = LogicalDeviceType(DeviceID::HMLCSW2SM, DeviceFamily::HomeMaticBidCoS, 0x000A, "HM-LC-Sw2-SM");
	_types[DeviceID::HMLCSW1PL] = LogicalDeviceType(DeviceID::HMLCSW1PL, DeviceFamily::HomeMaticBidCoS, 0x0011, "HM-LC-Sw1-Pl");
	_types[DeviceID::HMLCSW1SMATMEGA168] = LogicalDeviceType(DeviceID::HMLCSW1SMATMEGA168, DeviceFamily::HomeMaticBidCoS, 0x0014, "HM-LC-Sw1-SM-ATmega168");
	_types[DeviceID::HMLCSW4SMATMEGA168] = LogicalDeviceType(DeviceID::HMLCSW4SMATMEGA168, DeviceFamily::HomeMaticBidCoS, 0x0015, "HM-LC-Sw4-SM-ATmega168");
	_types[DeviceID::HMRCP1] = LogicalDeviceType(DeviceID::HMRCP1, DeviceFamily::HomeMaticBidCoS, 0x001A, "HM-RC-Pl");
	_types[DeviceID::HMRCSEC3] = LogicalDeviceType(DeviceID::HMRCSEC3, DeviceFamily::HomeMaticBidCoS, 0x001B, "HM-RC-Sec3");
	_types[DeviceID::HMRCSEC3B] = LogicalDeviceType(DeviceID::HMRCSEC3B, DeviceFamily::HomeMaticBidCoS, 0x001C, "HM-RC-Sec3-B");
	_types[DeviceID::HMRCKEY3] = LogicalDeviceType(DeviceID::HMRCKEY3, DeviceFamily::HomeMaticBidCoS, 0x001D, "HM-RC-Key3");
	_types[DeviceID::HMRCKEY3B] = LogicalDeviceType(DeviceID::HMRCKEY3B, DeviceFamily::HomeMaticBidCoS, 0x001E, "HM-RC-Key3-B");
	_types[DeviceID::HMRC12] = LogicalDeviceType(DeviceID::HMRC12, DeviceFamily::HomeMaticBidCoS, 0x0029, "HM-RC-12");
	_types[DeviceID::HMRC12B] = LogicalDeviceType(DeviceID::HMRC12B, DeviceFamily::HomeMaticBidCoS, 0x002A, "HM-RC-12-B");
	_types[DeviceID::HMLCSW4PCB] = LogicalDeviceType(DeviceID::HMLCSW4PCB, DeviceFamily::HomeMaticBidCoS, 0x002D, "HM-LC-Sw4-PCB");
	_types[DeviceID::HMSECSC] = LogicalDeviceType(DeviceID::HMSECSC, DeviceFamily::HomeMaticBidCoS, 0x002F, "HM-Sec-SC");
	_types[DeviceID::HMSECRHS] = LogicalDeviceType(DeviceID::HMSECRHS, DeviceFamily::HomeMaticBidCoS, 0x0030, "HM-Sec-RHS");
	_types[DeviceID::HMPB4WM] = LogicalDeviceType(DeviceID::HMPB4WM, DeviceFamily::HomeMaticBidCoS, 0x0035, "HM-PB4-WM");
	_types[DeviceID::HMPB2WM] = LogicalDeviceType(DeviceID::HMPB2WM, DeviceFamily::HomeMaticBidCoS, 0x0036, "HM-PB2-WM");
	_types[DeviceID::HMRC19] = LogicalDeviceType(DeviceID::HMRC19, DeviceFamily::HomeMaticBidCoS, 0x0037, "HM-RC-19");
	_types[DeviceID::HMRC19B] = LogicalDeviceType(DeviceID::HMRC19B, DeviceFamily::HomeMaticBidCoS, 0x0038, "HM-RC-19-B");
	_types[DeviceID::HMCCTC] = LogicalDeviceType(DeviceID::HMCCTC, DeviceFamily::HomeMaticBidCoS, 0x0039, "HM-CC-TC");
	_types[DeviceID::HMCCVD] = LogicalDeviceType(DeviceID::HMCCVD, DeviceFamily::HomeMaticBidCoS, 0x003A, "HM-CC-VD");
	_types[DeviceID::HMRC4B] = LogicalDeviceType(DeviceID::HMRC4B, DeviceFamily::HomeMaticBidCoS, 0x003B, "HM-RC-4-B");
	_types[DeviceID::HMWDS30TO] = LogicalDeviceType(DeviceID::HMWDS30TO, DeviceFamily::HomeMaticBidCoS, 0x003E, "HM-WDS30-T-O");
	_types[DeviceID::HMWDS100C6O] = LogicalDeviceType(DeviceID::HMWDS100C6O, DeviceFamily::HomeMaticBidCoS, 0x0040, "HM-WDS100-C6-O");
	_types[DeviceID::HMSECSD] = LogicalDeviceType(DeviceID::HMSECSD, DeviceFamily::HomeMaticBidCoS, 0x0042, "HM-Sec-SD");
	_types[DeviceID::HMSENEP] = LogicalDeviceType(DeviceID::HMSENEP, DeviceFamily::HomeMaticBidCoS, 0x0044, "HM-Sen-EP");
	_types[DeviceID::HMSWI3FM] = LogicalDeviceType(DeviceID::HMSWI3FM, DeviceFamily::HomeMaticBidCoS, 0x0046, "HM-SwI-3-FM");
	_types[DeviceID::HMSECMDIR] = LogicalDeviceType(DeviceID::HMSECMDIR, DeviceFamily::HomeMaticBidCoS, 0x004A, "HM-Sec-MDIR");
	_types[DeviceID::HMRC12SW] = LogicalDeviceType(DeviceID::HMRC12SW, DeviceFamily::HomeMaticBidCoS, 0x004C, "HM-RC-12-SW");
	_types[DeviceID::HMRC19SW] = LogicalDeviceType(DeviceID::HMRC19SW, DeviceFamily::HomeMaticBidCoS, 0x004D, "HM-RC-19-SW");
	_types[DeviceID::HMSENMDIRSM] = LogicalDeviceType(DeviceID::HMSENMDIRSM, DeviceFamily::HomeMaticBidCoS, 0x004F, "HM-Sen-MDIR-SM");
	_types[DeviceID::HMLCSW1PBFM] = LogicalDeviceType(DeviceID::HMLCSW1PBFM, DeviceFamily::HomeMaticBidCoS, 0x0051, "HM-LC-Sw1-PB-FM");
	_types[DeviceID::HMLCSW2PBFM] = LogicalDeviceType(DeviceID::HMLCSW2PBFM, DeviceFamily::HomeMaticBidCoS, 0x0052, "HM-LC-Sw2-PB-FM");
	_types[DeviceID::RCH] = LogicalDeviceType(DeviceID::RCH, DeviceFamily::HomeMaticBidCoS, 0x0054, "RCH");
	_types[DeviceID::HMLCDIM1TPL] = LogicalDeviceType(DeviceID::HMLCDIM1TPL, DeviceFamily::HomeMaticBidCoS, 0x0057, "HM-LC-Dim1T-PL");
	_types[DeviceID::HMLCDIM1TCV] = LogicalDeviceType(DeviceID::HMLCDIM1TCV, DeviceFamily::HomeMaticBidCoS, 0x0058, "HM-LC-Dim1T-CV");
	_types[DeviceID::HMLCDIM1TFM] = LogicalDeviceType(DeviceID::HMLCDIM1TFM, DeviceFamily::HomeMaticBidCoS, 0x0059, "HM-LC-Dim1T-FM");
	_types[DeviceID::HMLCDIM2TSM] = LogicalDeviceType(DeviceID::HMLCDIM2TSM, DeviceFamily::HomeMaticBidCoS, 0x005A, "HM-LC-Dim2T-SM");
	_types[DeviceID::HMSENMDIRO] = LogicalDeviceType(DeviceID::HMSENMDIRO, DeviceFamily::HomeMaticBidCoS, 0x005D, "HM-Sen-MDIR-O");
	_types[DeviceID::HMSCI3FM] = LogicalDeviceType(DeviceID::HMSCI3FM, DeviceFamily::HomeMaticBidCoS, 0x005F, "HM-SCI-3-FM");
	_types[DeviceID::HMPB4DISWM] = LogicalDeviceType(DeviceID::HMPB4DISWM, DeviceFamily::HomeMaticBidCoS, 0x0060, "HM-PB-4Dis-WM");
	_types[DeviceID::HMLCSW4DR] = LogicalDeviceType(DeviceID::HMLCSW4DR, DeviceFamily::HomeMaticBidCoS, 0x0061, "HM-LC-Sw4-DR");
	_types[DeviceID::HMLCSW2DR] = LogicalDeviceType(DeviceID::HMLCSW2DR, DeviceFamily::HomeMaticBidCoS, 0x0062, "HM-LC-Sw2-DR");
	_types[DeviceID::ATENT] = LogicalDeviceType(DeviceID::ATENT, DeviceFamily::HomeMaticBidCoS, 0x0064, "Atent");
	_types[DeviceID::HMLCSW4WM] = LogicalDeviceType(DeviceID::HMLCSW4WM, DeviceFamily::HomeMaticBidCoS, 0x0066, "HM-LC-Sw4-WM");
	_types[DeviceID::HMLCDIM1PWMCV] = LogicalDeviceType(DeviceID::HMLCDIM1PWMCV, DeviceFamily::HomeMaticBidCoS, 0x0067, "HM-LC-Dim1PWM-CV");
	_types[DeviceID::HMLCDIM1TPBUFM] = LogicalDeviceType(DeviceID::HMLCDIM1TPBUFM, DeviceFamily::HomeMaticBidCoS, 0x0068, "HM-LC-Dim1TPBU-FM");
	_types[DeviceID::HMLCSW1PBUFM] = LogicalDeviceType(DeviceID::HMLCSW1PBUFM, DeviceFamily::HomeMaticBidCoS, 0x0069, "HM-LC-Sw1PBU-FM");
	_types[DeviceID::HMLCSW1BAPCB] = LogicalDeviceType(DeviceID::HMLCSW1BAPCB, DeviceFamily::HomeMaticBidCoS, 0x006C, "HM-LC-Sw1-Ba-PCB");
	_types[DeviceID::HMLCDIM1LCV644] = LogicalDeviceType(DeviceID::HMLCDIM1LCV644, DeviceFamily::HomeMaticBidCoS, 0x006E, "HM-LC-Dim1L-CV-644");
	_types[DeviceID::HMLCDIM1LPL644] = LogicalDeviceType(DeviceID::HMLCDIM1LPL644, DeviceFamily::HomeMaticBidCoS, 0x006F, "HM-LC-Dim1L-PL-644");
	_types[DeviceID::HMLCDIM2LSM644] = LogicalDeviceType(DeviceID::HMLCDIM2LSM644, DeviceFamily::HomeMaticBidCoS, 0x0070, "HM-LC-Dim2L-SM-644");
	_types[DeviceID::HMLCDIM1TPL644] = LogicalDeviceType(DeviceID::HMLCDIM1TPL644, DeviceFamily::HomeMaticBidCoS, 0x0071, "HM-LC-Dim1T-PL-644");
	_types[DeviceID::HMLCDIM1TCV644] = LogicalDeviceType(DeviceID::HMLCDIM1TCV644, DeviceFamily::HomeMaticBidCoS, 0x0072, "HM-LC-Dim1T-CV-644");
	_types[DeviceID::HMLCDIM1TFM644] = LogicalDeviceType(DeviceID::HMLCDIM1TFM644, DeviceFamily::HomeMaticBidCoS, 0x0073, "HM-LC-Dim1T-FM-644");
	_types[DeviceID::HMLCDIM2TSM644] = LogicalDeviceType(DeviceID::HMLCDIM2TSM644, DeviceFamily::HomeMaticBidCoS, 0x0074, "HM-LC-Dim2T-SM-644");
	//_types[DeviceID::HMOUCFMPL] = LogicalDeviceType(DeviceID::HMOUCFMPL, DeviceFamily::HomeMaticBidCoS, 0x0075, "HM-OU-CFM-Pl");
	_types[DeviceID::ZELSTGRMHS4] = LogicalDeviceType(DeviceID::ZELSTGRMHS4, DeviceFamily::HomeMaticBidCoS, 0x0080, "ZEL STG RM HS 4");
	_types[DeviceID::HMSWI3FMROTO] = LogicalDeviceType(DeviceID::HMSWI3FMROTO, DeviceFamily::HomeMaticBidCoS, 0x0083, "ZEL STG RM FSS UP3");
	_types[DeviceID::HMLCDIMSCHUECO] = LogicalDeviceType(DeviceID::HMLCDIMSCHUECO, DeviceFamily::HomeMaticBidCoS, 0x0089, "263 134");
	_types[DeviceID::HMLCDIMSCHUECO2] = LogicalDeviceType(DeviceID::HMLCDIMSCHUECO2, DeviceFamily::HomeMaticBidCoS, 0x008A, "263 133");
	_types[DeviceID::HMLCSWSCHUECO] = LogicalDeviceType(DeviceID::HMLCSWSCHUECO, DeviceFamily::HomeMaticBidCoS, 0x008B, "263 130");
	_types[DeviceID::HMLCSWSCHUECO2] = LogicalDeviceType(DeviceID::HMLCSWSCHUECO2, DeviceFamily::HomeMaticBidCoS, 0x008C, "263 131");
	_types[DeviceID::HMSECMDIRSCHUECO] = LogicalDeviceType(DeviceID::HMSECMDIRSCHUECO, DeviceFamily::HomeMaticBidCoS, 0x0090, "263 162");
	_types[DeviceID::HMSWI3FMSCHUECO] = LogicalDeviceType(DeviceID::HMSWI3FMSCHUECO, DeviceFamily::HomeMaticBidCoS, 0x0092, "263 144");
	_types[DeviceID::HMRC42] = LogicalDeviceType(DeviceID::HMRC42, DeviceFamily::HomeMaticBidCoS, 0x00A0, "HM-RC-4-2");
	_types[DeviceID::HMLCSW1PL2] = LogicalDeviceType(DeviceID::HMLCSW1PL2, DeviceFamily::HomeMaticBidCoS, 0x00A1, "HM-LC-Sw1-Pl-2");
	_types[DeviceID::HMLCDIM1TPL2] = LogicalDeviceType(DeviceID::HMLCDIM1TPL2, DeviceFamily::HomeMaticBidCoS, 0x00A4, "HM-LC-Dim1T-Pl-2");
	_types[DeviceID::HMRCSEC42] = LogicalDeviceType(DeviceID::HMRCSEC42, DeviceFamily::HomeMaticBidCoS, 0x00A5, "HM-RC-Sec4-2");
	_types[DeviceID::HMRCKEY42] = LogicalDeviceType(DeviceID::HMRCKEY42, DeviceFamily::HomeMaticBidCoS, 0x00A6, "HM-RC-Key4-2");
	_types[DeviceID::HMPB6WM55] = LogicalDeviceType(DeviceID::HMPB6WM55, DeviceFamily::HomeMaticBidCoS, 0x00A9, "HM-PB6-WM55");
	_types[DeviceID::HMLCSW4BAPCB] = LogicalDeviceType(DeviceID::HMLCSW4BAPCB, DeviceFamily::HomeMaticBidCoS, 0x00AB, "HM-LC-Sw4-Ba-PCB");
	_types[DeviceID::HMESPMSW1PL] = LogicalDeviceType(DeviceID::HMESPMSW1PL, DeviceFamily::HomeMaticBidCoS, 0x00AC, "HM-ES-PMSw1-Pl");
	_types[DeviceID::HMSECSC2] = LogicalDeviceType(DeviceID::HMSECSC2, DeviceFamily::HomeMaticBidCoS, 0x00B1, "HM-Sec-SC-2");

	for(std::map<DeviceID, LogicalDeviceType>::iterator i = _types.begin(); i != _types.end(); ++i)
	{
		_types2[i->second.family()][i->second.type()] = i->second;
	}
}

LogicalDeviceType DeviceTypes::get(DeviceID id)
{
	if(_types.find(id) != _types.end()) return _types.at(id);
	return LogicalDeviceType();
}

LogicalDeviceType DeviceTypes::get(DeviceFamily family, uint32_t type)
{
	if(_types2.find(family) != _types2.end() && _types2.at(family).find(type) != _types2.at(family).end()) return _types2.at(family).at(type);
	return LogicalDeviceType();
}
