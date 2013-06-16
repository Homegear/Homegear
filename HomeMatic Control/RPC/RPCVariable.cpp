#include "RPCVariable.h"

namespace RPC
{

std::shared_ptr<RPCVariable> RPCVariable::createError(int32_t faultCode, std::string faultString)
{
	std::shared_ptr<RPCVariable> error(new RPCVariable(RPCVariableType::rpcStruct));
	error->errorStruct = true;
	error->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcInteger)));
	error->structValue->back()->name = "faultCode";
	error->structValue->back()->integerValue = faultCode;
	error->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcString)));
	error->structValue->back()->name = "faultString";
	error->structValue->back()->stringValue = faultString;
	return error;
}

void RPCVariable::print()
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

void RPCVariable::print(std::shared_ptr<RPCVariable> variable, std::string indent)
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
		std::cout << indent << "(unknown)" << std::endl;
	}
}

void RPCVariable::printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> array, std::string indent)
{
	std::cout << indent << "(array length=" << array->size() << ")" << std::endl << indent << "{" << std::endl;
	std::string currentIndent = indent;
	currentIndent.push_back(' ');
	currentIndent.push_back(' ');
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = array->begin(); i != array->end(); ++i)
	{
		print(*i, currentIndent);
	}
	std::cout << indent << "}" << std::endl;
}

void RPCVariable::printStruct(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent)
{
	std::cout << indent << "(struct length=" << rpcStruct->size() << ")" << std::endl << indent << "{" << std::endl;
	std::string currentIndent = indent;
	currentIndent.push_back(' ');
	currentIndent.push_back(' ');
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = rpcStruct->begin(); i != rpcStruct->end(); ++i)
	{
		std::cout << currentIndent << "[" << (*i)->name << "]" << std::endl << currentIndent << "{" << std::endl;
		print(*i, currentIndent + "  ");
		std::cout << currentIndent << "}" << std::endl;
	}
	std::cout << indent << "}" << std::endl;
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
