#ifndef RPCVARIABLE_H_
#define RPCVARIABLE_H_

#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace RPC
{

enum class RPCVariableType
{
	rpcVoid = 0x00,
	rpcInteger = 0x01,
	rpcBoolean = 0x02,
	rpcString = 0x03,
	rpcArray = 0x100,
	rpcStruct = 0x101
};

class RPCVariable {
public:
	bool errorStruct = false;
	RPCVariableType type;
	std::string name;
	std::string stringValue;
	int32_t integerValue = 0;
	bool booleanValue = false;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> arrayValue;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> structValue;

	RPCVariable() { type = RPCVariableType::rpcVoid; arrayValue = std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>()); structValue = std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>()); }
	RPCVariable(RPCVariableType variableType) : RPCVariable() { type = variableType; }
	RPCVariable(int32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = integer; }
	RPCVariable(std::string structElementName, int32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; name = structElementName; integerValue = integer; }
	RPCVariable(std::string string) : RPCVariable() { type = RPCVariableType::rpcString; stringValue = string; }
	RPCVariable(std::string structElementName, std::string string) : RPCVariable() { type = RPCVariableType::rpcString; name = structElementName; stringValue = string; }
	virtual ~RPCVariable() {}
	static std::shared_ptr<RPCVariable> createError(int32_t faultCode, std::string faultString);
	void print();
private:
	void print(std::shared_ptr<RPCVariable>, std::string indent);
	void printStruct(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
	void printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
};

} /* namespace XMLRPC */
#endif /* RPCVARIABLE_H_ */
