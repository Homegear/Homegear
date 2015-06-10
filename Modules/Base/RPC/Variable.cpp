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
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{
Variable::Variable(xml_node<>* node) : Variable()
{
	type = VariableType::rpcStruct;
	parseXmlNode(node, structValue);
}

Variable::~Variable()
{
}

void Variable::parseXmlNode(xml_node<>* node, PRPCStruct& xmlStruct)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		xmlStruct->insert(std::pair<std::string, PVariable>(std::string(attr->name()), PVariable(new Variable(std::string(attr->value())))));
	}
	for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
	{
		if(subNode->first_node())
		{
			PVariable subStruct(new Variable(VariableType::rpcStruct));
			parseXmlNode(subNode, subStruct->structValue);
			if(subStruct->structValue->size() == 1 && subStruct->structValue->begin()->first.empty()) xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), subStruct->structValue->begin()->second));
			else xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), subStruct));
		}
		else xmlStruct->insert(std::pair<std::string, PVariable>(std::string(subNode->name()), PVariable(new Variable(std::string(subNode->value())))));
	}
}

std::shared_ptr<Variable> Variable::createError(int32_t faultCode, std::string faultString)
{
	std::shared_ptr<Variable> error(new Variable(VariableType::rpcStruct));
	error->errorStruct = true;
	error->structValue->insert(RPCStructElement("faultCode", std::shared_ptr<Variable>(new Variable(faultCode))));
	error->structValue->insert(RPCStructElement("faultString", std::shared_ptr<Variable>(new Variable(faultString))));
	return error;
}

bool Variable::operator==(const Variable& rhs)
{
	if(type != rhs.type) return false;
	if(type == VariableType::rpcBoolean) return booleanValue == rhs.booleanValue;
	if(type == VariableType::rpcInteger) return integerValue == rhs.integerValue;
	if(type == VariableType::rpcString) return stringValue == rhs.stringValue;
	if(type == VariableType::rpcFloat) return floatValue == rhs.floatValue;
	if(type == VariableType::rpcArray)
	{
		if(arrayValue->size() != rhs.arrayValue->size()) return false;
		for(std::pair<RPCArray::iterator, RPCArray::iterator> i(arrayValue->begin(), rhs.arrayValue->begin()); i.first != arrayValue->end(); ++i.first, ++i.second)
		{
			if(*(i.first) != *(i.second)) return false;
		}
	}
	if(type == VariableType::rpcStruct)
	{
		if(structValue->size() != rhs.structValue->size()) return false;
		for(std::pair<RPCStruct::iterator, RPCStruct::iterator> i(structValue->begin(), rhs.structValue->begin()); i.first != structValue->end(); ++i.first, ++i.second)
		{
			if(i.first->first != i.first->first || *(i.second->second) != *(i.second->second)) return false;
		}
	}
	if(type == VariableType::rpcBase64) return stringValue == rhs.stringValue;
	return false;
}

