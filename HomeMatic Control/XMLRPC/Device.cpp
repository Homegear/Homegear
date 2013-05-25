#include "Device.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace XMLRPC {

Parameter::Parameter(xml_node<>* parameterNode)
{
	for(xml_attribute<>* attr = parameterNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = std::stod(attributeValue);
		else if(attributeName == "size") size = std::stod(attributeValue);
		else if(attributeName == "cond_op")
		{
			HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
			if(attributeValue == "e") conditionalOperator = ConditionalOperator::Enum::e;
			else if(attributeValue == "g") conditionalOperator = ConditionalOperator::Enum::g;
			else if(attributeValue == "l") conditionalOperator = ConditionalOperator::Enum::l;
			else if(attributeValue == "ge") conditionalOperator = ConditionalOperator::Enum::ge;
			else if(attributeValue == "le") conditionalOperator = ConditionalOperator::Enum::le;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"parameter\": " << attributeName << std::endl;
		}
		else if(attributeName == "const_value") constValue = std::stoll(attributeValue, 0, 16);
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "operations")
		{
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "read") operations = (Operations::Enum)(operations | Operations::Enum::read);
				else if(element == "write") operations = (Operations::Enum)(operations | Operations::Enum::write);
				else if(element == "event") operations = (Operations::Enum)(operations | Operations::Enum::event);
			}
		}
		else if(attributeName == "ui_flags")
		{
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "service") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::service);
				else if(element == "sticky") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::sticky);
			}
		}
	}
}

DeviceType::DeviceType(xml_node<>* typeNode)
{
	for(xml_attribute<>* attr = typeNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "name") name = attributeValue;
		else if(attributeName == "id") id = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"type\": " << attributeName << std::endl;
	}
	for(xml_node<>* parameterNode = typeNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		Parameter parameter(parameterNode);
		parameters.push_back(parameter);
	}
}

ParameterSet::ParameterSet(xml_node<>* parameterSetNode)
{
	init(parameterSetNode);
}

void ParameterSet::init(xml_node<>* parameterSetNode)
{
	for(xml_node<>* parameterNode = parameterSetNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		Parameter parameter(parameterNode);
		parameters.push_back(parameter);
	}
}

Device::Device(std::string xmlFilename)
{
	load(xmlFilename);
}

Device::~Device() {

}

void Device::load(std::string xmlFilename)
{
	xml_document<> doc;
	std::ifstream fileStream("Device types/" + xmlFilename, std::ios::in | std::ios::binary);
	if(fileStream)
	{
		uint32_t length;
		fileStream.seekg(0, std::ios::end);
		length = fileStream.tellg();
		fileStream.seekg(0, std::ios::beg);
		char buffer[length];
		fileStream.read(&buffer[0], length);
		fileStream.close();
		doc.parse<0>(buffer);
		parseXML(&doc);
	}
	else
	{
		throw(errno);
	}
	doc.clear();
}

void Device::parseXML(xml_document<>* doc)
{
	//Device
	xml_node<>* node = doc->first_node("device");
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "version") _version = std::stoll(attributeValue);
		else if(attributeName == "rx_modes")
		{
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "wakeup") _rxModes = (RXModes::Enum)(_rxModes | RXModes::Enum::wakeUp);
				else if(element == "config") _rxModes = (RXModes::Enum)(_rxModes | RXModes::Enum::config);
			}
		}
		else if(attributeName == "cyclic_timeout") _cyclicTimeout = std::stoll(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"device\": " << attributeName << std::endl;
	}

	for(node = node->first_node(); node; node = node->next_sibling())
	{
		std::string nodeName(node->name());
		if(nodeName == "supported_types")
		{
			for(xml_node<>* typeNode = node->first_node("type"); typeNode; typeNode = typeNode->next_sibling())
			{
				DeviceType type(typeNode);
				_supportedTypes.push_back(typeNode);
			}
		}
		else if(nodeName == "paramset")
		{
			for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::string attributeName(attr->name());
				std::string attributeValue(attr->value());
				HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
				if(attributeName == "id") parameterSet.id = attributeValue;
				else if(attributeName == "type")
				{
					if(attributeValue == "MASTER") parameterSet.type = ParameterSet::Type::Enum::master;
					else if(attributeValue == "VALUES") parameterSet.type = ParameterSet::Type::Enum::values;
					else if(attributeValue == "LINK") parameterSet.type = ParameterSet::Type::Enum::values;
				}
				else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset\": " << attributeName << std::endl;
			}
			parameterSet.init(node);
		}
	}
}

}
