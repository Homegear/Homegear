#ifndef DEVICE_H_
#define DEVICE_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "rapidxml.hpp"
#include "LogicalParameter.h"
#include "PhysicalParameter.h"
#include "../BidCoSPacket.h"
#include "../HMDeviceTypes.h"

using namespace rapidxml;

namespace XMLRPC {

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
		enum Enum { none, floatIntegerScale, integerIntegerScale, booleanInteger, integerIntegerMap };
	};
	Type::Enum type = Type::Enum::none;
	std::unordered_map<int32_t, int32_t> integerIntegerMap;
	double factor = 0;
	int32_t div = 0;
	int32_t mul = 0;
	int32_t threshold = 0;
	int32_t valueFalse = 0;
	int32_t valueTrue = 0;

	ParameterConversion() {}
	ParameterConversion(xml_node<>* node);
	virtual ~ParameterConversion() {}
};

class Parameter
{
private:
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
		enum Enum { none = 0, service = 1, sticky = 2 };
	};
	double index = 0;
	double size = 0;
	bool isSigned = false;
	BooleanOperator::Enum booleanOperator = BooleanOperator::Enum::e;
	Operations::Enum operations = Operations::Enum::none;
	UIFlags::Enum uiFlags = UIFlags::Enum::none;
	uint32_t constValue = 0;
	std::string id;
	std::string param;
	std::string control;
	LogicalParameter logicalParameter;
	PhysicalParameter physicalParameter;
	ParameterConversion conversion;
	ParameterDescription description;

	Parameter() {}
	Parameter(xml_node<>* node);
	virtual ~Parameter() {}
	bool checkCondition(int64_t value);
};

class DeviceType
{
public:
	std::string name;
	std::string id;
	std::vector<Parameter> parameters;

	DeviceType() {}
	DeviceType(xml_node<>* typeNode);
	virtual ~DeviceType() {}

	bool matches(BidCoSPacket* packet);
	bool matches(HMDeviceTypes deviceType, uint32_t firmwareVersion);
};

class ParameterSet
{
public:
	struct Type
	{
		enum Enum { none = 0, master = 1, values = 2, link = 3 };
	};
	Type::Enum type = Type::Enum::none;
	std::string id;
	std::vector<Parameter> parameters;

	ParameterSet() {}
	ParameterSet(xml_node<>* parameterSetNode);
	virtual ~ParameterSet() {}
	void init(xml_node<>* parameterSetNode);
};

class EnforceLink
{
public:
	struct ID
	{
		enum Enum { none = 0, peerNeedsBurst = 1 };
	};
	ID::Enum id = ID::Enum::none;
	bool value = false;

	EnforceLink() {}
	EnforceLink(xml_node<>* parameterSetNode);
	virtual ~EnforceLink() {}
};

class LinkRole
{
public:
	std::string sourceName;
	std::string targetName;

	LinkRole() {}
	LinkRole(xml_node<>* parameterSetNode);
	virtual ~LinkRole() {}
};

class DeviceChannel
{
public:
	struct UIFlags
	{
		enum Enum { none = 0, internal = 1 };
	};

	uint32_t index = 0;
	std::string type;
	UIFlags::Enum uiFlags = UIFlags::Enum::none;
	std::string channelClass;
	uint32_t count = 0;
	std::vector<ParameterSet> parameterSets;
	std::unordered_map<uint32_t, ParameterSet*> parameterSetsByType;
	std::vector<LinkRole> linkRoles;
	std::vector<EnforceLink> enforceLinks;

	DeviceChannel() {}
	DeviceChannel(xml_node<>* node);
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
	uint32_t subtype = 0;
	uint32_t subtypeIndex = 0;
	uint32_t channelField = 0;
	uint32_t fixedChannel = 0;
	std::vector<Parameter> parameters;

	DeviceFrame() {}
	DeviceFrame(xml_node<>* node);
	virtual ~DeviceFrame() {}
};

class Device
{
public:
	struct RXModes
	{
		enum Enum { none = 0, config = 1, wakeUp = 2 };
	};

	uint32_t version = 0;
	uint32_t cyclicTimeout = 0;
	ParameterSet parameterSet;
	std::vector<DeviceChannel> channels;
	std::vector<DeviceType> supportedTypes;
	std::vector<DeviceFrame> frames;
	RXModes::Enum rxModes = RXModes::Enum::none;

	Device() {}
	Device(std::string xmlFilename);
	virtual ~Device();
protected:
	virtual void load(std::string xmlFilename);
	virtual void parseXML(xml_document<>* doc);
private:
};

}
#endif /* DEVICE_H_ */
