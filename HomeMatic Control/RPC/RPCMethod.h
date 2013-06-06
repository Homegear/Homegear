#ifndef RPCMETHOD_H_
#define RPCMETHOD_H_

#include <vector>
#include <memory>

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
};

} /* namespace RPC */
#endif /* RPCMETHOD_H_ */