bool Variable::operator<(const Variable& rhs)
{
	if(type == VariableType::rpcBoolean) return booleanValue < rhs.booleanValue;
	if(type == VariableType::rpcString) return stringValue < rhs.stringValue;
	if(type == VariableType::rpcFloat) return floatValue < rhs.floatValue;
	if(type == VariableType::rpcArray)
	{
		if(arrayValue->size() < rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcStruct)
	{
		if(structValue->size() < rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcBase64) return stringValue < rhs.stringValue;
	return false;
}

bool Variable::operator<=(const Variable& rhs)
{
	if(type == VariableType::rpcBoolean) return booleanValue <= rhs.booleanValue;
	if(type == VariableType::rpcString) return stringValue <= rhs.stringValue;
	if(type == VariableType::rpcFloat) return floatValue <= rhs.floatValue;
	if(type == VariableType::rpcArray)
	{
		if(arrayValue->size() <= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcStruct)
	{
		if(structValue->size() <= rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcBase64) return stringValue <= rhs.stringValue;
	return false;
}

bool Variable::operator>(const Variable& rhs)
{
	if(type == VariableType::rpcBoolean) return booleanValue > rhs.booleanValue;
	if(type == VariableType::rpcString) return stringValue > rhs.stringValue;
	if(type == VariableType::rpcFloat) return floatValue > rhs.floatValue;
	if(type == VariableType::rpcArray)
	{
		if(arrayValue->size() > rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcStruct)
	{
		if(structValue->size() > rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcBase64) return stringValue > rhs.stringValue;
	return false;
}

bool Variable::operator>=(const Variable& rhs)
{
	if(type == VariableType::rpcBoolean) return booleanValue >= rhs.booleanValue;
	if(type == VariableType::rpcString) return stringValue >= rhs.stringValue;
	if(type == VariableType::rpcFloat) return floatValue >= rhs.floatValue;
	if(type == VariableType::rpcArray)
	{
		if(arrayValue->size() >= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcStruct)
	{
		if(structValue->size() >= rhs.structValue->size()) return true; else return false;
	}
	if(type == VariableType::rpcBase64) return stringValue >= rhs.stringValue;
	return false;
}

bool Variable::operator!=(const Variable& rhs)
{
	return !(operator==(rhs));
}

void Variable::print()
{
	if(type == VariableType::rpcVoid)
	{
		std::cout << "(void)" << std::endl;
	}
	else if(type == VariableType::rpcBoolean)
	{
		std::cout << "(Boolean) " << booleanValue << std::endl;
	}
	else if(type == VariableType::rpcInteger)
	{
		std::cout << "(Integer) " << integerValue << std::endl;
	}
	else if(type == VariableType::rpcFloat)
	{
		std::cout << "(Float) " << floatValue << std::endl;
	}
	else if(type == VariableType::rpcString)
	{
		std::cout << "(String) " << stringValue << std::endl;
	}
	else if(type == VariableType::rpcBase64)
	{
		std::cout << "(Base64) " << stringValue << std::endl;
	}
	else if(type == VariableType::rpcArray)
	{
		std::string indent("");
		printArray(arrayValue, indent);
	}
	else if(type == VariableType::rpcStruct)
	{
		std::string indent("");
		printStruct(structValue, indent);
	}
	else
	{
		std::cout << "(unknown)" << std::endl;
	}
}

void Variable::print(std::shared_ptr<Variable> variable, std::string indent)
{
	if(!variable) return;
	if(variable->type == VariableType::rpcVoid)
	{
		std::cout << indent << "(void)" << std::endl;
	}
	else if(variable->type == VariableType::rpcInteger)
	{
		std::cout << indent << "(Integer) " << variable->integerValue << std::endl;
	}
	else if(variable->type == VariableType::rpcFloat)
	{
		std::cout << indent << "(Float) " << variable->floatValue << std::endl;
	}
	else if(variable->type == VariableType::rpcBoolean)
	{
		std::cout << indent << "(Boolean) " << variable->booleanValue << std::endl;
	}
	else if(variable->type == VariableType::rpcString)
	{
		std::cout << indent << "(String) " << variable->stringValue << std::endl;
	}
	else if(type == VariableType::rpcBase64)
	{
		std::cout << indent << "(Base64) " << variable->stringValue << std::endl;
	}
	else if(variable->type == VariableType::rpcArray)
	{
		printArray(variable->arrayValue, indent);
	}
	else if(variable->type == VariableType::rpcStruct)
	{
		printStruct(variable->structValue, indent);
	}
	else
	{
		std::cout << indent << "(Unknown)" << std::endl;
	}
}

void Variable::printArray(PRPCArray array, std::string indent)
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

void Variable::printStruct(PRPCStruct rpcStruct, std::string indent)
{
	std::cout << indent << "(Struct length=" << rpcStruct->size() << ")" << std::endl << indent << "{" << std::endl;
	std::string currentIndent = indent;
	currentIndent.push_back(' ');
	currentIndent.push_back(' ');
	for(std::map<std::string, std::shared_ptr<Variable>>::iterator i = rpcStruct->begin(); i != rpcStruct->end(); ++i)
	{
		std::cout << currentIndent << "[" << i->first << "]" << std::endl << currentIndent << "{" << std::endl;
		print(i->second, currentIndent + "  ");
		std::cout << currentIndent << "}" << std::endl;
	}
	std::cout << indent << "}" << std::endl;
}

std::shared_ptr<Variable> Variable::fromString(std::string value, VariableType type)
{
	if(type == VariableType::rpcBoolean)
	{
		HelperFunctions::toLower(value);
		if(value == "1" || value == "true") return std::shared_ptr<Variable>(new Variable(true));
		else return std::shared_ptr<Variable>(new Variable(false));
	}
	else if(type == VariableType::rpcString) return std::shared_ptr<Variable>(new Variable(value));
	else if(type == VariableType::rpcInteger) return std::shared_ptr<Variable>(new Variable(Math::getNumber(value)));
	else if(type == VariableType::rpcFloat) return std::shared_ptr<Variable>(new Variable(Math::getDouble(value)));
	else if(type == VariableType::rpcBase64)
	{
		std::shared_ptr<Variable> variable(new Variable(VariableType::rpcBase64));
		variable->stringValue = value;
		return variable;
	}
	return createError(-1, "Type not supported.");
}

std::string Variable::toString()
{
	switch(type)
	{
	case VariableType::rpcArray:
		return "array";
	case VariableType::rpcBase64:
		return stringValue;
	case VariableType::rpcBoolean:
		if(booleanValue) return "true"; else return "false";
	case VariableType::rpcFloat:
		return std::to_string(floatValue);
	case VariableType::rpcInteger:
		return std::to_string(integerValue);
	case VariableType::rpcString:
		return stringValue;
	case VariableType::rpcStruct:
		return "struct";
	case VariableType::rpcVoid:
		return "";
	case VariableType::rpcVariant:
		return "valuetype";
	}
	return "";
}

std::string Variable::getTypeString(VariableType type)
{
	switch(type)
	{
	case VariableType::rpcArray:
		return "array";
	case VariableType::rpcBase64:
		return "base64";
	case VariableType::rpcBoolean:
		return "boolean";
	//case VariableType::rpcDate:
	//	return "dateTime.iso8601";
	case VariableType::rpcFloat:
		return "double";
	case VariableType::rpcInteger:
		return "i4";
	case VariableType::rpcString:
		return "string";
	case VariableType::rpcStruct:
		return "struct";
	case VariableType::rpcVoid:
		return "void";
	case VariableType::rpcVariant:
		return "valuetype";
	}
	return "string";
}
}
}
