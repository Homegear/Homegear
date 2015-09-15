/* Copyright 2013-2015 Sathya Laufer
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

#include "Variable.h"
#include "BaseLib.h"

namespace BaseLib
{
Variable::Variable(xml_node<>* node) : Variable()
{
	type = VariableType::tStruct;
	parseXmlNode(node, structValue);
}

Variable::~Variable()
{
}

void Variable::parseXmlNode(xml_node<>* node, PStruct& xmlStruct)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		xmlStruct->insert(std::pair<std::string, PVariable>(std::string(attr->name()), PVariable(new Variable(std::string(attr->value())))));
	}
	for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
	{
		if(subNode->first_node())
		{
			PVariable subStruct(new Variable(VariableType::tStruct));
			parseXmlNode(subNode, subStruct->structValue);
			if(subStruct->structValue->size() == 1 && subStruct->structValue->begin()->first.empty()) xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), subStruct->structValue->begin()->second));
			else xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), subStruct));
		}
		else xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), PVariable(new Variable(std::string(subNode->value())))));
	}
}

std::shared_ptr<Variable> Variable::createError(int32_t faultCode, std::string faultString)
{
	std::shared_ptr<Variable> error(new Variable(VariableType::tStruct));
	error->errorStruct = true;
	error->structValue->insert(StructElement("faultCode", std::shared_ptr<Variable>(new Variable(faultCode))));
	error->structValue->insert(StructElement("faultString", std::shared_ptr<Variable>(new Variable(faultString))));
	return error;
}

Variable& Variable::operator=(const Variable& rhs)
{
	if(&rhs == this) return *this;
	errorStruct = rhs.errorStruct;
	type = rhs.type;
	stringValue = rhs.stringValue;
	integerValue = rhs.integerValue;
	floatValue = rhs.floatValue;
	booleanValue = rhs.booleanValue;
	binaryValue = rhs.binaryValue;
	for(Array::const_iterator i = rhs.arrayValue->begin(); i != rhs.arrayValue->end(); ++i)
	{
		PVariable lhs(new Variable());
		*lhs = *(*i);
		arrayValue->push_back(lhs);
	}
	for(Struct::const_iterator i = rhs.structValue->begin(); i != rhs.structValue->end(); ++i)
	{
		PVariable lhs(new Variable());
		*lhs = *(i->second);
		structValue->insert(std::pair<std::string, PVariable>(i->first, lhs));
	}
	return *this;
}

bool Variable::operator==(const Variable& rhs)
{
	if(type != rhs.type) return false;
	if(type == VariableType::tBoolean) return booleanValue == rhs.booleanValue;
	if(type == VariableType::tInteger) return integerValue == rhs.integerValue;
	if(type == VariableType::tString) return stringValue == rhs.stringValue;
	if(type == VariableType::tFloat) return floatValue == rhs.floatValue;
	if(type == VariableType::tArray)
	{
		if(arrayValue->size() != rhs.arrayValue->size()) return false;
		for(std::pair<Array::iterator, Array::iterator> i(arrayValue->begin(), rhs.arrayValue->begin()); i.first != arrayValue->end(); ++i.first, ++i.second)
		{
			if(*(i.first) != *(i.second)) return false;
		}
	}
	if(type == VariableType::tStruct)
	{
		if(structValue->size() != rhs.structValue->size()) return false;
		for(std::pair<Struct::iterator, Struct::iterator> i(structValue->begin(), rhs.structValue->begin()); i.first != structValue->end(); ++i.first, ++i.second)
		{
			if(i.first->first != i.first->first || *(i.second->second) != *(i.second->second)) return false;
		}
	}
	if(type == VariableType::tBase64) return stringValue == rhs.stringValue;
	if(type == VariableType::tBinary)
	{
		if(binaryValue.size() != rhs.binaryValue.size()) return false;
		if(binaryValue.empty()) return true;
		uint8_t* i = &binaryValue[0];
		uint8_t* j = (uint8_t*)&rhs.binaryValue[0];
		for(; i < &binaryValue[0] + binaryValue.size(); i++, j++)
		{
			if(*i != *j) return false;
		}
		return true;
	}
	return false;
}

bool Variable::operator<(const Variable& rhs)
{
	if(type == VariableType::tBoolean) return booleanValue < rhs.booleanValue;
	if(type == VariableType::tString) return stringValue < rhs.stringValue;
	if(type == VariableType::tFloat) return floatValue < rhs.floatValue;
	if(type == VariableType::tArray)
	{
		if(arrayValue->size() < rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::tStruct)
	{
		if(structValue->size() < rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::tBase64) return stringValue < rhs.stringValue;
	return false;
}

bool Variable::operator<=(const Variable& rhs)
{
	if(type == VariableType::tBoolean) return booleanValue <= rhs.booleanValue;
	if(type == VariableType::tString) return stringValue <= rhs.stringValue;
	if(type == VariableType::tFloat) return floatValue <= rhs.floatValue;
	if(type == VariableType::tArray)
	{
		if(arrayValue->size() <= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::tStruct)
	{
		if(structValue->size() <= rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::tBase64) return stringValue <= rhs.stringValue;
	return false;
}

bool Variable::operator>(const Variable& rhs)
{
	if(type == VariableType::tBoolean) return booleanValue > rhs.booleanValue;
	if(type == VariableType::tString) return stringValue > rhs.stringValue;
	if(type == VariableType::tFloat) return floatValue > rhs.floatValue;
	if(type == VariableType::tArray)
	{
		if(arrayValue->size() > rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::tStruct)
	{
		if(structValue->size() > rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::tBase64) return stringValue > rhs.stringValue;
	return false;
}

bool Variable::operator>=(const Variable& rhs)
{
	if(type == VariableType::tBoolean) return booleanValue >= rhs.booleanValue;
	if(type == VariableType::tString) return stringValue >= rhs.stringValue;
	if(type == VariableType::tFloat) return floatValue >= rhs.floatValue;
	if(type == VariableType::tArray)
	{
		if(arrayValue->size() >= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::tStruct)
	{
		if(structValue->size() >= rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::tBase64) return stringValue >= rhs.stringValue;
	return false;
}

bool Variable::operator!=(const Variable& rhs)
{
	return !(operator==(rhs));
}

void Variable::print()
{
	if(type == VariableType::tVoid)
	{
		std::cout << "(void)" << std::endl;
	}
	else if(type == VariableType::tBoolean)
	{
		std::cout << "(Boolean) " << booleanValue << std::endl;
	}
	else if(type == VariableType::tInteger)
	{
		std::cout << "(Integer) " << integerValue << std::endl;
	}
	else if(type == VariableType::tFloat)
	{
		std::cout << "(Float) " << floatValue << std::endl;
	}
	else if(type == VariableType::tString)
	{
		std::cout << "(String) " << stringValue << std::endl;
	}
	else if(type == VariableType::tBase64)
	{
		std::cout << "(Base64) " << stringValue << std::endl;
	}
	else if(type == VariableType::tArray)
	{
		std::string indent("");
		printArray(arrayValue, indent);
	}
	else if(type == VariableType::tStruct)
	{
		std::string indent("");
		printStruct(structValue, indent);
	}
	else if(type == VariableType::tBinary)
	{
		std::cout << "(Binary) " << HelperFunctions::getHexString(binaryValue) << std::endl;
	}
	else
	{
		std::cout << "(unknown)" << std::endl;
	}
}

void Variable::print(std::shared_ptr<Variable> variable, std::string indent)
{
	if(!variable) return;
	if(variable->type == VariableType::tVoid)
	{
		std::cout << indent << "(void)" << std::endl;
	}
	else if(variable->type == VariableType::tInteger)
	{
		std::cout << indent << "(Integer) " << variable->integerValue << std::endl;
	}
	else if(variable->type == VariableType::tFloat)
	{
		std::cout << indent << "(Float) " << variable->floatValue << std::endl;
	}
	else if(variable->type == VariableType::tBoolean)
	{
		std::cout << indent << "(Boolean) " << variable->booleanValue << std::endl;
	}
	else if(variable->type == VariableType::tString)
	{
		std::cout << indent << "(String) " << variable->stringValue << std::endl;
	}
	else if(type == VariableType::tBase64)
	{
		std::cout << indent << "(Base64) " << variable->stringValue << std::endl;
	}
	else if(variable->type == VariableType::tArray)
	{
		printArray(variable->arrayValue, indent);
	}
	else if(variable->type == VariableType::tStruct)
	{
		printStruct(variable->structValue, indent);
	}
	else if(variable->type == VariableType::tBinary)
	{
		std::cout << indent << "(Binary) " << HelperFunctions::getHexString(binaryValue) << std::endl;
	}
	else
	{
		std::cout << indent << "(Unknown)" << std::endl;
	}
}

void Variable::printArray(PArray array, std::string indent)
{
	std::cout << indent << "(Array length=" << array->size() << ")" << std::endl << indent << "{" << std::endl;
	std::string currentIndent = indent;
	currentIndent.push_back(' ');
	currentIndent.push_back(' ');
	for(std::vector<std::shared_ptr<Variable>>::iterator i = array->begin(); i != array->end(); ++i)
	{
		print(*i, currentIndent);
	}
	std::cout << indent << "}" << std::endl;
}

void Variable::printStruct(PStruct tStruct, std::string indent)
{
	std::cout << indent << "(Struct length=" << tStruct->size() << ")" << std::endl << indent << "{" << std::endl;
	std::string currentIndent = indent;
	currentIndent.push_back(' ');
	currentIndent.push_back(' ');
	for(std::map<std::string, std::shared_ptr<Variable>>::iterator i = tStruct->begin(); i != tStruct->end(); ++i)
	{
		std::cout << currentIndent << "[" << i->first << "]" << std::endl << currentIndent << "{" << std::endl;
		print(i->second, currentIndent + "  ");
		std::cout << currentIndent << "}" << std::endl;
	}
	std::cout << indent << "}" << std::endl;
}

PVariable Variable::fromString(std::string& value, DeviceDescription::ILogical::Type::Enum type)
{
	VariableType variableType = VariableType::tVoid;
	switch(type)
	{
	case DeviceDescription::ILogical::Type::Enum::none:
		break;
	case DeviceDescription::ILogical::Type::Enum::tAction:
		variableType = VariableType::tBoolean;
		break;
	case DeviceDescription::ILogical::Type::Enum::tBoolean:
		variableType = VariableType::tBoolean;
		break;
	case DeviceDescription::ILogical::Type::Enum::tEnum:
		variableType = VariableType::tInteger;
		break;
	case DeviceDescription::ILogical::Type::Enum::tFloat:
		variableType = VariableType::tFloat;
		break;
	case DeviceDescription::ILogical::Type::Enum::tInteger:
		variableType = VariableType::tInteger;
		break;
	case DeviceDescription::ILogical::Type::Enum::tString:
		variableType = VariableType::tString;
		break;
	case DeviceDescription::ILogical::Type::Enum::tArray:
		variableType = VariableType::tArray;
		break;
	case DeviceDescription::ILogical::Type::Enum::tStruct:
		variableType = VariableType::tStruct;
		break;
	}
	return fromString(value, variableType);
}

PVariable Variable::fromString(std::string& value, DeviceDescription::IPhysical::Type::Enum type)
{
	VariableType variableType = VariableType::tVoid;
	switch(type)
	{
	case DeviceDescription::IPhysical::Type::Enum::none:
		break;
	case DeviceDescription::IPhysical::Type::Enum::tBoolean:
		variableType = VariableType::tBoolean;
		break;
	case DeviceDescription::IPhysical::Type::Enum::tInteger:
		variableType = VariableType::tInteger;
		break;
	case DeviceDescription::IPhysical::Type::Enum::tString:
		variableType = VariableType::tString;
		break;
	}
	return fromString(value, variableType);
}

PVariable Variable::fromString(std::string& value, VariableType type)
{
	if(type == VariableType::tBoolean)
	{
		HelperFunctions::toLower(value);
		if(value == "1" || value == "true") return std::shared_ptr<Variable>(new Variable(true));
		else return std::shared_ptr<Variable>(new Variable(false));
	}
	else if(type == VariableType::tString) return std::shared_ptr<Variable>(new Variable(value));
	else if(type == VariableType::tInteger) return std::shared_ptr<Variable>(new Variable(Math::getNumber(value)));
	else if(type == VariableType::tFloat) return std::shared_ptr<Variable>(new Variable(Math::getDouble(value)));
	else if(type == VariableType::tBase64)
	{
		std::shared_ptr<Variable> variable(new Variable(VariableType::tBase64));
		variable->stringValue = value;
		return variable;
	}
	return createError(-1, "Type not supported.");
}

std::string Variable::toString()
{
	switch(type)
	{
	case VariableType::tArray:
		return "array";
	case VariableType::tBase64:
		return stringValue;
	case VariableType::tBoolean:
		if(booleanValue) return "true"; else return "false";
	case VariableType::tFloat:
		return std::to_string(floatValue);
	case VariableType::tInteger:
		return std::to_string(integerValue);
	case VariableType::tString:
		return stringValue;
	case VariableType::tStruct:
		return "struct";
	case VariableType::tBinary:
		return HelperFunctions::getHexString(binaryValue);
	case VariableType::tVoid:
		return "";
	case VariableType::tVariant:
		return "valuetype";
	}
	return "";
}

std::string Variable::getTypeString(VariableType type)
{
	switch(type)
	{
	case VariableType::tArray:
		return "array";
	case VariableType::tBase64:
		return "base64";
	case VariableType::tBoolean:
		return "boolean";
	//case VariableType::rpcDate:
	//	return "dateTime.iso8601";
	case VariableType::tFloat:
		return "double";
	case VariableType::tInteger:
		return "i4";
	case VariableType::tString:
		return "string";
	case VariableType::tStruct:
		return "struct";
	case VariableType::tBinary:
		return "binary";
	case VariableType::tVoid:
		return "void";
	case VariableType::tVariant:
		return "valuetype";
	}
	return "string";
}
}
