#ifndef RPCMETHOD_H_
#define RPCMETHOD_H_

#include <vector>
#include <memory>

#include "../Exception.h"
#include "RPCVariable.h"

namespace RPC
{

class RPCMethod
{
public:
	struct ParameterError
	{
		enum Enum { noError, wrongCount, wrongType };
	};

	RPCMethod() {}
	virtual ~RPCMethod() {}

	ParameterError::Enum checkParameters(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, std::vector<RPCVariableType> types);
	virtual std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<RPCVariable> getError(ParameterError::Enum error);
	std::shared_ptr<RPCVariable> getSignature() { return _signatures; }
	std::shared_ptr<RPCVariable> getHelp() { return _help; }
protected:
	std::shared_ptr<RPCVariable> _signatures;
	std::shared_ptr<RPCVariable> _help;

	void addSignature(RPCVariableType returnType, std::vector<RPCVariableType> parameterTypes);
	void setHelp(std::string help);
};

} /* namespace RPC */
#endif /* RPCMETHOD_H_ */
