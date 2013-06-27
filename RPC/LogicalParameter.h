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

#include "RPCVariable.h"
#include "rapidxml.hpp"

using namespace rapidxml;

namespace RPC
{

class ParameterOption
{
public:
	std::string id;
	bool isDefault = false;
	int32_t index = -1;

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
	std::string unit;
	bool defaultValueExists = false;
	bool enforce = false;
	Type::Enum type = Type::none;

	LogicalParameter() {}
	virtual ~LogicalParameter() {}
	static std::shared_ptr<LogicalParameter> fromXML(xml_node<>* node);
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid)); }
};

class LogicalParameterInteger : public LogicalParameter
{
public:
	int32_t min = -2147483648;
	int32_t max = 2147483647;
	int32_t defaultValue = 0;
	int32_t enforceValue = 0;
	std::unordered_map<std::string, int32_t> specialValues;

	LogicalParameterInteger();
	LogicalParameterInteger(xml_node<>* node);
	virtual ~LogicalParameterInteger() {}
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

class LogicalParameterFloat : public LogicalParameter
{
public:
	double min = std::numeric_limits<double>::min();
	double max = std::numeric_limits<double>::max();
	double defaultValue = 0;
	double enforceValue = 0;
	std::unordered_map<std::string, double> specialValues;

	LogicalParameterFloat();
	LogicalParameterFloat(xml_node<>* node);
	virtual ~LogicalParameterFloat() {}
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

class LogicalParameterEnum : public LogicalParameter
{
public:
	int32_t min = 0;
	int32_t max = 0;
	int32_t defaultValue = 0;
	int32_t enforceValue = 0;

	LogicalParameterEnum();
	LogicalParameterEnum(xml_node<>* node);
	virtual ~LogicalParameterEnum() {}
	std::vector<ParameterOption> options;
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

class LogicalParameterBoolean : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	bool enforceValue = false;

	LogicalParameterBoolean();
	LogicalParameterBoolean(xml_node<>* node);
	virtual ~LogicalParameterBoolean() {}
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

class LogicalParameterString : public LogicalParameter
{
public:
	std::string min;
	std::string max;
	std::string defaultValue;
	std::string enforceValue;

	LogicalParameterString();
	LogicalParameterString(xml_node<>* node);
	virtual ~LogicalParameterString() {}
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

class LogicalParameterAction : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	bool enforceValue = false;

	LogicalParameterAction();
	LogicalParameterAction(xml_node<>* node);
	virtual ~LogicalParameterAction() {}
	virtual std::shared_ptr<RPCVariable> getEnforceValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(enforceValue)); }
	virtual std::shared_ptr<RPCVariable> getDefaultValue() { return std::shared_ptr<RPCVariable>(new RPCVariable(defaultValue)); }
};

} /* namespace XMLRPC */
#endif /* LOGICALPARAMETER_H_ */
