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
	rpcString = 0x03,
	rpcArray = 0x100,
	rpcStruct = 0x101
};

class RPCVariable {
public:
	RPCVariableType type;
	std::string name;
	std::string stringValue;
	int32_t integerValue = 0;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> arrayValue;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> structValue;

	RPCVariable() { type = RPCVariableType::rpcInteger; }
	RPCVariable(RPCVariableType variableType) : RPCVariable() { type = variableType; }
	virtual ~RPCVariable() {}
	void print();
private:
	void print(std::shared_ptr<RPCVariable>, std::string indent);
	void printStruct(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
	void printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
};

} /* namespace XMLRPC */
#endif /* RPCVARIABLE_H_ */
