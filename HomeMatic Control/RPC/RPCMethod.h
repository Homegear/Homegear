#ifndef RPCMETHOD_H_
#define RPCMETHOD_H_

#include <vector>
#include <memory>

#include "RPCVariable.h"

namespace RPC
{

class RPCMethod {
public:
	RPCMethod() {}
	virtual ~RPCMethod() {}

	virtual std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters) { return std::shared_ptr<RPCVariable>(new RPCVariable()); }
};

} /* namespace RPC */
#endif /* RPCMETHOD_H_ */
