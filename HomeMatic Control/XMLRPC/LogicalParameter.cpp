#include "LogicalParameter.h"
#include "../GD.h"

namespace XMLRPC {

ParameterOption::ParameterOption(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "default" && attributeValue == "true") isDefault = true;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"option\": " << attributeName << std::endl;
	}
}

std::unique_ptr<LogicalParameter> LogicalParameter::fromXML(xml_node<>* node)
{
	LogicalParameter* parameter = nullptr;
	xml_attribute<>* attr = node->first_attribute("type");
	if(attr != nullptr)
	{
		std::string attributeValue(attr->value());
		if(attributeValue == "option") parameter = new LogicalParameterOption(node);
		else if(attributeValue == "integer") parameter = new LogicalParameterInteger(node);
		else if(attributeValue == "boolean") parameter = new LogicalParameterBoolean(node);
	}
	return std::unique_ptr<LogicalParameter>(parameter);
}

LogicalParameterOption::LogicalParameterOption()
{
	_type = Type::Enum::typeOption;
}

LogicalParameterOption::LogicalParameterOption(xml_node<>* node)
{
	_type = Type::Enum::typeOption;
	for(xml_node<>* optionNode = node->first_node("option"); optionNode; optionNode = optionNode->next_sibling())
	{
		ParameterOption option(optionNode);
		options.push_back(option);
	}
}

LogicalParameterInteger::LogicalParameterInteger()
{
	_type = Type::Enum::typeInteger;
}

LogicalParameterInteger::LogicalParameterInteger(xml_node<>* node)
{
	_type = Type::Enum::typeInteger;
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "type") {}
		else if(attributeName == "min") min = std::stoll(attributeValue);
		else if(attributeName == "max") max = std::stoll(attributeValue);
		else if(attributeName == "default") defaultValue = std::stoll(attributeValue);
		else if(attributeName == "unit") unit = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\": " << attributeName << std::endl;
	}
}

LogicalParameterFloat::LogicalParameterFloat()
{
	_type = Type::Enum::typeFloat;
}

LogicalParameterFloat::LogicalParameterFloat(xml_node<>* node)
{
	_type = Type::Enum::typeFloat;
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "type") {}
		else if(attributeName == "min") min = std::stod(attributeValue);
		else if(attributeName == "max") max = std::stod(attributeValue);
		else if(attributeName == "default") defaultValue = std::stod(attributeValue);
		else if(attributeName == "unit") unit = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\": " << attributeName << std::endl;
	}
	for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
	{
		std::string nodeName(node->name());
		if(nodeName == "special_value")
		{
			xml_attribute<>* attr1;
			xml_attribute<>* attr2;
			attr1 = node->first_attribute("id");
			attr2 = node->first_attribute("value");
			if(attr1 != nullptr && attr2 != nullptr) specialValues[attr1->name()] = std::stod(attr1->value());
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node in \"logical\": " << nodeName << std::endl;
	}
}

LogicalParameterBoolean::LogicalParameterBoolean()
{
	_type = Type::Enum::typeBoolean;
}

LogicalParameterBoolean::LogicalParameterBoolean(xml_node<>* node)
{
	_type = Type::Enum::typeBoolean;
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		if(attributeName == "type") {}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\": " << attributeName << std::endl;
	}
}

} /* namespace XMLRPC */
