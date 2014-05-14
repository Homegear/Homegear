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

#ifndef DEVICE_H_
#define DEVICE_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <cmath>
#include <memory>

#include "../Systems/Packet.h"
#include "../Encoding/rapidxml.hpp"
#include "../Systems/DeviceTypes.h"
#include "LogicalParameter.h"
#include "PhysicalParameter.h"

using namespace rapidxml;

namespace BaseLib
{
namespace RPC
{

class ParameterSet;
class Device;
class RPCVariable;

class DescriptionField
{
public:
	std::string id;
	std::string value;

	DescriptionField() {}
	DescriptionField(xml_node<>* node);
	virtual ~DescriptionField() {}
};

class ParameterDescription
{
public:
	std::vector<DescriptionField> fields;

	ParameterDescription() {}
	ParameterDescription(xml_node<>* node);
	virtual ~ParameterDescription() {}
};

class ParameterConversion
{
public:
	struct Type
	{
		enum Enum { none, toggle, floatIntegerScale, integerIntegerScale, booleanInteger, integerIntegerMap, floatConfigTime, optionInteger, integerTinyFloat, stringUnsignedInteger, blindTest, cfm };
	};
	Type::Enum type = Type::Enum::none;
	std::unordered_map<int32_t, int32_t> integerValueMapDevice;
	std::unordered_map<int32_t, int32_t> integerValueMapParameter;
	double factor = 0;
	std::vector<double> factors;
	int32_t div = 0;
	int32_t mul = 0;
	int32_t threshold = 0;
	int32_t valueFalse = 0;
	int32_t valueTrue = 0;
	double offset = 0;
	double valueSize = 0;
	int32_t mantissaStart = 5;
	int32_t mantissaSize = 11;
	int32_t exponentStart = 0;
	int32_t exponentSize = 5;
	std::string stringValue;
	int32_t on = 200;
	int32_t off = 0;
	bool invert = false;
	bool fromDevice = true;
	bool toDevice = true;

	ParameterConversion();
	ParameterConversion(xml_node<>* node);
	virtual ~ParameterConversion() {}
	virtual void fromPacket(std::shared_ptr<RPCVariable> value);
	virtual void toPacket(std::shared_ptr<RPCVariable> value);
};

class Parameter
{
public:
	struct BooleanOperator
	{
		enum Enum { e, g, l, ge, le };
	};
	struct Operations
	{
		enum Enum { none = 0, read = 1, write = 2, event = 4 };
	};
	struct UIFlags
	{
		enum Enum { none = 0, visible = 1, internal = 2, transform = 4, service = 8, sticky = 0x10, invisible = 0x20 };
	};
	uint32_t _bitmask[8] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
	ParameterSet* parentParameterSet = nullptr;
	double index = 0;
	double size = 0;
	bool isSigned = false;
	bool hidden = false;
	BooleanOperator::Enum booleanOperator = BooleanOperator::Enum::e;
	Operations::Enum operations = (Operations::Enum)3;
	UIFlags::Enum uiFlags = UIFlags::Enum::visible;
	PhysicalParameter::Type::Enum type = PhysicalParameter::Type::Enum::none;
	int32_t constValue = -1;
	std::string id;
	std::string param;
	std::string additionalParameter;
	std::string control;
	std::shared_ptr<LogicalParameter> logicalParameter;
	std::shared_ptr<PhysicalParameter> physicalParameter;
	std::vector<std::shared_ptr<ParameterConversion>> conversion;
	ParameterDescription description;
	bool omitIfSet = false;
	int32_t omitIf = 0;
	bool loopback = false;
	bool hasDominoEvents = false;

	Parameter();
	Parameter(xml_node<>* node, bool checkForID = false);
	virtual ~Parameter() {}
	virtual bool checkCondition(int32_t value);
	virtual std::shared_ptr<RPC::RPCVariable> convertFromPacket(const std::vector<uint8_t>& data, bool isEvent = false);
	virtual std::vector<uint8_t> convertToPacket(std::shared_ptr<RPC::RPCVariable> value);
	virtual std::vector<uint8_t> convertToPacket(std::string value);
	virtual std::vector<uint8_t> reverseData(const std::vector<uint8_t>& data);
	virtual void adjustBitPosition(std::vector<uint8_t>& data);
};

class DeviceType
{
public:
	std::string name;
	std::string id;
	std::vector<Parameter> parameters;
	bool updatable = false;
	int32_t priority = 0;
	Device* device = nullptr;

	DeviceType();
	DeviceType(xml_node<>* typeNode);
	virtual ~DeviceType() {}

	virtual bool matches(Systems::DeviceFamilies family, std::shared_ptr<Systems::Packet> packet);
	virtual bool matches(Systems::DeviceFamilies family, std::string typeID);
	virtual bool matches(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion);
};

typedef std::vector<std::pair<std::string, std::string>> DefaultValue;
class ParameterSet
{
public:
	struct Type
	{
		enum Enum { none = 0, master = 1, values = 2, link = 3 };
	};
	Type::Enum type = Type::Enum::none;
	std::string id;
	std::vector<std::shared_ptr<Parameter>> parameters;
	std::map<std::string, DefaultValue> defaultValues;
	std::map<uint32_t, uint32_t> lists;
	std::string subsetReference;
	int32_t addressStart = -1;
	int32_t addressStep = -1;
	int32_t count = -1;
	int32_t channelOffset = -1;
	int32_t peerAddressOffset = -1;
	int32_t peerChannelOffset = -1;

