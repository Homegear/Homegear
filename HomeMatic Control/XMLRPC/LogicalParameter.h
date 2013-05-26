#ifndef LOGICALPARAMETER_H_
#define LOGICALPARAMETER_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

#include "rapidxml.hpp"

using namespace rapidxml;

namespace XMLRPC
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
		enum Enum { none, typeBoolean, typeInteger, typeFloat, typeOption };
	};
	Type::Enum type() { return _type; }

	LogicalParameter() {}
	virtual ~LogicalParameter() {}
	static std::unique_ptr<LogicalParameter> fromXML(xml_node<>* node);
protected:
	Type::Enum _type = Type::Enum::none;
};

class LogicalParameterInteger : public LogicalParameter
{
public:
	int32_t min = 0;
	int32_t max = 0;
	int32_t defaultValue = 0;
	std::string unit;

	LogicalParameterInteger();
	LogicalParameterInteger(xml_node<>* node);
	virtual ~LogicalParameterInteger() {}
};

class LogicalParameterFloat : public LogicalParameter
{
public:
	double min = 0;
	double max = 0;
	double defaultValue = 0;
	std::string unit;
	std::unordered_map<std::string, double> specialValues;

	LogicalParameterFloat();
	LogicalParameterFloat(xml_node<>* node);
	virtual ~LogicalParameterFloat() {}
};

class LogicalParameterOption : public LogicalParameter
{
public:
	LogicalParameterOption();
	LogicalParameterOption(xml_node<>* node);
	virtual ~LogicalParameterOption() {}
	std::vector<ParameterOption> options;
};

class LogicalParameterBoolean : public LogicalParameter
{
public:
	bool defaultValue = false;

	LogicalParameterBoolean();
	LogicalParameterBoolean(xml_node<>* node);
	virtual ~LogicalParameterBoolean() {}
};

} /* namespace XMLRPC */
#endif /* LOGICALPARAMETER_H_ */
