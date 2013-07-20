#include "Server.h"
#include "RPCMethods.h"

namespace RPC {

void Server::registerMethods()
{
	_server->registerMethod("system.getCapabilities", std::shared_ptr<RPCMethod>(new RPCSystemGetCapabilities()));
	_server->registerMethod("system.listMethods", std::shared_ptr<RPCMethod>(new RPCSystemListMethods(_server)));
	_server->registerMethod("system.methodHelp", std::shared_ptr<RPCMethod>(new RPCSystemMethodHelp(_server)));
	_server->registerMethod("system.methodSignature", std::shared_ptr<RPCMethod>(new RPCSystemMethodSignature(_server)));
	_server->registerMethod("system.multicall", std::shared_ptr<RPCMethod>(new RPCSystemMulticall(_server)));
	_server->registerMethod("addLink", std::shared_ptr<RPCMethod>(new RPCAddLink()));
	_server->registerMethod("deleteDevice", std::shared_ptr<RPCMethod>(new RPCDeleteDevice()));
	_server->registerMethod("deleteMetadata", std::shared_ptr<RPCMethod>(new RPCDeleteMetadata()));
	_server->registerMethod("getAllMetadata", std::shared_ptr<RPCMethod>(new RPCGetAllMetadata()));
	_server->registerMethod("getDeviceDescription", std::shared_ptr<RPCMethod>(new RPCGetDeviceDescription()));
	_server->registerMethod("getInstallMode", std::shared_ptr<RPCMethod>(new RPCGetInstallMode()));
	_server->registerMethod("getKeyMismatchDevice", std::shared_ptr<RPCMethod>(new RPCGetKeyMismatchDevice()));
	_server->registerMethod("getLinkInfo", std::shared_ptr<RPCMethod>(new RPCGetLinkInfo()));
	_server->registerMethod("getLinkPeers", std::shared_ptr<RPCMethod>(new RPCGetLinkPeers()));
	_server->registerMethod("getLinks", std::shared_ptr<RPCMethod>(new RPCGetLinks()));
	_server->registerMethod("getMetadata", std::shared_ptr<RPCMethod>(new RPCGetMetadata()));
	_server->registerMethod("getParamset", std::shared_ptr<RPCMethod>(new RPCGetParamset()));
	_server->registerMethod("getParamsetDescription", std::shared_ptr<RPCMethod>(new RPCGetParamsetDescription()));
	_server->registerMethod("getParamsetId", std::shared_ptr<RPCMethod>(new RPCGetParamsetId()));
	_server->registerMethod("getServiceMessages", std::shared_ptr<RPCMethod>(new RPCGetServiceMessages()));
	_server->registerMethod("getValue", std::shared_ptr<RPCMethod>(new RPCGetValue()));
	_server->registerMethod("init", std::shared_ptr<RPCMethod>(new RPCInit()));
	_server->registerMethod("listBidcosInterfaces", std::shared_ptr<RPCMethod>(new RPCListBidcosInterfaces()));
	_server->registerMethod("listDevices", std::shared_ptr<RPCMethod>(new RPCListDevices()));
	_server->registerMethod("listTeams", std::shared_ptr<RPCMethod>(new RPCListTeams()));
	_server->registerMethod("logLevel", std::shared_ptr<RPCMethod>(new RPCLogLevel()));
	_server->registerMethod("putParamset", std::shared_ptr<RPCMethod>(new RPCPutParamset()));
	_server->registerMethod("removeLink", std::shared_ptr<RPCMethod>(new RPCRemoveLink()));
	_server->registerMethod("setInstallMode", std::shared_ptr<RPCMethod>(new RPCSetInstallMode()));
	_server->registerMethod("setLinkInfo", std::shared_ptr<RPCMethod>(new RPCSetLinkInfo()));
	_server->registerMethod("setMetadata", std::shared_ptr<RPCMethod>(new RPCSetMetadata()));
	_server->registerMethod("setTeam", std::shared_ptr<RPCMethod>(new RPCSetTeam()));
	_server->registerMethod("setValue", std::shared_ptr<RPCMethod>(new RPCSetValue()));
}

void Server::start()
{
	registerMethods();
	_server->start();
}

void Server::stop()
{
	_server->stop();
}

} /* namespace RPC */
