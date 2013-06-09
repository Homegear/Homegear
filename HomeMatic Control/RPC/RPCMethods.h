#ifndef RPCMETHODS_H_
#define RPCMETHODS_H_

#include <vector>
#include <memory>

#include "RPCVariable.h"
#include "RPCMethod.h"

namespace RPC
{

class RPCGetParamsetDescription : public RPCMethod
{
public:
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCInit : public RPCMethod
{
public:
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCListDevices : public RPCMethod
{
public:
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCLogLevel : public RPCMethod
{
public:
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

} /* namespace RPC */
#endif /* RPCMETHODS_H_ */
