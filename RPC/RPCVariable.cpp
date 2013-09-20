/* Copyright 2013 Sathya Laufer
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

#include "RPCVariable.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC
{
RPCVariable::RPCVariable(RPCVariableType variableType, std::vector<uint8_t> data) : RPCVariable()
{
	try
	{
		type = variableType;
		if(data.empty()) return;
		if(variableType == RPCVariableType::rpcBoolean)
		{
			booleanValue = false;
			for(std::vector<uint8_t>::iterator i = data.begin(); i != data.end(); ++i)
			{
				if(*i != 0)
				{
					booleanValue = true;
					break;
				}
			}
		}
		else if(variableType == RPCVariableType::rpcInteger)
		{
			HelperFunctions::memcpyBigEndian(integerValue, data);
		}
		else HelperFunctions::printError("Error: Could not create RPCVariable. Type cannot be converted automatically.");
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPCVariable> RPCVariable::createError(int32_t faultCode, std::string faultString)
{
	std::shared_ptr<RPCVariable> error(new RPCVariable(RPCVariableType::rpcStruct));
	error->errorStruct = true;
	error->structValue->insert(RPCStructElement("faultCode", std::shared_ptr<RPCVariable>(new RPCVariable(faultCode))));
	error->structValue->insert(RPCStructElement("faultString", std::shared_ptr<RPCVariable>(new RPCVariable(faultString))));
	return error;
}

bool RPCVariable::operator==(const RPCVariable& rhs)
{
	if(type != rhs.type) return false;
	if(type == RPCVariableType::rpcBoolean) return booleanValue == rhs.booleanValue;
	if(type == RPCVariableType::rpcString) return stringValue == rhs.stringValue;
	if(type == RPCVariableType::rpcFloat) return floatValue == rhs.floatValue;
	if(type == RPCVariableType::rpcArray)
	{
		if(arrayValue->size() != rhs.arrayValue->size()) return false;
		for(std::pair<RPCArray::iterator, RPCArray::iterator> i(arrayValue->begin(), rhs.arrayValue->begin()); i.first != arrayValue->end(); ++i.first, ++i.second)
		{
			if(*(i.first) != *(i.second)) return false;
		}
	}
	if(type == RPCVariableType::rpcStruct)
	{
		if(structValue->size() != rhs.structValue->size()) return false;
		for(std::pair<RPCStruct::iterator, RPCStruct::iterator> i(structValue->begin(), rhs.structValue->begin()); i.first != structValue->end(); ++i.first, ++i.second)
		{
			if(i.first->first != i.first->first || *(i.second->second) != *(i.second->second)) return false;
		}
	}
	if(type == RPCVariableType::rpcBase64) return stringValue == rhs.stringValue;
	return false;
}

bool RPCVariable::operator<(const RPCVariable& rhs)
{
	if(type == RPCVariableType::rpcBoolean) return booleanValue < rhs.booleanValue;
	if(type == RPCVariableType::rpcString) return stringValue < rhs.stringValue;
	if(type == RPCVariableType::rpcFloat) return floatValue < rhs.floatValue;
	if(type == RPCVariableType::rpcArray)
	{
		if(arrayValue->size() < rhs.arrayValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcStruct)
	{
		if(structValue->size() < rhs.structValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcBase64) return stringValue < rhs.stringValue;
	return false;
}

bool RPCVariable::operator<=(const RPCVariable& rhs)
{
	if(type == RPCVariableType::rpcBoolean) return booleanValue <= rhs.booleanValue;
	if(type == RPCVariableType::rpcString) return stringValue <= rhs.stringValue;
	if(type == RPCVariableType::rpcFloat) return floatValue <= rhs.floatValue;
	if(type == RPCVariableType::rpcArray)
	{
		if(arrayValue->size() <= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcStruct)
	{
		if(structValue->size() <= rhs.structValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcBase64) return stringValue <= rhs.stringValue;
	return false;
}

bool RPCVariable::operator>(const RPCVariable& rhs)
{
	if(type == RPCVariableType::rpcBoolean) return booleanValue > rhs.booleanValue;
	if(type == RPCVariableType::rpcString) return stringValue > rhs.stringValue;
	if(type == RPCVariableType::rpcFloat) return floatValue > rhs.floatValue;
	if(type == RPCVariableType::rpcArray)
	{
		if(arrayValue->size() > rhs.arrayValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcStruct)
	{
		if(structValue->size() > rhs.structValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcBase64) return stringValue > rhs.stringValue;
	return false;
}

bool RPCVariable::operator>=(const RPCVariable& rhs)
{
	if(type == RPCVariableType::rpcBoolean) return booleanValue >= rhs.booleanValue;
	if(type == RPCVariableType::rpcString) return stringValue >= rhs.stringValue;
	if(type == RPCVariableType::rpcFloat) return floatValue >= rhs.floatValue;
	if(type == RPCVariableType::rpcArray)
	{
		if(arrayValue->size() >= rhs.arrayValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcStruct)
	{
		if(structValue->size() >= rhs.structValue->size()) return true; else return false;
	}
	if(type == RPCVariableType::rpcBase64) return stringValue >= rhs.stringValue;
	return false;
}

bool RPCVariable::operator!=(const RPCVariable& rhs)
{
	return !(operator==(rhs));
}

void RPCVariable::print()
{
	try
	{
		if(type == RPCVariableType::rpcVoid)
		{
			std::cout << "(void)" << std::endl;
		}
		else if(type == RPCVariableType::rpcBoolean)
		{
			std::cout << "(Boolean) " << booleanValue << std::endl;
		}
		else if(type == RPCVariableType::rpcInteger)
		{
			std::cout << "(Integer) " << integerValue << std::endl;
		}
		else if(type == RPCVariableType::rpcFloat)
		{
			std::cout << "(Float) " << floatValue << std::endl;
		}
		else if(type == RPCVariableType::rpcString)
		{
			std::cout << "(String) " << stringValue << std::endl;
		}
		else if(type == RPCVariableType::rpcBase64)
		{
			std::cout << "(Base64) " << stringValue << std::endl;
		}
		else if(type == RPCVariableType::rpcArray)
		{
			std::string indent("");
			printArray(arrayValue, indent);
		}
		else if(type == RPCVariableType::rpcStruct)
		{
			std::string indent("");
			printStruct(structValue, indent);
		}
		else
		{
			std::cout << "(unknown)" << std::endl;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCVariable::print(std::shared_ptr<RPCVariable> variable, std::string indent)
{
	try
	{
		if(variable->type == RPCVariableType::rpcVoid)
		{
			std::cout << indent << "(void)" << std::endl;
		}
		else if(variable->type == RPCVariableType::rpcInteger)
		{
			std::cout << indent << "(Integer) " << variable->integerValue << std::endl;
		}
		else if(variable->type == RPCVariableType::rpcFloat)
		{
			std::cout << indent << "(Float) " << variable->floatValue << std::endl;
		}
		else if(variable->type == RPCVariableType::rpcBoolean)
		{
			std::cout << indent << "(Boolean) " << variable->booleanValue << std::endl;
		}
		else if(variable->type == RPCVariableType::rpcString)
		{
			std::cout << indent << "(String) " << variable->stringValue << std::endl;
		}
		else if(type == RPCVariableType::rpcBase64)
		{
			std::cout << indent << "(Base64) " << variable->stringValue << std::endl;
		}
		else if(variable->type == RPCVariableType::rpcArray)
		{
			printArray(variable->arrayValue, indent);
		}
		else if(variable->type == RPCVariableType::rpcStruct)
		{
			printStruct(variable->structValue, indent);
		}
		else
		{
			std::cout << indent << "(Unknown)" << std::endl;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCVariable::printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> array, std::string indent)
{
	try
	{
		std::cout << indent << "(Array length=" << array->size() << ")" << std::endl << indent << "{" << std::endl;
		std::string currentIndent = indent;
		currentIndent.push_back(' ');
		currentIndent.push_back(' ');
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = array->begin(); i != array->end(); ++i)
		{
			print(*i, currentIndent);
		}
		std::cout << indent << "}" << std::endl;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCVariable::printStruct(std::shared_ptr<std::map<std::string, std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent)
{
	try
	{
		std::cout << indent << "(Struct length=" << rpcStruct->size() << ")" << std::endl << indent << "{" << std::endl;
		std::string currentIndent = indent;
		currentIndent.push_back(' ');
		currentIndent.push_back(' ');
		for(std::map<std::string, std::shared_ptr<RPCVariable>>::iterator i = rpcStruct->begin(); i != rpcStruct->end(); ++i)
		{
			std::cout << currentIndent << "[" << i->first << "]" << std::endl << currentIndent << "{" << std::endl;
			print(i->second, currentIndent + "  ");
			std::cout << currentIndent << "}" << std::endl;
		}
		std::cout << indent << "}" << std::endl;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPCVariable> RPCVariable::fromString(std::string value, RPCVariableType type)
{
	try
	{
		if(type == RPCVariableType::rpcBoolean)
		{
			HelperFunctions::toLower(value);
			if(value == "1" || value == "true") return std::shared_ptr<RPCVariable>(new RPCVariable(true));
			else return std::shared_ptr<RPCVariable>(new RPCVariable(false));
		}
		else if(type == RPCVariableType::rpcString) return std::shared_ptr<RPCVariable>(new RPCVariable(value));
		else if(type == RPCVariableType::rpcInteger) return std::shared_ptr<RPCVariable>(new RPCVariable(HelperFunctions::getNumber(value)));
		else if(type == RPCVariableType::rpcFloat) return std::shared_ptr<RPCVariable>(new RPCVariable(HelperFunctions::getDouble(value)));
		else if(type == RPCVariableType::rpcBase64)
		{
			std::shared_ptr<RPCVariable> variable(new RPCVariable(RPCVariableType::rpcBase64));
			variable->stringValue = value;
			return variable;
		}
		else return createError(-1, "Type not supported.");
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCVariable>();
}

std::string RPCVariable::getTypeString(RPCVariableType type)
{
	switch(type)
	{
	case RPCVariableType::rpcArray:
		return "array";
	case RPCVariableType::rpcBase64:
		return "base64";
	case RPCVariableType::rpcBoolean:
		return "boolean";
	case RPCVariableType::rpcDate:
		return "dateTime.iso8601";
	case RPCVariableType::rpcFloat:
		return "double";
	case RPCVariableType::rpcInteger:
		return "i4";
	case RPCVariableType::rpcString:
		return "string";
	case RPCVariableType::rpcStruct:
		return "struct";
	case RPCVariableType::rpcVoid:
		return "void";
	case RPCVariableType::rpcVariant:
		return "valuetype";
	}
	return "string";
}
} /* namespace XMLRPC */
