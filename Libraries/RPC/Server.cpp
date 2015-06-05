/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "Server.h"
#include "../GD/GD.h"

namespace RPC
{
Server::Server()
{
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
		_server->registerMethod("system.getCapabilities", std::shared_ptr<RPCMethod>(new RPCSystemGetCapabilities()));
		_server->registerMethod("system.listMethods", std::shared_ptr<RPCMethod>(new RPCSystemListMethods(_server)));
		_server->registerMethod("system.methodHelp", std::shared_ptr<RPCMethod>(new RPCSystemMethodHelp(_server)));
		_server->registerMethod("system.methodSignature", std::shared_ptr<RPCMethod>(new RPCSystemMethodSignature(_server)));
		_server->registerMethod("system.multicall", std::shared_ptr<RPCMethod>(new RPCSystemMulticall(_server)));
		_server->registerMethod("activateLinkParamset", std::shared_ptr<RPCMethod>(new RPCActivateLinkParamset()));
		_server->registerMethod("abortEventReset", std::shared_ptr<RPCMethod>(new RPCTriggerEvent()));
		_server->registerMethod("addDevice", std::shared_ptr<RPCMethod>(new RPCAddDevice()));
		_server->registerMethod("addEvent", std::shared_ptr<RPCMethod>(new RPCAddEvent()));
		_server->registerMethod("addLink", std::shared_ptr<RPCMethod>(new RPCAddLink()));
		_server->registerMethod("clientServerInitialized", std::shared_ptr<RPCMethod>(new RPCClientServerInitialized()));
		_server->registerMethod("createDevice", std::shared_ptr<RPCMethod>(new RPCCreateDevice()));
		_server->registerMethod("deleteDevice", std::shared_ptr<RPCMethod>(new RPCDeleteDevice()));
		_server->registerMethod("deleteMetadata", std::shared_ptr<RPCMethod>(new RPCDeleteMetadata()));
		_server->registerMethod("deleteSystemVariable", std::shared_ptr<RPCMethod>(new RPCDeleteSystemVariable()));
		_server->registerMethod("enableEvent", std::shared_ptr<RPCMethod>(new RPCEnableEvent()));
		_server->registerMethod("getAllMetadata", std::shared_ptr<RPCMethod>(new RPCGetAllMetadata()));
		_server->registerMethod("getAllScripts", std::shared_ptr<RPCMethod>(new RPCGetAllScripts()));
		_server->registerMethod("getAllSystemVariables", std::shared_ptr<RPCMethod>(new RPCGetAllSystemVariables()));
		_server->registerMethod("getAllValues", std::shared_ptr<RPCMethod>(new RPCGetAllValues()));
		_server->registerMethod("getDeviceDescription", std::shared_ptr<RPCMethod>(new RPCGetDeviceDescription()));
		_server->registerMethod("getDeviceInfo", std::shared_ptr<RPCMethod>(new RPCGetDeviceInfo()));
		_server->registerMethod("getEvent", std::shared_ptr<RPCMethod>(new RPCGetEvent()));
		_server->registerMethod("getInstallMode", std::shared_ptr<RPCMethod>(new RPCGetInstallMode()));
		_server->registerMethod("getKeyMismatchDevice", std::shared_ptr<RPCMethod>(new RPCGetKeyMismatchDevice()));
		_server->registerMethod("getLinkInfo", std::shared_ptr<RPCMethod>(new RPCGetLinkInfo()));
		_server->registerMethod("getLinkPeers", std::shared_ptr<RPCMethod>(new RPCGetLinkPeers()));
		_server->registerMethod("getLinks", std::shared_ptr<RPCMethod>(new RPCGetLinks()));
		_server->registerMethod("getMetadata", std::shared_ptr<RPCMethod>(new RPCGetMetadata()));
		_server->registerMethod("getName", std::shared_ptr<RPCMethod>(new RPCGetName()));
		_server->registerMethod("getParamset", std::shared_ptr<RPCMethod>(new RPCGetParamset()));
		_server->registerMethod("getParamsetDescription", std::shared_ptr<RPCMethod>(new RPCGetParamsetDescription()));
		_server->registerMethod("getParamsetId", std::shared_ptr<RPCMethod>(new RPCGetParamsetId()));
		_server->registerMethod("getPeerId", std::shared_ptr<RPCMethod>(new RPCGetPeerId()));
		_server->registerMethod("getServiceMessages", std::shared_ptr<RPCMethod>(new RPCGetServiceMessages()));
		_server->registerMethod("getSystemVariable", std::shared_ptr<RPCMethod>(new RPCGetSystemVariable()));
		_server->registerMethod("getUpdateStatus", std::shared_ptr<RPCMethod>(new RPCGetUpdateStatus()));
		_server->registerMethod("getValue", std::shared_ptr<RPCMethod>(new RPCGetValue()));
		_server->registerMethod("getVersion", std::shared_ptr<RPCMethod>(new RPCGetVersion()));
		_server->registerMethod("init", std::shared_ptr<RPCMethod>(new RPCInit()));
		_server->registerMethod("listBidcosInterfaces", std::shared_ptr<RPCMethod>(new RPCListBidcosInterfaces()));
		_server->registerMethod("listClientServers", std::shared_ptr<RPCMethod>(new RPCListClientServers()));
		_server->registerMethod("listDevices", std::shared_ptr<RPCMethod>(new RPCListDevices()));
		_server->registerMethod("listEvents", std::shared_ptr<RPCMethod>(new RPCListEvents()));
		_server->registerMethod("listFamilies", std::shared_ptr<RPCMethod>(new RPCListFamilies()));
		_server->registerMethod("listInterfaces", std::shared_ptr<RPCMethod>(new RPCListInterfaces()));
		_server->registerMethod("listTeams", std::shared_ptr<RPCMethod>(new RPCListTeams()));
		_server->registerMethod("logLevel", std::shared_ptr<RPCMethod>(new RPCLogLevel()));
		_server->registerMethod("putParamset", std::shared_ptr<RPCMethod>(new RPCPutParamset()));
		_server->registerMethod("removeEvent", std::shared_ptr<RPCMethod>(new RPCRemoveEvent()));
		_server->registerMethod("removeLink", std::shared_ptr<RPCMethod>(new RPCRemoveLink()));
		_server->registerMethod("reportValueUsage", std::shared_ptr<RPCMethod>(new RPCReportValueUsage()));
		_server->registerMethod("rssiInfo", std::shared_ptr<RPCMethod>(new RPCRssiInfo()));
		_server->registerMethod("runScript", std::shared_ptr<RPCMethod>(new RPCRunScript()));
		_server->registerMethod("searchDevices", std::shared_ptr<RPCMethod>(new RPCSearchDevices()));
		_server->registerMethod("setId", std::shared_ptr<RPCMethod>(new RPCSetId()));
		_server->registerMethod("setInstallMode", std::shared_ptr<RPCMethod>(new RPCSetInstallMode()));
		_server->registerMethod("setInterface", std::shared_ptr<RPCMethod>(new RPCSetInterface()));
		_server->registerMethod("setLinkInfo", std::shared_ptr<RPCMethod>(new RPCSetLinkInfo()));
		_server->registerMethod("setMetadata", std::shared_ptr<RPCMethod>(new RPCSetMetadata()));
		_server->registerMethod("setName", std::shared_ptr<RPCMethod>(new RPCSetName()));
		_server->registerMethod("setSystemVariable", std::shared_ptr<RPCMethod>(new RPCSetSystemVariable()));
		_server->registerMethod("setTeam", std::shared_ptr<RPCMethod>(new RPCSetTeam()));
		_server->registerMethod("setValue", std::shared_ptr<RPCMethod>(new RPCSetValue()));
		_server->registerMethod("subscribePeers", std::shared_ptr<RPCMethod>(new RPCSubscribePeers()));
		_server->registerMethod("triggerEvent", std::shared_ptr<RPCMethod>(new RPCTriggerEvent()));
		_server->registerMethod("triggerRPCEvent", std::shared_ptr<RPCMethod>(new RPCTriggerRPCEvent()));
		_server->registerMethod("unsubscribePeers", std::shared_ptr<RPCMethod>(new RPCUnsubscribePeers()));
		_server->registerMethod("updateFirmware", std::shared_ptr<RPCMethod>(new RPCUpdateFirmware()));
		_server->registerMethod("writeLog", std::shared_ptr<RPCMethod>(new RPCWriteLog()));
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

std::shared_ptr<BaseLib::RPC::Variable> Server::callMethod(std::string methodName, std::shared_ptr<BaseLib::RPC::Variable> parameters)
{
	if(!_server) return BaseLib::RPC::Variable::createError(-32500, "Server is nullptr.");
	return _server->callMethod(methodName, parameters);
}

void Server::start(std::shared_ptr<ServerInfo::Info>& serverInfo)
{
	if(!_server) _server.reset(new RPCServer());
	if(_server->getMethods()->size() == 0) registerMethods();
	_server->start(serverInfo);
}

void Server::stop()
{
	if(_server) _server->stop();
}

int32_t Server::checkAddonClient(int32_t clientID)
{
	if(!_server) return -1;
	return _server->isAddonClient(clientID);
}

int32_t Server::isAddonClient(int32_t clientID)
{
	int32_t result = -1;
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		result = i->second.checkAddonClient(clientID);
		if(result != -1) return result;
	}
	return -1;
}

std::string Server::getClientIP(int32_t clientID)
{
	std::string ipAddress;
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		ipAddress = i->second.getClientIP(clientID);
		if(!ipAddress.empty()) return ipAddress;
	}
	return "";
}

}
