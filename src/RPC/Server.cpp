/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "Server.h"
#include "../GD/GD.h"

namespace Rpc
{
Server::Server()
{
	if(!_server) _server.reset(new RPCServer());
}

Server::~Server()
{
	dispose();
}

void Server::dispose()
{
	if(_server) _server->dispose();
	_server.reset();
}

void Server::registerMethods()
{
	if(!_server) return;
	try
	{
		_server->registerMethod("devTest", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDevTest()));
		_server->registerMethod("system.getCapabilities", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSystemGetCapabilities()));
		_server->registerMethod("system.listMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSystemListMethods(_server)));
		_server->registerMethod("system.methodHelp", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSystemMethodHelp(_server)));
		_server->registerMethod("system.methodSignature", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSystemMethodSignature(_server)));
		_server->registerMethod("system.multicall", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSystemMulticall(_server)));
		_server->registerMethod("activateLinkParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCActivateLinkParamset()));
		_server->registerMethod("abortEventReset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCTriggerEvent()));
		_server->registerMethod("addCategoryToDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCAddCategoryToDevice()));
		_server->registerMethod("addDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCAddDevice()));
		_server->registerMethod("addDeviceToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCAddDeviceToRoom()));
		_server->registerMethod("addEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCAddEvent()));
		_server->registerMethod("addLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCAddLink()));
		_server->registerMethod("copyConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCCopyConfig()));
		_server->registerMethod("clientServerInitialized", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCClientServerInitialized()));
		_server->registerMethod("createCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCCreateCategory()));
		_server->registerMethod("createDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCCreateDevice()));
		_server->registerMethod("createRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCCreateRoom()));
		_server->registerMethod("deleteCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteCategory()));
		_server->registerMethod("deleteData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteData()));
		_server->registerMethod("deleteDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteDevice()));
		_server->registerMethod("deleteMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteMetadata()));
		_server->registerMethod("deleteNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteNodeData()));
		_server->registerMethod("deleteRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteRoom()));
		_server->registerMethod("deleteSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCDeleteSystemVariable()));
		_server->registerMethod("enableEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCEnableEvent()));
		_server->registerMethod("getAllConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetAllConfig()));
		_server->registerMethod("getAllMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetAllMetadata()));
		_server->registerMethod("getAllScripts", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetAllScripts()));
		_server->registerMethod("getAllSystemVariables", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetAllSystemVariables()));
		_server->registerMethod("getAllValues", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetAllValues()));
		_server->registerMethod("getCategories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetCategories()));
		_server->registerMethod("getConfigParameter", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetConfigParameter()));
		_server->registerMethod("getData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetData()));
		_server->registerMethod("getDeviceDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetDeviceDescription()));
		_server->registerMethod("getDeviceInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetDeviceInfo()));
		_server->registerMethod("getDevicesInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetDevicesInCategory()));
		_server->registerMethod("getDevicesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetDevicesInRoom()));
		_server->registerMethod("getEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetEvent()));
		_server->registerMethod("getLastEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetLastEvents()));
		_server->registerMethod("getInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetInstallMode()));
		_server->registerMethod("getKeyMismatchDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetKeyMismatchDevice()));
		_server->registerMethod("getLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetLinkInfo()));
		_server->registerMethod("getLinkPeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetLinkPeers()));
		_server->registerMethod("getLinks", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetLinks()));
		_server->registerMethod("getMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetMetadata()));
		_server->registerMethod("getName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetName()));
		_server->registerMethod("getNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetFlowData()));
		_server->registerMethod("getFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetGlobalData()));
		_server->registerMethod("getGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetNodeData()));
		_server->registerMethod("getNodeEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetNodeEvents()));
		_server->registerMethod("getPairingMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetPairingMethods()));
		_server->registerMethod("getParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetParamset()));
		_server->registerMethod("getParamsetDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetParamsetDescription()));
		_server->registerMethod("getParamsetId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetParamsetId()));
		_server->registerMethod("getPeerId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetPeerId()));
		_server->registerMethod("getRooms", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetRooms()));
		_server->registerMethod("getServiceMessages", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetServiceMessages()));
		_server->registerMethod("getSniffedDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetSniffedDevices()));
		_server->registerMethod("getSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetSystemVariable()));
		_server->registerMethod("getUpdateStatus", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetUpdateStatus()));
		_server->registerMethod("getValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetValue()));
		_server->registerMethod("getVariableDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetVariableDescription()));
		_server->registerMethod("getVersion", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetVersion()));
		_server->registerMethod("init", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCInit()));
		_server->registerMethod("listBidcosInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListBidcosInterfaces()));
		_server->registerMethod("listClientServers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListClientServers()));
		_server->registerMethod("listDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListDevices()));
		_server->registerMethod("listEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListEvents()));
		_server->registerMethod("listFamilies", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListFamilies()));
		_server->registerMethod("listInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListInterfaces()));
		_server->registerMethod("listKnownDeviceTypes", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListKnownDeviceTypes()));
		_server->registerMethod("listTeams", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCListTeams()));
		_server->registerMethod("logLevel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCLogLevel()));
		_server->registerMethod("nodeOutput", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCNodeOutput()));
		_server->registerMethod("ping", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCPing()));
		_server->registerMethod("putParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCPutParamset()));
		_server->registerMethod("removeCategoryFromDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRemoveCategoryFromDevice()));
		_server->registerMethod("removeDeviceFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRemoveDeviceFromRoom()));
		_server->registerMethod("getDevicesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCGetDevicesInRoom()));
		_server->registerMethod("removeEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRemoveEvent()));
		_server->registerMethod("removeLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRemoveLink()));
		_server->registerMethod("reportValueUsage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCReportValueUsage()));
		_server->registerMethod("rssiInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRssiInfo()));
		_server->registerMethod("runScript", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCRunScript()));
		_server->registerMethod("searchDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSearchDevices()));
		_server->registerMethod("setData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetData()));
		_server->registerMethod("setId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetId()));
		_server->registerMethod("setInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetInstallMode()));
		_server->registerMethod("setInterface", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetInterface()));
		_server->registerMethod("setLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetLinkInfo()));
		_server->registerMethod("setMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetMetadata()));
		_server->registerMethod("setName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetName()));
		_server->registerMethod("setNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetNodeData()));
		_server->registerMethod("setFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetFlowData()));
		_server->registerMethod("setGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetGlobalData()));
		_server->registerMethod("setNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetNodeVariable()));
		_server->registerMethod("setSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetSystemVariable()));
		_server->registerMethod("setTeam", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetTeam()));
		_server->registerMethod("setValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSetValue()));
		_server->registerMethod("startSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCStartSniffing()));
		_server->registerMethod("stopSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCStopSniffing()));
		_server->registerMethod("subscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCSubscribePeers()));
		_server->registerMethod("triggerEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCTriggerEvent()));
		_server->registerMethod("triggerRpcEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCTriggerRpcEvent()));
		_server->registerMethod("unsubscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCUnsubscribePeers()));
		_server->registerMethod("updateCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCUpdateCategory()));
		_server->registerMethod("updateFirmware", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCUpdateFirmware()));
		_server->registerMethod("updateRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCUpdateRoom()));
		_server->registerMethod("writeLog", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RPCWriteLog()));
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint32_t Server::connectionCount()
{
	if(_server) return _server->connectionCount(); else return 0;
}

BaseLib::PVariable Server::callMethod(std::string methodName, BaseLib::PVariable parameters)
{
	if(!_server) return BaseLib::Variable::createError(-32500, "Server is nullptr.");
	return _server->callMethod(methodName, parameters);
}

void Server::start(BaseLib::Rpc::PServerInfo& serverInfo)
{
	if(_server->getMethods()->size() == 0) registerMethods();
	_server->start(serverInfo);
}

void Server::stop()
{
	if(_server) _server->stop();
}

bool Server::lifetick()
{
	if(!_server) return true; return _server->lifetick();
}

bool Server::isRunning()
{
	if(!_server) return false; return _server->isRunning();
}

const std::vector<std::shared_ptr<BaseLib::RpcClientInfo>> Server::getClientInfo()
{
	if(!_server) return std::vector<std::shared_ptr<BaseLib::RpcClientInfo>>(); return _server->getClientInfo();
}

const std::shared_ptr<RPCServer> Server::getServer()
{
	return _server;
}

const BaseLib::Rpc::PServerInfo Server::getInfo()
{
	if(!_server) return BaseLib::Rpc::PServerInfo(); return _server->getInfo();
}

BaseLib::PEventHandler Server::addWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler)
{
	if(!_server) return BaseLib::PEventHandler();
	return _server->addWebserverEventHandler(eventHandler);
}

void Server::removeWebserverEventHandler(BaseLib::PEventHandler eventHandler)
{
	if(!_server) return;
	_server->removeWebserverEventHandler(eventHandler);
}

}
