/* Copyright 2013-2014 Sathya Laufer
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
	_server.reset(new RPCServer());
	registerMethods();
}

void Server::registerMethods()
{
	try
	{
		_server->registerMethod("system.getCapabilities", std::shared_ptr<RPCMethod>(new RPCSystemGetCapabilities()));
		_server->registerMethod("system.listMethods", std::shared_ptr<RPCMethod>(new RPCSystemListMethods(_server)));
		_server->registerMethod("system.methodHelp", std::shared_ptr<RPCMethod>(new RPCSystemMethodHelp(_server)));
		_server->registerMethod("system.methodSignature", std::shared_ptr<RPCMethod>(new RPCSystemMethodSignature(_server)));
		_server->registerMethod("system.multicall", std::shared_ptr<RPCMethod>(new RPCSystemMulticall(_server)));
		_server->registerMethod("abortEventReset", std::shared_ptr<RPCMethod>(new RPCTriggerEvent()));
		_server->registerMethod("addDevice", std::shared_ptr<RPCMethod>(new RPCAddDevice()));
		_server->registerMethod("addEvent", std::shared_ptr<RPCMethod>(new RPCAddEvent()));
		_server->registerMethod("addLink", std::shared_ptr<RPCMethod>(new RPCAddLink()));
		_server->registerMethod("clientServerInitialized", std::shared_ptr<RPCMethod>(new RPCClientServerInitialized()));
		_server->registerMethod("deleteDevice", std::shared_ptr<RPCMethod>(new RPCDeleteDevice()));
		_server->registerMethod("deleteMetadata", std::shared_ptr<RPCMethod>(new RPCDeleteMetadata()));
		_server->registerMethod("enableEvent", std::shared_ptr<RPCMethod>(new RPCEnableEvent()));
		_server->registerMethod("getAllMetadata", std::shared_ptr<RPCMethod>(new RPCGetAllMetadata()));
		_server->registerMethod("getDeviceDescription", std::shared_ptr<RPCMethod>(new RPCGetDeviceDescription()));
		_server->registerMethod("getDeviceInfo", std::shared_ptr<RPCMethod>(new RPCGetDeviceInfo()));
		_server->registerMethod("getInstallMode", std::shared_ptr<RPCMethod>(new RPCGetInstallMode()));
		_server->registerMethod("getKeyMismatchDevice", std::shared_ptr<RPCMethod>(new RPCGetKeyMismatchDevice()));
		_server->registerMethod("getLinkInfo", std::shared_ptr<RPCMethod>(new RPCGetLinkInfo()));
		_server->registerMethod("getLinkPeers", std::shared_ptr<RPCMethod>(new RPCGetLinkPeers()));
		_server->registerMethod("getLinks", std::shared_ptr<RPCMethod>(new RPCGetLinks()));
		_server->registerMethod("getMetadata", std::shared_ptr<RPCMethod>(new RPCGetMetadata()));
		_server->registerMethod("getParamset", std::shared_ptr<RPCMethod>(new RPCGetParamset()));
		_server->registerMethod("getParamsetDescription", std::shared_ptr<RPCMethod>(new RPCGetParamsetDescription()));
		_server->registerMethod("getParamsetId", std::shared_ptr<RPCMethod>(new RPCGetParamsetId()));
		_server->registerMethod("getPeerId", std::shared_ptr<RPCMethod>(new RPCGetPeerId()));
		_server->registerMethod("getServiceMessages", std::shared_ptr<RPCMethod>(new RPCGetServiceMessages()));
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
		_server->registerMethod("runScript", std::shared_ptr<RPCMethod>(new RPCRunScript()));
		_server->registerMethod("searchDevices", std::shared_ptr<RPCMethod>(new RPCSearchDevices()));
		_server->registerMethod("setInstallMode", std::shared_ptr<RPCMethod>(new RPCSetInstallMode()));
		_server->registerMethod("setInterface", std::shared_ptr<RPCMethod>(new RPCSetInterface()));
		_server->registerMethod("setLinkInfo", std::shared_ptr<RPCMethod>(new RPCSetLinkInfo()));
		_server->registerMethod("setMetadata", std::shared_ptr<RPCMethod>(new RPCSetMetadata()));
		_server->registerMethod("setName", std::shared_ptr<RPCMethod>(new RPCSetName()));
		_server->registerMethod("setTeam", std::shared_ptr<RPCMethod>(new RPCSetTeam()));
		_server->registerMethod("setValue", std::shared_ptr<RPCMethod>(new RPCSetValue()));
		_server->registerMethod("triggerEvent", std::shared_ptr<RPCMethod>(new RPCTriggerEvent()));
		_server->registerMethod("updateFirmware", std::shared_ptr<RPCMethod>(new RPCUpdateFirmware()));
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

std::shared_ptr<BaseLib::RPC::RPCVariable> Server::callMethod(std::string methodName, std::shared_ptr<BaseLib::RPC::RPCVariable> parameters)
{
	if(!_server) return BaseLib::RPC::RPCVariable::createError(-32500, "Server is nullptr.");
	return _server->callMethod(methodName, parameters);
}

void Server::start(std::shared_ptr<ServerSettings::Settings>& settings)
{
	if(!_server) return;
	_server->start(settings);
}

void Server::stop()
{
	if(_server) _server->stop();
}

} /* namespace RPC */
