/* Copyright 2013-2015 Sathya Laufer
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

#ifndef HOMEGEARDEVICE_H_
#define HOMEGEARDEVICE_H_

#include "DevicePacket.h"
#include "SupportedDevice.h"
#include "RunProgram.h"
#include "Function.h"

using namespace rapidxml;

namespace BaseLib
{
namespace DeviceDescription
{

class HomegearDevice;

typedef std::shared_ptr<HomegearDevice> PHomegearDevice;

class HomegearDevice
{
public:
	struct ReceiveModes
	{
		enum Enum { none = 0, always = 1, wakeOnRadio = 2, config = 4, wakeUp = 8, lazyConfig = 16, wakeUp2 = 32 };
	};

	Systems::DeviceFamilies family = Systems::DeviceFamilies::none;

	HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies family);
	HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily, xml_node<>* node);
	HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies family, std::string xmlFilename, bool& oldFormat);
	virtual ~HomegearDevice();

	bool loaded() { return _loaded; }

	// {{{ Shortcuts
	int32_t dynamicChannelCountIndex = -1;
	double dynamicChannelCountSize = 1;
	// }}}

	// {{{ Attributes
	int32_t version = 0;
	// }}}

	// {{{ Properties
	ReceiveModes::Enum receiveModes = ReceiveModes::always;
	bool encryption = false;
	uint32_t timeout = 0;
	uint32_t memorySize = 1024;
	bool visible = true;
	bool deletable = true;
	bool internal = false;
	bool needsTime = false;
	bool hasBattery = false;
	// }}}

	// {{{ Elements
	SupportedDevices supportedDevices;
	PRunProgram runProgram;
	Functions functions;
	PacketsByMessageType packetsByMessageType;
	PacketsById packetsById;
	PacketsByFunction packetsByFunction1;
	PacketsByFunction packetsByFunction2;
	ValueRequestPackets valueRequestPackets;
	PHomegearDevice group;
	// }}}

	// {{{ Helpers
	int32_t getDynamicChannelCount();
	void setDynamicChannelCount(int32_t value);
	PSupportedDevice getType(Systems::LogicalDeviceType deviceType, int32_t firmwareVersion);
	void save(std::string& filename);
	// }}}
protected:
	BaseLib::Obj* _bl = nullptr;
	bool _loaded = false;

	// {{{ Helpers
	int32_t _dynamicChannelCount = -1;
	// }}}

	virtual void load(std::string xmlFilename, bool& oldFormat);
	virtual void postProcessFunction(PFunction& function, std::map<std::string, PConfigParameters>& configParameters, std::map<std::string, PVariables>& variables, std::map<std::string, PLinkParameters>& linkParameters);
	virtual void parseXML(xml_node<>* node);

	// {{{ Helpers
	void saveDevice(xml_document<>* doc, xml_node<>* parentNode, HomegearDevice* device, std::vector<std::string>& tempStrings);
	void saveFunction(xml_document<>* doc, xml_node<>* parentNode, PFunction& function, std::vector<std::string>& tempStrings, std::map<std::string, PConfigParameters>& configParameters, std::map<std::string, PVariables>& variables, std::map<std::string, PLinkParameters>& linkParameters);
	void saveParameter(xml_document<>* doc, xml_node<>* parentNode, PParameter& parameter, std::vector<std::string>& tempStrings);
	void saveScenario(xml_document<>* doc, xml_node<>* parentNode, PScenario& scenario, std::vector<std::string>& tempStrings);
	void saveParameterPacket(xml_document<>* doc, xml_node<>* parentNode, std::shared_ptr<Parameter::Packet>& packet, std::vector<std::string>& tempStrings);
	// }}}
};
}
}

#endif
