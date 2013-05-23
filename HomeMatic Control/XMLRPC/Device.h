#ifndef DEVICE_H_
#define DEVICE_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include "rapidxml.hpp"

using namespace rapidxml;

namespace XMLRPC {

class Parameter
{
private:
public:
	struct ConditionalOperator
	{
		enum Enum { none = 0, e = 1, g = 2, l = 3, ge = 4, le = 5 };
	};
	double index = 0;
	double size = 0;
	ConditionalOperator::Enum conditionalOperator = ConditionalOperator::Enum::none;
	uint32_t constValue = 0;

	Parameter() {}
	Parameter(xml_node<>* parameterNode);
};

class DeviceType
{
public:
	std::string name = "";
	std::string id = "";
	std::vector<Parameter> parameters;

	DeviceType() {}
	DeviceType(xml_node<>* typeNode);
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
	void init(xml_node<>* parameterSetNode);
};

class Device
{
public:
	struct RXModes
	{
		enum Enum { none = 0, config = 1, wakeUp = 2 };
	};

	Device() {}
	Device(std::string xmlFilename);
	uint32_t version() { return _version; }
	uint32_t cyclicTimeout() { return _cyclicTimeout; }
	RXModes::Enum rxModes() { return _rxModes; }
	std::vector<DeviceType>* supportedTypes() { return &_supportedTypes; }
	ParameterSet parameterSet;

	virtual ~Device();
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
