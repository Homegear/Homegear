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

#include "FlowsServer.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Flows
{

FlowsServer::FlowsServer() : IQueue(GD::bl.get(), 1000)
{
	_out.init(GD::bl.get());
	_out.setPrefix("Flows Engine Server: ");

	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, true));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true));
	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("devTest", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCDevTest())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("system.getCapabilities", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSystemGetCapabilities())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("system.listMethods", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSystemListMethods(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("system.methodHelp", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSystemMethodHelp(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("system.methodSignature", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSystemMethodSignature(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("system.multicall", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSystemMulticall(GD::rpcServers[0].getServer()))));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("activateLinkParamset", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCActivateLinkParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("abortEventReset", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCTriggerEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("addDevice", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCAddDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("addEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCAddEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("addLink", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCAddLink())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("copyConfig", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCCopyConfig())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("clientServerInitialized", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCClientServerInitialized())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("createDevice", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCCreateDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("deleteDevice", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCDeleteDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("deleteMetadata", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCDeleteMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("deleteSystemVariable", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCDeleteSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("enableEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCEnableEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getAllConfig", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetAllConfig())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getAllMetadata", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetAllMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getAllScripts", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetAllScripts())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getAllSystemVariables", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetAllSystemVariables())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getAllValues", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetAllValues())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getConfigParameter", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetConfigParameter())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getDeviceDescription", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetDeviceDescription())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getDeviceInfo", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetDeviceInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getInstallMode", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetInstallMode())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getKeyMismatchDevice", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetKeyMismatchDevice())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getLinkInfo", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetLinkInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getLinkPeers", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetLinkPeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getLinks", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetLinks())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getMetadata", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getName", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetName())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getPairingMethods", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetPairingMethods())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getParamset", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getParamsetDescription", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetParamsetDescription())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getParamsetId", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetParamsetId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getPeerId", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetPeerId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getServiceMessages", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetServiceMessages())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getSystemVariable", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getUpdateStatus", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetUpdateStatus())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getValue", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetValue())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("getVersion", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCGetVersion())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("init", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCInit())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listBidcosInterfaces", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListBidcosInterfaces())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listClientServers", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListClientServers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listDevices", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListDevices())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listEvents", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListEvents())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listFamilies", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListFamilies())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listInterfaces", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListInterfaces())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listKnownDeviceTypes", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListKnownDeviceTypes())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("listTeams", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCListTeams())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("logLevel", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCLogLevel())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("ping", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCPing())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("putParamset", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCPutParamset())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("removeEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCRemoveEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("removeLink", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCRemoveLink())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("reportValueUsage", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCReportValueUsage())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("rssiInfo", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCRssiInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("runScript", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCRunScript())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("searchDevices", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSearchDevices())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setId", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetId())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setInstallMode", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetInstallMode())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setInterface", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetInterface())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setLinkInfo", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetLinkInfo())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setMetadata", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetMetadata())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setName", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetName())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setSystemVariable", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetSystemVariable())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setTeam", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetTeam())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("setValue", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSetValue())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("subscribePeers", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCSubscribePeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("triggerEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCTriggerEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("triggerRpcEvent", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCTriggerRpcEvent())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("unsubscribePeers", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCUnsubscribePeers())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("updateFirmware", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCUpdateFirmware())));
	_rpcMethods.insert(std::pair<std::string, std::shared_ptr<Rpc::RPCMethod>>("writeLog", std::shared_ptr<Rpc::RPCMethod>(new Rpc::RPCWriteLog())));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("registerFlowsClient", std::bind(&FlowsServer::registerFlowsClient, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("flowFinished", std::bind(&FlowsServer::flowFinished, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
}

FlowsServer::~FlowsServer()
{
	if(!_stopServer) stop();
	GD::bl->threadManager.join(_checkSessionIdThread);
}

void FlowsServer::collectGarbage()
{
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<PFlowsProcess> processesToShutdown;
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			bool emptyProcess = false;
			for(std::map<pid_t, PFlowsProcess>::iterator i = _processes.begin(); i != _processes.end(); ++i)
			{
				if(i->second->flowCount() == 0 && i->second->getClientData() && !i->second->getClientData()->closed)
				{
					if(emptyProcess) closeClientConnection(i->second->getClientData());
					else emptyProcess = true;
				}
			}
		}

		std::vector<PFlowsClientData> clientsToRemove;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			try
			{
				for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
		for(std::vector<PFlowsClientData>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				_clients.erase((*i)->id);
			}
			_out.printDebug("Debug: Flows engine client " + std::to_string((*i)->id) + " removed.");
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

bool FlowsServer::start()
{
	try
	{
		stop();
		_socketPath = GD::bl->settings.socketPath() + "homegearFE.sock";
		_shuttingDown = false;
		_stopServer = false;
		if(!getFileDescriptor(true)) return false;
		startQueue(0, GD::bl->settings.flowsProcessingThreadCount(), 0, SCHED_OTHER);
		GD::bl->threadManager.start(_mainThread, true, &FlowsServer::mainThread, this);
		startFlows();
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

void FlowsServer::stop()
{
	try
	{
		_shuttingDown = true;
		_out.printDebug("Debug: Waiting for flows engine server's client threads to finish.");
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				clients.push_back(i->second);
				closeClientConnection(i->second);
				std::lock_guard<std::mutex> processGuard(_processMutex);
				std::map<pid_t, PFlowsProcess>::iterator processIterator = _processes.find(i->second->pid);
				if(processIterator != _processes.end())
				{
					processIterator->second->invokeFlowFinished(-32501);
					_processes.erase(processIterator);
				}
			}
		}

		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		while(_clients.size() > 0)
		{
			for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}
		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::homegearReloading()
{
	try
	{
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}
		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped)
{
	try
	{
		std::shared_ptr<FlowsProcess> process;

		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			std::map<pid_t, PFlowsProcess>::iterator processIterator = _processes.find(pid);
			if(processIterator != _processes.end())
			{
				process = processIterator->second;
				closeClientConnection(process->getClientData());
				_processes.erase(processIterator);
			}
		}

		if(process)
		{
			if(signal != -1) _out.printCritical("Critical: Client process with pid " + std::to_string(pid) + " was killed with signal " + std::to_string(signal) + '.');
			else if(exitCode != 0) _out.printError("Error: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');
			else _out.printInfo("Info: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');

			if(signal != -1) exitCode = -32500;
			process->invokeFlowFinished(exitCode);
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

void FlowsServer::startFlows()
{
	try
	{
		std::vector<std::string> flowFiles = GD::bl->io.getFiles(GD::bl->settings.flowsPath(), true);
		for(std::vector<std::string>::iterator i = flowFiles.begin(); i != flowFiles.end(); ++i)
		{
			PFlowInfoServer flowInfo(new FlowInfoServer());
			{
				std::lock_guard<std::mutex> flowIdGuard(_currentFlowIdMutex);
				while(flowInfo->id == 0) flowInfo->id = _currentFlowId++;
			}
			flowInfo->fullPath = GD::bl->settings.flowsPath() + *i;
			executeFlow(flowInfo);
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

uint32_t FlowsServer::flowCount()
{
	try
	{
		if(_shuttingDown) return 0;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		uint32_t count = 0;
		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array());
			BaseLib::PVariable response = sendRequest(*i, "flowCount", parameters);
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

void FlowsServer::broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, BaseLib::PArray values)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::broadcastNewDevices(BaseLib::PVariable deviceDescriptions)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::broadcastDeleteDevices(BaseLib::PVariable deviceInfo)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint)
{
	try
	{
		if(_shuttingDown) return;
		std::vector<PFlowsClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PFlowsClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
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

void FlowsServer::closeClientConnection(PFlowsClientData client)
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

void FlowsServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
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

			std::map<std::string, std::function<BaseLib::PVariable(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
			if(localMethodIterator != _localRpcMethods.end())
			{
				if(GD::bl->debugLevel >= 4)
				{
					_out.printDebug("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName);
					if(GD::bl->debugLevel >= 5)
					{
						for(BaseLib::Array::iterator i = parameters->at(2)->arrayValue->begin(); i != parameters->at(2)->arrayValue->end(); ++i)
						{
							(*i)->print(true, false);
						}
					}
				}
				BaseLib::PVariable result = localMethodIterator->second(queueEntry->clientData, parameters->at(0)->integerValue, parameters->at(2)->arrayValue);
				if(GD::bl->debugLevel >= 5)
				{
					_out.printDebug("Response: ");
					result->print(true, false);
				}

				sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
				return;
			}

			std::map<std::string, std::shared_ptr<Rpc::RPCMethod>>::iterator methodIterator = _rpcMethods.find(methodName);
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
					(*i)->print(true, false);
				}
			}
			BaseLib::PVariable result = _rpcMethods.at(methodName)->invoke(_dummyClientInfo, parameters->at(2)->arrayValue);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print(true, false);
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

BaseLib::PVariable FlowsServer::send(PFlowsClientData& clientData, std::vector<char>& data)
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
				if(errno == EAGAIN) continue;
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

BaseLib::PVariable FlowsServer::sendRequest(PFlowsClientData& clientData, std::string methodName, BaseLib::PArray& parameters)
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
			return BaseLib::Variable::createError(-1, "No response received.");
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

void FlowsServer::sendResponse(PFlowsClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable)
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

void FlowsServer::mainThread()
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
				for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
				PFlowsClientData clientData = PFlowsClientData(new FlowsClientData(clientFileDescriptor));
				clientData->id = _currentClientId++;
				_clients[clientData->id] = clientData;
				continue;
			}

			PFlowsClientData clientData;
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				for(std::map<int32_t, PFlowsClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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

PFlowsProcess FlowsServer::getFreeProcess()
{
	try
	{
		std::lock_guard<std::mutex> processGuard(_newProcessMutex);
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			for(std::map<pid_t, PFlowsProcess>::iterator i = _processes.begin(); i != _processes.end(); ++i)
			{
				if(i->second->flowCount() < GD::bl->threadManager.getMaxThreadCount() / 4 && (GD::bl->settings.maxFlowsPerProcess() == -1 || i->second->flowCount() < (unsigned)GD::bl->settings.maxFlowsPerProcess()))
				{
					return i->second;
				}
			}
		}
		_out.printInfo("Info: Spawning new flows process.");
		PFlowsProcess process(new FlowsProcess());
		std::vector<std::string> arguments{ "-c", GD::configPath, "-rl" };
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
				_out.printError("Error: Could not start new flows process.");
				return PFlowsProcess();
			}
			_out.printInfo("Info: Flows process successfully spawned. Process id is " + std::to_string(process->getPid()) + ". Client id is: " + std::to_string(process->getClientData()->id) + ".");
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
    return PFlowsProcess();
}

void FlowsServer::readClient(PFlowsClientData& clientData)
{
	try
	{
		int32_t processedBytes = 0;
		int32_t bytesRead = 0;
		bytesRead = read(clientData->fileDescriptor->descriptor, &(clientData->buffer[0]), clientData->buffer.size());
		if(bytesRead <= 0) //read returns 0, when connection is disrupted.
		{
			_out.printInfo("Info: Connection to flows server's client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
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

bool FlowsServer::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		if(!BaseLib::Io::directoryExists(GD::bl->settings.socketPath().c_str()))
		{
			if(errno == ENOENT) _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise flows won't work.");
			else _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath() + ". The script engine won't work: " + strerror(errno));
			_stopServer = true;
			return false;
		}
		bool isDirectory = false;
		int32_t result = BaseLib::Io::isDirectory(GD::bl->settings.socketPath(), isDirectory);
		if(result != 0 || !isDirectory)
		{
			_out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise flows won't work.");
			_stopServer = true;
			return false;
		}
		if(deleteOldSocket)
		{
			if(unlink(_socketPath.c_str()) == -1 && errno != ENOENT)
			{
				_out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath + ". Please delete it manually. Flows won't work. Error: " + strerror(errno));
				return false;
			}
		}
		else if(BaseLib::Io::fileExists(_socketPath)) return false;

		_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if(_serverFileDescriptor->descriptor == -1)
		{
			_out.printCritical("Critical: Couldn't create socket: " + _socketPath + ". Flows won't work. Error: " + strerror(errno));
			return false;
		}
		int32_t reuseAddress = 1;
		if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseAddress, sizeof(int32_t)) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Critical: Couldn't set socket options: " + _socketPath + ". Flows won't work correctly. Error: " + strerror(errno));
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
			_out.printCritical("Critical: Flows server could not start listening. Error: " + std::string(strerror(errno)));
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

void FlowsServer::executeFlow(PFlowInfoServer& flowInfo)
{
	try
	{
		if(_shuttingDown) return;
		PFlowsProcess process = getFreeProcess();
		if(!process)
		{
			_out.printError("Error: Could not get free process. Not executing flow.");
			flowInfo->exitCode = -1;
			if(flowInfo->flowFinishedCallback) flowInfo->flowFinishedCallback(flowInfo, -1);
			return;
		}

		{
			std::lock_guard<std::mutex> processGuard(_currentFlowIdMutex);
			while(flowInfo->id == 0) flowInfo->id = _currentFlowId++;
		}

		_out.printInfo("Info: Starting flow \"" + flowInfo->fullPath + "\" with id " + std::to_string(flowInfo->id) + ".");
		process->registerFlow(flowInfo->id, flowInfo);

		PFlowsClientData clientData = process->getClientData();

		BaseLib::PArray parameters(new BaseLib::Array{
				BaseLib::PVariable(new BaseLib::Variable(flowInfo->id)),
				BaseLib::PVariable(new BaseLib::Variable(flowInfo->fullPath))});

		BaseLib::PVariable result = sendRequest(clientData, "executeFlow", parameters);
		if(result->errorStruct)
		{
			_out.printError("Error: Could not execute flow: " + result->structValue->at("faultString")->stringValue);
			process->invokeFlowFinished(flowInfo->id, result->structValue->at("faultCode")->integerValue);
			process->unregisterFlow(flowInfo->id);
			return;
		}
		flowInfo->started = true;
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

// {{{ RPC methods
BaseLib::PVariable FlowsServer::registerFlowsClient(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		pid_t pid = parameters->at(0)->integerValue;
		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, PFlowsProcess>::iterator processIterator = _processes.find(pid);
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

BaseLib::PVariable FlowsServer::flowFinished(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)
{
	try
	{
		if(parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		int32_t exitCode = parameters->at(0)->integerValue;
		std::lock_guard<std::mutex> processGuard(_processMutex);
		std::map<pid_t, PFlowsProcess>::iterator processIterator = _processes.find(clientData->pid);
		if(processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
		processIterator->second->invokeFlowFinished(scriptId, exitCode);
		processIterator->second->unregisterFlow(scriptId);
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
// }}}

}
