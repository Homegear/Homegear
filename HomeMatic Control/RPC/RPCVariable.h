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
	rpcFloat = 0x04,
	rpcArray = 0x100,
	rpcStruct = 0x101,
	rpcDate = 0x10,
	rpcBase64 = 0x11,
	rpcVariant = 0x1111
};

class RPCVariable {
public:
	bool errorStruct = false;
	RPCVariableType type;
	std::string name;
	std::string stringValue;
	int32_t integerValue = 0;
	double floatValue = 0;
	bool booleanValue = false;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> arrayValue;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> structValue;

	RPCVariable() { type = RPCVariableType::rpcVoid; arrayValue = std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>()); structValue = std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>()); }
	RPCVariable(RPCVariableType variableType) : RPCVariable() { type = variableType; }
	RPCVariable(int32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = integer; }
	RPCVariable(std::string structElementName, int32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; name = structElementName; integerValue = integer; }
	RPCVariable(uint32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = (int32_t)integer; }
	RPCVariable(std::string structElementName, uint32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; name = structElementName; integerValue = (int32_t)integer; }
	RPCVariable(bool boolean) : RPCVariable() { type = RPCVariableType::rpcBoolean; booleanValue = boolean; }
	RPCVariable(std::string structElementName, bool boolean) : RPCVariable() { type = RPCVariableType::rpcBoolean; name = structElementName; booleanValue = boolean; }
	RPCVariable(std::string string) : RPCVariable() { type = RPCVariableType::rpcString; stringValue = string; }
	RPCVariable(std::string structElementName, std::string string) : RPCVariable() { type = RPCVariableType::rpcString; name = structElementName; stringValue = string; }
	RPCVariable(double floatVal) : RPCVariable() { type = RPCVariableType::rpcFloat; floatValue = floatVal; }
	RPCVariable(std::string structElementName, double floatVal) : RPCVariable() { type = RPCVariableType::rpcFloat; name = structElementName; floatValue = floatVal; }
	virtual ~RPCVariable() {}
	static std::shared_ptr<RPCVariable> createError(int32_t faultCode, std::string faultString);
	void print();
	static std::string getTypeString(RPCVariableType type);
private:
	void print(std::shared_ptr<RPCVariable>, std::string indent);
	void printStruct(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
	void printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
};

} /* namespace XMLRPC */
#endif /* RPCVARIABLE_H_ */
