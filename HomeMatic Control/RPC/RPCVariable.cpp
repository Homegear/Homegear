#include "RPCVariable.h"

namespace RPC
{

void RPCVariable::print()
{
	if(type == RPCVariableType::rpcVoid)
	{
		std::cout << "(void)" << std::endl;
	}
	else if(type == RPCVariableType::rpcInteger)
	{
		std::cout << "(Integer) " << integerValue;
	}
	else if(type == RPCVariableType::rpcString)
	{
		std::cout << "(String) " << stringValue;
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
	else if(variable->type == RPCVariableType::rpcString)
	{
		std::cout << indent << "(String) " << variable->stringValue << std::endl;
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

} /* namespace XMLRPC */