	ParameterSet();
	ParameterSet(xml_node<>* parameterSetNode);
	virtual ~ParameterSet() {}
	virtual void init(xml_node<>* parameterSetNode);
	virtual std::vector<std::shared_ptr<Parameter>> getIndices(uint32_t startIndex, uint32_t endIndex, int32_t list);
	virtual std::shared_ptr<Parameter> getIndex(double index);
	virtual std::shared_ptr<Parameter> getParameter(std::string id);
	virtual std::string typeString();
	static ParameterSet::Type::Enum typeFromString(std::string type);
};

class EnforceLink
{
public:
	std::string id;
	std::string value;

	EnforceLink();
	EnforceLink(xml_node<>* parameterSetNode);
	virtual ~EnforceLink() {}
	virtual std::shared_ptr<RPCVariable> getValue(LogicalParameter::Type::Enum type);
};

class LinkRole
{
public:
	std::vector<std::string> sourceNames;
	std::vector<std::string> targetNames;

	LinkRole() {}
	LinkRole(xml_node<>* parameterSetNode);
	virtual ~LinkRole() {}
};

class DeviceChannel
{
public:
	struct UIFlags
	{
		enum Enum { none = 0, visible = 1, internal = 2, dontdelete = 8 };
	};
	struct Direction
	{
		enum Enum { none = 0, sender = 1, receiver = 2 };
	};
	Device* parentDevice = nullptr;
	int32_t physicalIndexOffset = 0;
	std::string type;
	UIFlags::Enum uiFlags = UIFlags::Enum::visible;
	Direction::Enum direction = Direction::Enum::none;
	std::string channelClass;
	uint32_t startIndex = 0;
	uint32_t count = 1;
	bool hasTeam = false;
	bool aesDefault = false;
	bool aesAlways = false;
	bool aesCBC = false;
	bool hidden = false;
	bool autoregister = false;
	bool paired = false;
	double countFromSysinfo = -1;
	double countFromSysinfoSize = 1;
	std::string function;
	std::string pairFunction1;
	std::string pairFunction2;
	std::string teamTag;
	std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>> parameterSets;
	std::shared_ptr<LinkRole> linkRoles;
	std::vector<std::shared_ptr<EnforceLink>> enforceLinks;
	std::shared_ptr<Parameter> specialParameter;
	std::shared_ptr<DeviceChannel> subconfig;

	DeviceChannel();
	DeviceChannel(xml_node<>* node, uint32_t& index);
	virtual ~DeviceChannel() {}
};

class DeviceFrame
{
public:
	struct Direction
	{
		enum Enum { none, toDevice, fromDevice };
	};
	struct AllowedReceivers
	{
		enum Enum { none = 0, broadcast = 1, central = 2, other = 4 };
	};

	Direction::Enum direction = Direction::Enum::none;
	AllowedReceivers::Enum allowedReceivers = AllowedReceivers::Enum::none;
	std::string id;
	bool isEvent = false;
	uint32_t type = 0;
	int32_t subtype = -1;
	int32_t subtypeIndex = -1;
	int32_t channelField = -1;
	double channelFieldSize = 1.0;
	int32_t channelIndexOffset = 0;
	int32_t fixedChannel = -1;
	std::vector<Parameter> parameters;
	std::vector<std::shared_ptr<Parameter>> associatedValues;

	DeviceFrame() {}
	DeviceFrame(xml_node<>* node);
	virtual ~DeviceFrame() {}
};

class Device
{
public:
	struct UIFlags
	{
		enum Enum { none = 0, visible = 1, internal = 2, dontdelete = 8 };
	};
	struct RXModes
	{
		enum Enum { none = 0, always = 1, burst = 2, config = 4, wakeUp = 8, lazyConfig = 16 };
	};

	bool loaded() { return _loaded; }
	bool hasBattery = false;
	Systems::DeviceFamilies family = Systems::DeviceFamilies::HomeMaticBidCoS;
	uint32_t version = 0;
	uint32_t cyclicTimeout = 0;
	int32_t eepSize = 1024;
	std::shared_ptr<ParameterSet> parameterSet;
	std::map<uint32_t, std::shared_ptr<DeviceChannel>> channels;
	std::vector<std::shared_ptr<DeviceType>> supportedTypes;
	std::multimap<uint32_t, std::shared_ptr<DeviceFrame>> framesByMessageType;
	std::map<std::string, std::shared_ptr<DeviceFrame>> framesByID;
	RXModes::Enum rxModes = RXModes::Enum::always;
	UIFlags::Enum uiFlags = UIFlags::Enum::visible;
	int32_t countFromSysinfoIndex = -1;
	double countFromSysinfoSize = 1;
	std::string deviceClass;
	bool supportsAES = false;
	bool peeringSysinfoExpectChannel = true;
	std::shared_ptr<Device> team;

	Device();
	Device(std::string xmlFilename);
	virtual ~Device();
	virtual std::shared_ptr<DeviceType> getType(Systems::LogicalDeviceType deviceType, int32_t firmwareVersion);
	virtual int32_t getCountFromSysinfo() { return _countFromSysinfo; }
	virtual int32_t getCountFromSysinfo(std::shared_ptr<Systems::Packet> packet);
	virtual void setCountFromSysinfo(int32_t countFromSysinfo);
protected:
	bool _loaded = false;
	int32_t _countFromSysinfo = -1;

	virtual void load(std::string xmlFilename);
	virtual void parseXML(xml_node<>* node);
};
}
}
#endif /* DEVICE_H_ */
