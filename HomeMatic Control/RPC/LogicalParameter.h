#ifndef LOGICALPARAMETER_H_
#define LOGICALPARAMETER_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <limits>

#include "rapidxml.hpp"

using namespace rapidxml;

namespace RPC
{

class ParameterOption
{
public:
	std::string id;
	bool isDefault = false;

	ParameterOption() {}
	ParameterOption(xml_node<>* node);
	virtual ~ParameterOption() {}
};

class LogicalParameter
{
public:
	struct Type
	{
		enum Enum { none = 0x00, typeInteger = 0x01, typeBoolean = 0x02, typeString = 0x03, typeFloat = 0x04, typeEnum = 0x20, typeAction = 0x30 };
	};
	Type::Enum type;

	LogicalParameter() {}
	virtual ~LogicalParameter() {}
	static std::shared_ptr<LogicalParameter> fromXML(xml_node<>* node);
};

class LogicalParameterInteger : public LogicalParameter
{
public:
	int32_t min = -2147483648;
	int32_t max = 2147483647;
	int32_t defaultValue = 0;
	std::string unit;
	std::unordered_map<std::string, int32_t> specialValues;

	LogicalParameterInteger();
	LogicalParameterInteger(xml_node<>* node);
	virtual ~LogicalParameterInteger() {}
};

class LogicalParameterFloat : public LogicalParameter
{
public:
	double min = std::numeric_limits<double>::min();
	double max = std::numeric_limits<double>::max();
	double defaultValue = 0;
	std::string unit;
	std::unordered_map<std::string, double> specialValues;

	LogicalParameterFloat();
	LogicalParameterFloat(xml_node<>* node);
	virtual ~LogicalParameterFloat() {}
};

class LogicalParameterEnum : public LogicalParameter
{
public:
	int32_t min = 0;
	int32_t max = 0;
	int32_t defaultValue = 0;
	std::string unit;

	LogicalParameterEnum();
	LogicalParameterEnum(xml_node<>* node);
	virtual ~LogicalParameterEnum() {}
	std::vector<ParameterOption> options;
};

class LogicalParameterBoolean : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	std::string unit;

	LogicalParameterBoolean();
	LogicalParameterBoolean(xml_node<>* node);
	virtual ~LogicalParameterBoolean() {}
};

class LogicalParameterString : public LogicalParameter
{
public:
	std::string min;
	std::string max;
	std::string defaultValue;
	std::string unit;

	LogicalParameterString();
	LogicalParameterString(xml_node<>* node);
	virtual ~LogicalParameterString() {}
};

class LogicalParameterAction : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	std::string unit;

	LogicalParameterAction();
	LogicalParameterAction(xml_node<>* node);
	virtual ~LogicalParameterAction() {}
};

} /* namespace XMLRPC */
#endif /* LOGICALPARAMETER_H_ */
