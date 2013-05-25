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

using namespace rapidxml;

namespace XMLRPC {

class ParameterConversion
{
public:
	struct Type
	{
		enum Enum { none, floatIntegerScale, integerIntegerScale, booleanInteger, integerIntegerMap };
	};
	Type::Enum type = Type::Enum::none;
	unordered_map<int32_t, int32_t> integerIntegerMap;
	double factor = 0;
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
	struct ConditionalOperator
	{
		enum Enum { none = 0, e = 1, g = 2, l = 3, ge = 4, le = 5 };
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
	ConditionalOperator::Enum conditionalOperator = ConditionalOperator::Enum::none;
	Operations::Enum operations = Operations::Enum::none;
	UIFlags::Enum uiFlags = UIFlags::Enum::none;
	uint32_t constValue = 0;
	std::string id;
	std::string param;

	Parameter() {}
	Parameter(xml_node<>* parameterNode);
	virtual ~Parameter() {}
};

class DeviceType
{
public:
	std::string name = "";
	std::string id = "";
	std::vector<Parameter> parameters;

	DeviceType() {}
	DeviceType(xml_node<>* typeNode);
	virtual ~DeviceType() {}
};

class ParameterSet
{
public:
	struct Type
	{
		enum Enum { none = 0, master = 1, values = 2, link = 3 };
	};
	Type::Enum type = Type::Enum::none;
	std::string id = "";
	std::vector<Parameter> parameters;

	ParameterSet() {}
	ParameterSet(xml_node<>* parameterSetNode);
	virtual ~ParameterSet() {}
	void init(xml_node<>* parameterSetNode);
};

class Device
{
public:
	struct RXModes
	{
		enum Enum { none = 0, config = 1, wakeUp = 2 };
	};

	std::vector<DeviceType>* supportedTypes() { return &_supportedTypes; }
	ParameterSet parameterSet;

	Device() {}
	Device(std::string xmlFilename);
	virtual ~Device();
	uint32_t version() { return _version; }
	uint32_t cyclicTimeout() { return _cyclicTimeout; }
	RXModes::Enum rxModes() { return _rxModes; }
protected:
	uint32_t _version = 0;
	uint32_t _cyclicTimeout = 0;
	RXModes::Enum _rxModes = RXModes::Enum::none;
	std::vector<DeviceType> _supportedTypes;

	virtual void load(std::string xmlFilename);
	virtual void parseXML(xml_document<>* doc);
private:
};

}
#endif /* DEVICE_H_ */
