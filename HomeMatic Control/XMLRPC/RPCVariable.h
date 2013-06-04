#ifndef RPCVARIABLE_H_
#define RPCVARIABLE_H_

#include <vector>
#include <string>

namespace XMLRPC
{

	enum class RPCVariableType
	{
		rpcInteger = 0x01,
		rpcString = 0x03,
		rpcArray = 0x100,
		rpcStruct = 0x101
	};

	class RPCVariable {
	public:
		RPCVariableType type;
		std::string stringValue;
		int32_t integerValue = 0;
		std::vector<RPCVariable> arrayValue;
		std::vector<RPCVariable> structValue;

		RPCVariable() { type = RPCVariableType::rpcInteger; }
		RPCVariable(RPCVariableType variableType) { type = variableType; }
		virtual ~RPCVariable() {}
	};

} /* namespace XMLRPC */
#endif /* RPCVARIABLE_H_ */
