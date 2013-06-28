#include "LogicalParameter.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC {

ParameterOption::ParameterOption(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "default" && attributeValue == "true") isDefault = true;
		else if(attributeName == "index") index = HelperFunctions::getNumber(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"option\": " << attributeName << std::endl;
	}
}

std::shared_ptr<LogicalParameter> LogicalParameter::fromXML(xml_node<>* node)
{
	std::shared_ptr<LogicalParameter> parameter;
	try
	{
		xml_attribute<>* attr = node->first_attribute("type");
		if(attr != nullptr)
		{
			std::string attributeValue(attr->value());
			if(attributeValue == "option") parameter.reset(new LogicalParameterEnum(node));
			else if(attributeValue == "integer") parameter.reset(new LogicalParameterInteger(node));
			else if(attributeValue == "float") parameter.reset(new LogicalParameterFloat(node));
			else if(attributeValue == "boolean") parameter.reset(new LogicalParameterBoolean(node));
			else if(attributeValue == "string") parameter.reset(new LogicalParameterString(node));
			else if(attributeValue == "action") parameter.reset(new LogicalParameterAction(node));
			else  parameter.reset(new LogicalParameterInteger(node));
		}
		else parameter.reset(new LogicalParameterInteger(node));
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return parameter;
}

LogicalParameterEnum::LogicalParameterEnum()
{
	type = Type::Enum::typeEnum;
}

LogicalParameterEnum::LogicalParameterEnum(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeEnum;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "unit") unit = attributeValue;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type enum: " << attributeName << std::endl;
		}
		int32_t index = 0;
		for(xml_node<>* optionNode = node->first_node("option"); optionNode; optionNode = optionNode->next_sibling())
		{
			ParameterOption option(optionNode);
			if(option.index > -1)
			{
				while((unsigned)option.index > options.size()) options.push_back(ParameterOption());
				index = option.index;
			}
			options.push_back(option);
			if(options.back().isDefault)
			{
				defaultValueExists = true;
				defaultValue = index;
			}
			index++;
		}
		max = index - 1;
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

LogicalParameterInteger::LogicalParameterInteger()
{
	type = Type::Enum::typeInteger;
}

LogicalParameterInteger::LogicalParameterInteger(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeInteger;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "max") max = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = HelperFunctions::getNumber(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type integer: " << attributeName << std::endl;
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			std::string nodeName(logicalNode->name());
			if(nodeName == "special_value")
			{
				xml_attribute<>* attr1;
				xml_attribute<>* attr2;
				attr1 = logicalNode->first_attribute("id");
				attr2 = logicalNode->first_attribute("value");
				if(attr1 != nullptr && attr2 != nullptr) continue;
				std::string valueString(attr2->value());
				specialValues[attr1->value()] = HelperFunctions::getNumber(valueString);
			}
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node in \"logical\" with type integer: " << nodeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

LogicalParameterFloat::LogicalParameterFloat()
{
	type = Type::Enum::typeFloat;
}

LogicalParameterFloat::LogicalParameterFloat(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeFloat;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = HelperFunctions::getDouble(attributeValue);
			else if(attributeName == "max") max = HelperFunctions::getDouble(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = HelperFunctions::getDouble(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type float: " << attributeName << std::endl;
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			std::string nodeName(logicalNode->name());
			if(nodeName == "special_value")
			{
				xml_attribute<>* attr1;
				xml_attribute<>* attr2;
				attr1 = logicalNode->first_attribute("id");
				attr2 = logicalNode->first_attribute("value");
				if(attr1 != nullptr && attr2 != nullptr) continue;
				std::string valueString(attr2->value());
				specialValues[attr1->value()] = HelperFunctions::getDouble(valueString);
			}
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node in \"logical\" with type float: " << nodeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

LogicalParameterBoolean::LogicalParameterBoolean()
{
	type = Type::Enum::typeBoolean;
}

LogicalParameterBoolean::LogicalParameterBoolean(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeBoolean;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "default")
			{
				if(attributeValue == "true") defaultValue = true;
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(attributeName != "type" && GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type boolean: " << attributeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

LogicalParameterString::LogicalParameterString()
{
	type = Type::Enum::typeString;
}

LogicalParameterString::LogicalParameterString(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeString;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "default")
			{
				defaultValue = attributeValue;
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(attributeName == "use_default_on_failure") {} //ignore, not necessary - all values are initialized
			else if(attributeName != "type" && GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type string: " << attributeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

LogicalParameterAction::LogicalParameterAction()
{
	type = Type::Enum::typeAction;
}

LogicalParameterAction::LogicalParameterAction(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeAction;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			if(attributeName != "type" && GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"logical\" with type boolean: " << attributeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

} /* namespace XMLRPC */
