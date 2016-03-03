/* Copyright 2013-2016 Sathya Laufer
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

#include "ScriptEngineServer.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"
#include "php_sapi.h"

namespace ScriptEngine
{

ScriptEngineServer::ScriptEngineServer() : IQueue(GD::bl.get(), 1000)
{
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine Server: ");

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("devTest", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCDevTest())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("system.getCapabilities", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSystemGetCapabilities())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("system.listMethods", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSystemListMethods(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("system.methodHelp", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSystemMethodHelp(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("system.methodSignature", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSystemMethodSignature(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("system.multicall", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSystemMulticall(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("activateLinkParamset", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCActivateLinkParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("abortEventReset", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCTriggerEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("addDevice", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCAddDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("addEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCAddEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("addLink", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCAddLink())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("copyConfig", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCCopyConfig())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("clientServerInitialized", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCClientServerInitialized())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("createDevice", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCCreateDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("deleteDevice", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCDeleteDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("deleteMetadata", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCDeleteMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("deleteSystemVariable", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCDeleteSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("enableEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCEnableEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getAllConfig", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetAllConfig())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getAllMetadata", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetAllMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getAllScripts", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetAllScripts())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getAllSystemVariables", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetAllSystemVariables())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getAllValues", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetAllValues())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getConfigParameter", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetConfigParameter())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getDeviceDescription", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetDeviceDescription())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getDeviceInfo", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetDeviceInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getInstallMode", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetInstallMode())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getKeyMismatchDevice", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetKeyMismatchDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getLinkInfo", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetLinkInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getLinkPeers", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetLinkPeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getLinks", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetLinks())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getMetadata", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getName", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetName())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getPairingMethods", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetPairingMethods())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getParamset", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getParamsetDescription", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetParamsetDescription())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getParamsetId", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetParamsetId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getPeerId", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetPeerId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getServiceMessages", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetServiceMessages())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getSystemVariable", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getUpdateStatus", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetUpdateStatus())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getValue", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetValue())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("getVersion", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCGetVersion())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("init", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCInit())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listBidcosInterfaces", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListBidcosInterfaces())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listClientServers", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListClientServers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listDevices", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListDevices())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listEvents", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListEvents())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listFamilies", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListFamilies())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listInterfaces", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListInterfaces())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listKnownDeviceTypes", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListKnownDeviceTypes())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("listTeams", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCListTeams())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("logLevel", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCLogLevel())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("putParamset", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCPutParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("removeEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCRemoveEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("removeLink", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCRemoveLink())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("reportValueUsage", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCReportValueUsage())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("rssiInfo", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCRssiInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("runScript", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCRunScript())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("searchDevices", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSearchDevices())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setId", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setInstallMode", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetInstallMode())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setInterface", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetInterface())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setLinkInfo", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetLinkInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setMetadata", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setName", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetName())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setSystemVariable", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setTeam", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetTeam())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("setValue", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSetValue())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("subscribePeers", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCSubscribePeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("triggerEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCTriggerEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("triggerRPCEvent", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCTriggerRPCEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("unsubscribePeers", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCUnsubscribePeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("updateFirmware", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCUpdateFirmware())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<RPC::RPCMethod>>("writeLog", std::shared_ptr<RPC::RPCMethod>(new RPC::RPCWriteLog())));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("registerScriptEngineClient", std::bind(&ScriptEngineServer::registerScriptEngineClient, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("scriptFinished", std::bind(&ScriptEngineServer::scriptFinished, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("scriptOutput", std::bind(&ScriptEngineServer::scriptOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("scriptHeaders", std::bind(&ScriptEngineServer::scriptHeaders, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("peerExists", std::bind(&ScriptEngineServer::peerExists, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("listRpcClients", std::bind(&ScriptEngineServer::listRpcClients, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("auth", std::bind(&ScriptEngineServer::auth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("createUser", std::bind(&ScriptEngineServer::createUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("deleteUser", std::bind(&ScriptEngineServer::deleteUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("updateUser", std::bind(&ScriptEngineServer::updateUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("userExists", std::bind(&ScriptEngineServer::userExists, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("listUsers", std::bind(&ScriptEngineServer::listUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("listModules", std::bind(&ScriptEngineServer::listModules, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("loadModule", std::bind(&ScriptEngineServer::loadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("unloadModule", std::bind(&ScriptEngineServer::unloadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("reloadModule", std::bind(&ScriptEngineServer::reloadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("checkLicense", std::bind(&ScriptEngineServer::checkLicense, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("removeLicense", std::bind(&ScriptEngineServer::removeLicense, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("getLicenseStates", std::bind(&ScriptEngineServer::getLicenseStates, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

	php_homegear_init();
}

ScriptEngineServer::~ScriptEngineServer()
{
	if(!_stopServer) stop();
	GD::bl->threadManager.join(_checkSessionIdThread);
	php_homegear_shutdown();
}

void ScriptEngineServer::collectGarbage()
{
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<PScriptEngineProcess> processesToShutdown;
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			bool emptyProcess = false;
			for(std::map<pid_t, PScriptEngineProcess>::iterator i = _processes.begin(); i != _processes.end(); ++i)
			{
				if(i->second->scriptCount() == 0 && i->second->getClientData() && !i->second->getClientData()->closed)
				{
					if(emptyProcess) closeClientConnection(i->second->getClientData());
					else emptyProcess = true;
				}
			}
		}

		std::vector<PScriptEngineClientData> clientsToRemove;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			try
			{
				for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
				{
					if(i->second->closed) clientsToRemove.push_back(i->second);
				}
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		for(std::vector<PScriptEngineClientData>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				_clients.erase((*i)->id);
			}
			_out.printDebug("Debug: CLI client " + std::to_string((*i)->id) + " removed.");
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool ScriptEngineServer::start()
{
	try
	{
		stop();
		_socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
		_shuttingDown = false;
		_stopServer = false;
		if(!getFileDescriptor(true)) return false;
		startQueue(0, GD::bl->settings.scriptEngineThreadCount(), 0, SCHED_OTHER);
		GD::bl->threadManager.start(_mainThread, true, &ScriptEngineServer::mainThread, this);
		return true;
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void ScriptEngineServer::stop()
{
	try
	{
		_shuttingDown = true;
		_out.printDebug("Debug: Waiting for script engine server's client threads to finish.");
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				clients.push_back(i->second);
				closeClientConnection(i->second);
				std::lock_guard<std::mutex> processGuard(_processMutex);
				std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(i->second->pid);
				if(processIterator != _processes.end())
				{
					processIterator->second->invokeScriptFinished(-32501);
					_processes.erase(processIterator);
				}
			}
		}

		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		while(_clients.size() > 0)
		{
			for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
			{
				(*i)->requestConditionVariable.notify_all();
			}
			collectGarbage();
			if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		stopQueue(0);
		unlink(_socketPath.c_str());
	}
	catch(const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ScriptEngineServer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}
		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array());
			sendRequest(*i, "shutdown", parameters);
		}

		int32_t i = 0;
		while(_clients.size() > 0 && i < 60)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			collectGarbage();
			i++;
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::homegearReloading()
{
	try
	{
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}
		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array());
			sendRequest(*i, "reload", parameters);
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped)
{
	try
	{
		std::shared_ptr<ScriptEngineProcess> process;

		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
			if(processIterator != _processes.end())
			{
				process = processIterator->second;
				closeClientConnection(process->getClientData());
				_processes.erase(processIterator);
			}
		}

		if(signal != -1) _out.printCritical("Critical: Client process with pid " + std::to_string(pid) + " was killed with signal " + std::to_string(signal) + '.');
		else if(exitCode != 0) _out.printError("Error: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');
		else _out.printInfo("Info: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');
		if(process)
		{
			if(signal != -1) exitCode = -32500;
			process->invokeScriptFinished(exitCode);
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint32_t ScriptEngineServer::scriptCount()
{
	try
	{
		if(_shuttingDown) return 0;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		uint32_t count = 0;
		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array());
			BaseLib::PVariable response = sendRequest(*i, "scriptCount", parameters);
			count += response->integerValue;
		}
		return count;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

void ScriptEngineServer::broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, BaseLib::PArray values)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(*variables)), BaseLib::PVariable(new BaseLib::Variable(values))});
			BaseLib::PVariable response = sendRequest(*i, "broadcastEvent", parameters);
			if(response->errorStruct)
			{
				_out.printError("Error calling \"broadcastEvent\" on client " + std::to_string((*i)->id) + ": " + response->structValue->at("faultString")->stringValue);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::broadcastNewDevices(BaseLib::PVariable deviceDescriptions)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{deviceDescriptions});
			BaseLib::PVariable response = sendRequest(*i, "broadcastNewDevices", parameters);
			if(response->errorStruct)
			{
				_out.printError("Error calling \"broadcastNewDevices\" on client " + std::to_string((*i)->id) + ": " + response->structValue->at("faultString")->stringValue);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::broadcastDeleteDevices(BaseLib::PVariable deviceInfo)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{deviceInfo});
			BaseLib::PVariable response = sendRequest(*i, "broadcastDeleteDevices", parameters);
			if(response->errorStruct)
			{
				_out.printError("Error calling \"broadcastDeleteDevices\" on client " + std::to_string((*i)->id) + ": " + response->structValue->at("faultString")->stringValue);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PScriptEngineClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(hint))});
			BaseLib::PVariable response = sendRequest(*i, "broadcastUpdateDevice", parameters);
			if(response->errorStruct)
			{
				_out.printError("Error calling \"broadcastUpdateDevice\" on client " + std::to_string((*i)->id) + ": " + response->structValue->at("faultString")->stringValue);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::closeClientConnection(PScriptEngineClientData client)
{
	try
	{
		if(!client) return;
		GD::bl->fileDescriptorManager.shutdown(client->fileDescriptor);
		client->closed = true;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry || queueEntry->clientData->closed) return;

		if(queueEntry->isRequest)
		{
			std::string methodName;
			BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

			if(parameters->size() != 3)
			{
				_out.printError("Error: Wrong parameter count while calling method " + methodName);
				return;
			}

			std::map<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
			if(localMethodIterator != _localRpcMethods.end())
			{
				if(GD::bl->debugLevel >= 4)
				{
					_out.printDebug("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName);
					if(GD::bl->debugLevel >= 5)
					{
						for(BaseLib::Array::iterator i = parameters->at(2)->arrayValue->begin(); i != parameters->at(2)->arrayValue->end(); ++i)
						{
							(*i)->print();
						}
					}
				}
				BaseLib::PVariable result = localMethodIterator->second(queueEntry->clientData, parameters->at(0)->integerValue, parameters->at(2)->arrayValue);
				if(GD::bl->debugLevel >= 5)
				{
					_out.printDebug("Response: ");
					result->print();
				}

				sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
				return;
			}

			std::map<std::string, std::shared_ptr<RPC::RPCMethod>>::iterator methodIterator = _rpcMethods.find(methodName);
			if(methodIterator == _rpcMethods.end())
			{
				_out.printError("Error: RPC method not found: " + methodName);
				BaseLib::PVariable error = BaseLib::Variable::createError(-32601, ": Requested method not found.");
				sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), error);
				return;
			}

			if(GD::bl->debugLevel >= 4)
			{
				_out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName + " Parameters:");
				for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(2)->arrayValue->begin(); i != parameters->at(2)->arrayValue->end(); ++i)
				{
					(*i)->print();
				}
			}
			BaseLib::PVariable result = _rpcMethods.at(methodName)->invoke(_dummyClientInfo, parameters->at(2)->arrayValue);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print();
			}

			sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
		}
		else
		{
			BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
			{
				std::lock_guard<std::mutex> responseGuard(queueEntry->clientData->rpcResponsesMutex);
				BaseLib::PPVariable element = queueEntry->clientData->rpcResponses[response->arrayValue->at(0)->integerValue];
				if(element) *element = response;
			}
			queueEntry->clientData->requestConditionVariable.notify_one();
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BaseLib::PVariable ScriptEngineServer::send(PScriptEngineClientData& clientData, std::vector<char>& data)
{
	try
	{
		int32_t totallySentBytes = 0;
		std::lock_guard<std::mutex> sendGuard(clientData->sendMutex);
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sentBytes = ::send(clientData->fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
			if(sentBytes <= 0)
			{
				GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
				return BaseLib::Variable::createError(-32500, "Unknown application error.");
			}
			totallySentBytes += sentBytes;
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::PVariable(new BaseLib::Variable());
}

BaseLib::PVariable ScriptEngineServer::sendRequest(PScriptEngineClientData& clientData, std::string methodName, BaseLib::PArray& parameters)
{
	try
	{
		int32_t packetId;
		{
			std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
			packetId = _currentPacketId++;
		}
		BaseLib::PArray array(new BaseLib::Array{ BaseLib::PVariable(new BaseLib::Variable(packetId)), BaseLib::PVariable(new BaseLib::Variable(parameters)) });
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, array, data);

		BaseLib::PPVariable response;
		{
			std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
			BaseLib::PPVariable* element = &clientData->rpcResponses[packetId];
			element->reset(new BaseLib::PVariable());
			response = *element;
		}
		response->reset(new BaseLib::Variable());

		send(clientData, data);

		std::unique_lock<std::mutex> waitLock(clientData->waitMutex);
		while(!clientData->requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&]{
			return ((bool)(*response) && (*response)->arrayValue->size() == 2 && (*response)->arrayValue->at(0)->integerValue == packetId) || clientData->closed || _stopServer;
		}));

		if(!(*response) || (*response)->arrayValue->size() != 2 || (*response)->arrayValue->at(0)->integerValue != packetId)
		{
			_out.printError("Error: No response received to RPC request. Method: " + methodName);
			*response = BaseLib::Variable::createError(-1, "No response received.");
		}

		{
			std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
			clientData->rpcResponses.erase(packetId);
		}

		return (*response)->arrayValue->at(1);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineServer::sendResponse(PScriptEngineClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{ scriptId, packetId, variable })));
		std::vector<char> data;
		_rpcEncoder->encodeResponse(array, data);
		send(clientData, data);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ScriptEngineServer::mainThread()
{
	try
	{
		int32_t result = 0;
		std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor;
		while(!_stopServer)
		{
			if(!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				getFileDescriptor();
				continue;
			}

			timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			GD::bl->fileDescriptorManager.lock();
			int32_t maxfd = _serverFileDescriptor->descriptor;
			FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);

			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
				{
					if(i->second->closed) continue;
					if(i->second->fileDescriptor->descriptor == -1)
					{
						i->second->closed = true;
						continue;
					}
					FD_SET(i->second->fileDescriptor->descriptor, &readFileDescriptor);
					if(i->second->fileDescriptor->descriptor > maxfd) maxfd = i->second->fileDescriptor->descriptor;
				}
			}

			GD::bl->fileDescriptorManager.unlock();

			result = select(maxfd + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(result == 0)
			{
				if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.scriptEngineServerMaxConnections() * 100 / 112) collectGarbage();
				continue;
			}
			else if(result == -1)
			{
				if(errno == EINTR) continue;
				_out.printError("Error: select returned -1: " + std::string(strerror(errno)));
				continue;
			}

			if(FD_ISSET(_serverFileDescriptor->descriptor, &readFileDescriptor))
			{
				sockaddr_un clientAddress;
				socklen_t addressSize = sizeof(addressSize);
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientAddress, &addressSize));
				if(!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;
				_out.printInfo("Info: Connection accepted. Client number: " + std::to_string(clientFileDescriptor->id));

				if(_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections())
				{
					collectGarbage();
					if(_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections())
					{
						_out.printError("Error: There are too many clients connected to me. Closing connection. You can increase the number of allowed connections in main.conf.");
						GD::bl->fileDescriptorManager.close(clientFileDescriptor);
						continue;
					}
				}

				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				if(_shuttingDown)
				{
					GD::bl->fileDescriptorManager.close(clientFileDescriptor);
					continue;
				}
				PScriptEngineClientData clientData = PScriptEngineClientData(new ScriptEngineClientData(clientFileDescriptor));
				clientData->id = _currentClientId++;
				_clients[clientData->id] = clientData;
				continue;
			}

			PScriptEngineClientData clientData;
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				for(std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
				{
					if(i->second->fileDescriptor->descriptor == -1)
					{
						i->second->closed = true;
						continue;
					}
					if(FD_ISSET(i->second->fileDescriptor->descriptor, &readFileDescriptor))
					{
						clientData = i->second;
						break;
					}
				}
			}

			if(clientData) readClient(clientData);
		}
		GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

PScriptEngineProcess ScriptEngineServer::getFreeProcess()
{
	try
	{
		std::lock_guard<std::mutex> processGuard(_newProcessMutex);
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			for(std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator i = _processes.begin(); i != _processes.end(); ++i)
			{
				if(i->second->scriptCount() < GD::bl->threadManager.getMaxThreadCount() / GD::bl->settings.scriptEngineMaxThreadsPerScript() && (GD::bl->settings.scriptEngineMaxScriptsPerProcess() == -1 || i->second->scriptCount() < (unsigned)GD::bl->settings.scriptEngineMaxScriptsPerProcess()))
				{
					return i->second;
				}
			}
		}
		_out.printInfo("Info: Spawning new script engine process.");
		std::shared_ptr<ScriptEngineProcess> process(new ScriptEngineProcess());
		std::vector<std::string> arguments{ "-c", GD::configPath, "-rse" };
		process->setPid(GD::bl->hf.system(GD::executablePath + "/" + GD::executableFile, arguments));
		if(process->getPid() != -1)
		{
			{
				std::lock_guard<std::mutex> processGuard(_processMutex);
				_processes[process->getPid()] = process;
			}

			std::mutex requestMutex;
			std::unique_lock<std::mutex> requestLock(requestMutex);
			process->requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(30000), [&]{ return (bool)(process->getClientData()); });

			if(!process->getClientData())
			{
				std::lock_guard<std::mutex> processGuard(_processMutex);
				_processes.erase(process->getPid());
				_out.printError("Error: Could not start new script engine process.");
				return std::shared_ptr<ScriptEngineProcess>();
			}
			_out.printInfo("Info: Script engine process successfully spawned. Process id is " + std::to_string(process->getPid()) + ". Client id is: " + std::to_string(process->getClientData()->id) + ".");
			return process;
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<ScriptEngineProcess>();
}

void ScriptEngineServer::readClient(PScriptEngineClientData& clientData)
{
	try
	{
		int32_t processedBytes = 0;
		int32_t bytesRead = 0;
		bytesRead = read(clientData->fileDescriptor->descriptor, &(clientData->buffer[0]), clientData->buffer.size());
		if(bytesRead <= 0) //read returns 0, when connection is disrupted.
		{
			_out.printInfo("Info: Connection to script server's client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
			closeClientConnection(clientData);
			return;
		}

		if(bytesRead > (signed)clientData->buffer.size()) bytesRead = clientData->buffer.size();

		try
		{
			processedBytes = 0;
			while(processedBytes < bytesRead)
			{
				processedBytes += clientData->binaryRpc->process(&(clientData->buffer[processedBytes]), bytesRead - processedBytes);
				if(clientData->binaryRpc->isFinished())
				{
					std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(clientData, clientData->binaryRpc->getData(), clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request));
					enqueue(0, queueEntry);
					clientData->binaryRpc->reset();
				}
			}
		}
		catch(BaseLib::Rpc::BinaryRpcException& ex)
		{
			_out.printError("Error processing packet: " + ex.what());
			clientData->binaryRpc->reset();
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool ScriptEngineServer::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		struct stat sb;
		if(stat(GD::bl->settings.socketPath().c_str(), &sb) == -1)
		{
			if(errno == ENOENT) _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the script engine won't work.");
			else _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath() + ". The script engine won't work: " + strerror(errno));
			_stopServer = true;
			return false;
		}
		if(!S_ISDIR(sb.st_mode))
		{
			_out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the script engine interface won't work.");
			_stopServer = true;
			return false;
		}
		if(deleteOldSocket)
		{
			if(unlink(_socketPath.c_str()) == -1 && errno != ENOENT)
			{
				_out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath + ". Please delete it manually. The script engine won't work. Error: " + strerror(errno));
				return false;
			}
		}
		else if(stat(_socketPath.c_str(), &sb) == 0) return false;

		_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if(_serverFileDescriptor->descriptor == -1)
		{
			_out.printCritical("Critical: Couldn't create socket: " + _socketPath + ". The script engine won't work. Error: " + strerror(errno));
			return false;
		}
		int32_t reuseAddress = 1;
		if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseAddress, sizeof(int32_t)) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Critical: Couldn't set socket options: " + _socketPath + ". The script engine won't work correctly. Error: " + strerror(errno));
			return false;
		}
		sockaddr_un serverAddress;
		serverAddress.sun_family = AF_LOCAL;
		//104 is the size on BSD systems - slightly smaller than in Linux
		if(_socketPath.length() > 104)
		{
			//Check for buffer overflow
			_out.printCritical("Critical: Socket path is too long.");
			return false;
		}
		strncpy(serverAddress.sun_path, _socketPath.c_str(), 104);
		serverAddress.sun_path[103] = 0; //Just to make sure the string is null terminated.
		bool bound = (bind(_serverFileDescriptor->descriptor, (sockaddr*)&serverAddress, strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Critical: Script engine server could not start listening. Error: " + std::string(strerror(errno)));
		}
		if(chmod(_socketPath.c_str(), S_IRWXU | S_IRWXG) == -1)
		{
			_out.printError("Error: chmod failed on unix socket \"" + _socketPath + "\".");
		}
		return true;
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
    return false;
}

bool ScriptEngineServer::checkSessionId(const std::string& sessionId)
{
	try
	{
		bool result = false;
		std::lock_guard<std::mutex> checkSessionIdGuard(_checkSessionIdMutex);
		GD::bl->threadManager.join(_checkSessionIdThread);
		_checkSessionIdThread = std::thread();
		GD::bl->threadManager.start(_checkSessionIdThread, false, &ScriptEngineServer::checkSessionIdThread, this, sessionId, &result);
		GD::bl->threadManager.join(_checkSessionIdThread);

		return result;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

void ScriptEngineServer::checkSessionIdThread(std::string sessionId, bool* result)
{
	*result = false;
	std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()
	zend_homegear_globals* globals = php_homegear_get_globals();
	if(!globals) return;
	try
	{
		if(!tsrm_get_ls_cache() || !((sapi_globals_struct *) (*((void ***) tsrm_get_ls_cache()))[((sapi_globals_id)-1)]))
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists.");
			return;
		}
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise POST data is not processed.
		SG(sapi_headers).http_response_code = 200;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;
		SG(request_info).content_length = 0;
		SG(request_info).request_method = "GET";
		SG(request_info).proto_num = 1001;
		globals->http.getHeader().cookie = "PHPSESSID=" + sessionId;

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			ts_free_thread();
			return;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		ts_free_thread();
		return;
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		ts_free_thread();
		return;
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		ts_free_thread();
		return;
	}

	try
	{
		zval returnValue;
		zval function;

		ZVAL_STRINGL(&function, "session_start", sizeof("session_start") - 1);
		call_user_function(EG(function_table), NULL, &function, &returnValue, 0, nullptr);

		zval* reference = zend_hash_str_find(&EG(symbol_table), "_SESSION", sizeof("_SESSION") - 1);
		if(reference != NULL)
		{
			if(Z_ISREF_P(reference) && Z_RES_P(reference)->ptr && Z_TYPE_P(Z_REFVAL_P(reference)) == IS_ARRAY)
			{
				zval* token = zend_hash_str_find(Z_ARRVAL_P(Z_REFVAL_P(reference)), "authorized", sizeof("authorized") - 1);
				if(token != NULL)
				{
					*result = (Z_TYPE_P(token) == IS_TRUE);
				}
			}
        }
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	php_request_shutdown(NULL);
	ts_free_thread();
}

void ScriptEngineServer::executeScript(PScriptInfo& scriptInfo, bool wait)
{
	try
	{
		if(_shuttingDown) return;
		PScriptEngineProcess process = getFreeProcess();
		if(!process)
		{
			_out.printError("Error: Could not get free process. Not executing script.");
			if(scriptInfo->returnOutput) scriptInfo->output.append("Error: Could not get free process. Not executing script.\n");
			scriptInfo->exitCode = -1;
			if(scriptInfo->scriptFinishedCallback) scriptInfo->scriptFinishedCallback(scriptInfo, -1);
			return;
		}

		{
			std::lock_guard<std::mutex> processGuard(_currentScriptIdMutex);
			while(scriptInfo->id == 0) scriptInfo->id = _currentScriptId++;
		}

		_out.printInfo("Info: Starting script \"" + scriptInfo->path + "\" with id " + std::to_string(scriptInfo->id) + ".");
		process->registerScript(scriptInfo->id, scriptInfo);

		PScriptEngineClientData clientData = process->getClientData();
		PScriptFinishedInfo scriptFinishedInfo;
		std::unique_lock<std::mutex> scriptFinishedLock;
		if(wait)
		{
			scriptFinishedInfo = process->getScriptFinishedInfo(scriptInfo->id);
			scriptFinishedLock = std::move(std::unique_lock<std::mutex>(scriptFinishedInfo->mutex));
		}

		BaseLib::PArray parameters;
		ScriptInfo::ScriptType scriptType = scriptInfo->getType();
		if(scriptType == ScriptInfo::ScriptType::cli)
		{
			parameters = std::move(BaseLib::PArray(new BaseLib::Array{
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->id)),
				BaseLib::PVariable(new BaseLib::Variable((int32_t)scriptInfo->getType())),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->path)),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->script)),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->arguments)),
				BaseLib::PVariable(new BaseLib::Variable((bool)scriptInfo->scriptOutputCallback || scriptInfo->returnOutput))}));
		}
		else if(scriptType == ScriptInfo::ScriptType::web)
		{
			parameters = std::move(BaseLib::PArray(new BaseLib::Array{
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->id)),
				BaseLib::PVariable(new BaseLib::Variable((int32_t)scriptInfo->getType())),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->path)),
				scriptInfo->http.serialize(),
				scriptInfo->serverInfo->serialize()}));
		}
		else if(scriptType == ScriptInfo::ScriptType::device)
		{
			parameters = std::move(BaseLib::PArray(new BaseLib::Array{
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->id)),
				BaseLib::PVariable(new BaseLib::Variable((int32_t)scriptInfo->getType())),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->path)),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->script)),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->arguments)),
				BaseLib::PVariable(new BaseLib::Variable(scriptInfo->peerId))}));
		}

		BaseLib::PVariable result = sendRequest(clientData, "executeScript", parameters);
		if(result->errorStruct)
		{
			_out.printError("Error: Could not execute script: " + result->structValue->at("faultString")->stringValue);
			if(scriptInfo->returnOutput) scriptInfo->output.append("Error: Could not execute script: " + result->structValue->at("faultString")->stringValue + '\n');
			process->invokeScriptFinished(scriptInfo->id, result->structValue->at("faultCode")->integerValue);
			process->unregisterScript(scriptInfo->id);
			return;
		}
		scriptInfo->started = true;
		if(wait)
		{
			while(!scriptFinishedInfo->conditionVariable.wait_for(scriptFinishedLock, std::chrono::milliseconds(10000), [&]{ return scriptFinishedInfo->finished || clientData->closed || _stopServer; }));
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BaseLib::PVariable ScriptEngineServer::getAllScripts()
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));

		std::vector<std::string> scripts = GD::bl->io.getFiles(GD::bl->settings.scriptPath(), true);
		for(std::vector<std::string>::iterator i = scripts.begin(); i != scripts.end(); ++i)
		{
			array->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(*i)));
		}

		return array;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

// {{{ RPC methods
BaseLib::PVariable ScriptEngineServer::registerScriptEngineClient(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		pid_t pid = parameters->at(0)->integerValue;
		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
		if(processIterator == _processes.end())
		{
			_out.printError("Error: Cannot register client. No process with pid " + std::to_string(pid) + " found.");
			return BaseLib::Variable::createError(-1, "No matching process found.");
		}
		if(processIterator->second->getClientData())
		{
			_out.printError("Error: Cannot register client, because it already is registered.");
			return BaseLib::PVariable(new BaseLib::Variable());
		}
		clientData->pid = pid;
		processIterator->second->setClientData(clientData);
		processIterator->second->requestConditionVariable.notify_one();
		return BaseLib::PVariable(new BaseLib::Variable());
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptFinished(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		int32_t exitCode = parameters->at(0)->integerValue;
		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
		if(processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
		processIterator->second->invokeScriptFinished(scriptId, exitCode);
		processIterator->second->unregisterScript(scriptId);
		return BaseLib::PVariable(new BaseLib::Variable());
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptHeaders(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
		if(processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
		processIterator->second->invokeScriptHeaders(scriptId, parameters->at(0)->stringValue);
		return BaseLib::PVariable(new BaseLib::Variable());
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptOutput(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
		if(processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
		processIterator->second->invokeScriptOutput(scriptId, parameters->at(0)->stringValue);
		return BaseLib::PVariable(new BaseLib::Variable());
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::peerExists(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
		if(parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter is not of type integer.");

		return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->peerExists(parameters->at(0)->integerValue)));
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::listRpcClients(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

		BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
		for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
		{
			const std::vector<BaseLib::PRpcClientInfo> clients = i->second.getClientInfo();
			for(std::vector<BaseLib::PRpcClientInfo>::const_iterator j = clients.begin(); j != clients.end(); ++j)
			{
				BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

				element->structValue->insert(BaseLib::StructElement("CLIENT_ID", BaseLib::PVariable(new BaseLib::Variable((*j)->id))));
				element->structValue->insert(BaseLib::StructElement("IP_ADDRESS", BaseLib::PVariable(new BaseLib::Variable((*j)->address))));
				element->structValue->insert(BaseLib::StructElement("INIT_URL", BaseLib::PVariable(new BaseLib::Variable((*j)->initUrl))));
				element->structValue->insert(BaseLib::StructElement("INIT_INTERFACE_ID", BaseLib::PVariable(new BaseLib::Variable((*j)->initInterfaceId))));
				element->structValue->insert(BaseLib::StructElement("XML_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->xmlRpc))));
				element->structValue->insert(BaseLib::StructElement("BINARY_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->binaryRpc))));
				element->structValue->insert(BaseLib::StructElement("JSON_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->jsonRpc))));
				element->structValue->insert(BaseLib::StructElement("WEBSOCKET", BaseLib::PVariable(new BaseLib::Variable((*j)->webSocket))));
				element->structValue->insert(BaseLib::StructElement("INIT_KEEP_ALIVE", BaseLib::PVariable(new BaseLib::Variable((*j)->initKeepAlive))));
				element->structValue->insert(BaseLib::StructElement("INIT_BINARY_MODE", BaseLib::PVariable(new BaseLib::Variable((*j)->initBinaryMode))));
				element->structValue->insert(BaseLib::StructElement("INIT_NEW_FORMAT", BaseLib::PVariable(new BaseLib::Variable((*j)->initNewFormat))));
				element->structValue->insert(BaseLib::StructElement("INIT_SUBSCRIBE_PEERS", BaseLib::PVariable(new BaseLib::Variable((*j)->initSubscribePeers))));
				element->structValue->insert(BaseLib::StructElement("INIT_JSON_MODE", BaseLib::PVariable(new BaseLib::Variable((*j)->initJsonMode))));

				result->arrayValue->push_back(element);
			}
		}

		return result;
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

	// {{{ User methods
		BaseLib::PVariable ScriptEngineServer::auth(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameters are not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
				if(parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(User::verify(parameters->at(0)->stringValue, parameters->at(1)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::createUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameters are not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
				if(parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is empty.");
				if(!BaseLib::HelperFunctions::isAlphaNumeric(parameters->at(0)->stringValue)) return BaseLib::Variable::createError(-1, "Parameter 1 is not alphanumeric.");

				return BaseLib::PVariable(new BaseLib::Variable(User::create(parameters->at(0)->stringValue, parameters->at(1)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::deleteUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(User::remove(parameters->at(0)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::updateUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameters are not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
				if(parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(User::update(parameters->at(0)->stringValue, parameters->at(1)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::userExists(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(User::exists(parameters->at(0)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::listUsers(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

				std::map<uint64_t, std::string> users;
				User::getAll(users);

				BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
				result->arrayValue->reserve(users.size());
				for(std::map<uint64_t, std::string>::iterator i = users.begin(); i != users.end(); ++i)
				{
					result->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(i->second)));
				}

				return result;
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}
	// }}}

	// {{{ Module methods
		BaseLib::PVariable ScriptEngineServer::listModules(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

				std::vector<std::shared_ptr<FamilyController::ModuleInfo>> moduleInfo = GD::familyController->getModuleInfo();

				BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
				result->arrayValue->reserve(moduleInfo.size());
				for(std::vector<std::shared_ptr<FamilyController::ModuleInfo>>::iterator i = moduleInfo.begin(); i != moduleInfo.end(); ++i)
				{
					BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

					element->structValue->insert(BaseLib::StructElement("FILENAME", BaseLib::PVariable(new BaseLib::Variable((*i)->filename))));
					element->structValue->insert(BaseLib::StructElement("FAMILY_ID", BaseLib::PVariable(new BaseLib::Variable((*i)->familyId))));
					element->structValue->insert(BaseLib::StructElement("FAMILY_NAME", BaseLib::PVariable(new BaseLib::Variable((*i)->familyName))));
					element->structValue->insert(BaseLib::StructElement("BASELIB_VERSION", BaseLib::PVariable(new BaseLib::Variable((*i)->baselibVersion))));
					element->structValue->insert(BaseLib::StructElement("LOADED", BaseLib::PVariable(new BaseLib::Variable((*i)->loaded))));

					result->arrayValue->push_back(element);
				}

				return result;
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::loadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->loadModule(parameters->at(0)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::unloadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->unloadModule(parameters->at(0)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::reloadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
				if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

				return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->reloadModule(parameters->at(0)->stringValue)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}
	// }}}

	// {{{ Licensing methods
		BaseLib::PVariable ScriptEngineServer::checkLicense(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 3 && parameters->size() != 4) return BaseLib::Variable::createError(-1, "Method expects three or four parameters. " + std::to_string(parameters->size()) + " given.");
				if(parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
				if(parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
				if(parameters->at(2)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type integer.");
				if(parameters->size() == 4 && parameters->at(3)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 4 is not of type string.");

				int32_t moduleId = parameters->at(0)->integerValue;
				int32_t familyId = parameters->at(1)->integerValue;
				int32_t deviceId = parameters->at(2)->integerValue;
				std::string licenseKey = (parameters->size() == 4) ? parameters->at(3)->stringValue : "";

				std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
				if(i == GD::licensingModules.end() || !i->second)
				{
					_out.printError("Error: Could not check license. License module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
					return BaseLib::PVariable(new BaseLib::Variable(-2));
				}

				return BaseLib::PVariable(new BaseLib::Variable(i->second->checkLicense(familyId, deviceId, licenseKey)));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::removeLicense(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects exactly three parameters.");
				if(parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
				if(parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
				if(parameters->at(2)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type integer.");

				int32_t moduleId = parameters->at(0)->integerValue;
				int32_t familyId = parameters->at(1)->integerValue;
				int32_t deviceId = parameters->at(2)->integerValue;

				std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
				if(i == GD::licensingModules.end() || !i->second)
				{
					_out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
					return BaseLib::PVariable(new BaseLib::Variable(-2));
				}

				i->second->removeLicense(familyId, deviceId);

				return BaseLib::PVariable(new BaseLib::Variable(true));
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable ScriptEngineServer::getLicenseStates(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
		{
			try
			{
				if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
				if(parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");

				int32_t moduleId = parameters->at(0)->integerValue;

				std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
				if(i == GD::licensingModules.end() || !i->second)
				{
					_out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
					return BaseLib::PVariable(new BaseLib::Variable(false));
				}

				BaseLib::Licensing::Licensing::DeviceStates states = i->second->getDeviceStates();
				BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
				result->arrayValue->reserve(states.size());
				for(BaseLib::Licensing::Licensing::DeviceStates::iterator i = states.begin(); i != states.end(); ++i)
				{
					for(std::map<int32_t, BaseLib::Licensing::Licensing::PDeviceInfo>::iterator j = i->second.begin(); j != i->second.end(); ++j)
					{
						BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

						element->structValue->insert(BaseLib::StructElement("MODULE_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->moduleId))));
						element->structValue->insert(BaseLib::StructElement("FAMILY_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->familyId))));
						element->structValue->insert(BaseLib::StructElement("DEVICE_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->deviceId))));
						element->structValue->insert(BaseLib::StructElement("ACTIVATED", BaseLib::PVariable(new BaseLib::Variable(j->second->state))));
						element->structValue->insert(BaseLib::StructElement("LICENSE_KEY", BaseLib::PVariable(new BaseLib::Variable(j->second->licenseKey))));

						result->arrayValue->push_back(element);
					}
				}

				return result;
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}
	// }}}
// }}}

}
