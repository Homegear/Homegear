#include "Server.h"
#include "RPCMethods.h"

namespace RPC {

void Server::registerMethods()
{
	_server.registerMethod("getParamsetDescription", std::shared_ptr<RPCMethod>(new RPCGetParamsetDescription()));
	_server.registerMethod("init", std::shared_ptr<RPCMethod>(new RPCInit()));
	_server.registerMethod("listDevices", std::shared_ptr<RPCMethod>(new RPCListDevices()));
	_server.registerMethod("logLevel", std::shared_ptr<RPCMethod>(new RPCLogLevel()));
}

void Server::start()
{
	registerMethods();
	_server.start();
}

} /* namespace RPC */
