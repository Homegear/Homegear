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

int32_t ScriptEngineServer::_currentClientID = 0;

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
}

ScriptEngineServer::~ScriptEngineServer()
{
	stop();
}

void ScriptEngineServer::collectGarbage()
{
	_garbageCollectionMutex.lock();
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<std::shared_ptr<ClientData>> clientsToRemove;
		_stateMutex.lock();
		try
		{
			for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
		_stateMutex.unlock();
		for(std::vector<std::shared_ptr<ClientData>>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			_out.printDebug("Debug: Joining read thread of script engine client " + std::to_string((*i)->id));
			_stateMutex.lock();
			try
			{
				if(i->use_count() <= 2) _clients.erase((*i)->id);
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_stateMutex.unlock();
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
    _garbageCollectionMutex.unlock();
}

bool ScriptEngineServer::start()
{
	try
	{
		stop();
		_socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
		_stopServer = false;
		if(!getFileDescriptor(true)) return false;
		startQueue(0, GD::bl->settings.scriptEngineThreadCount(), -1, SCHED_OTHER);
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
		stopQueue(0);
		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		_out.printDebug("Debug: Waiting for script engine server's client threads to finish.");
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				closeClientConnection(i->second);
			}
		}
		while(_clients.size() > 0)
		{
			collectGarbage();
			if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
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

void ScriptEngineServer::processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped)
{
	try
	{
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			std::map<int32_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
			if(processIterator != _processes.end())
			{
				closeClientConnection(processIterator->second->clientData);
				_processes.erase(processIterator);
			}
		}

		std::cerr << "Process killed: " << (uint32_t)pid << ' ' << exitCode << ' ' << signal << ' ' << (int32_t)coreDumped << std::endl;
		//Todo: Call process killed event handlers.
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

void ScriptEngineServer::closeClientConnection(std::shared_ptr<ClientData> client)
{
	try
	{
		if(!client) return;
		GD::bl->fileDescriptorManager.close(client->fileDescriptor);
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
		if(!queueEntry) return;

		if(queueEntry->isRequest)
		{
			std::string methodName;
			BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

			if(methodName == "registerScriptEngineClient" && parameters->size() > 0)
			{
				{
					pid_t pid = parameters->at(0)->integerValue;
					std::lock_guard<std::mutex> processGuard(_processMutex);
					std::map<int32_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
					if(processIterator == _processes.end())
					{
						_out.printError("Error: Cannot register client. No process with pid " + std::to_string(pid) + " found.");
						BaseLib::PVariable result = BaseLib::Variable::createError(-1, "No matching process found.");
						sendResponse(queueEntry->clientData, result);
						return;
					}
					processIterator->second->clientData = queueEntry->clientData;
					processIterator->second->requestConditionVariable.notify_one();
				}

				BaseLib::PVariable result(new BaseLib::Variable());
				sendResponse(queueEntry->clientData, result);
				return;
			}

			std::map<std::string, std::shared_ptr<RPC::RPCMethod>>::iterator methodIterator = _rpcMethods.find(methodName);
			if(methodIterator == _rpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + methodName);
				BaseLib::PVariable result = BaseLib::Variable::createError(-32601, ": Requested method not found.");
				sendResponse(queueEntry->clientData, result);
				return;
			}
			if(GD::bl->debugLevel >= 4)
			{
				_out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName + " Parameters:");
				for(std::vector<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i)
				{
					(*i)->print();
				}
			}
			BaseLib::PVariable result = _rpcMethods.at(methodName)->invoke(_dummyClientInfo, parameters);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print();
			}

			sendResponse(queueEntry->clientData, result);
		}
		else
		{
			queueEntry->clientData->rpcResponse = _rpcDecoder->decodeResponse(queueEntry->packet);
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

BaseLib::PVariable ScriptEngineServer::sendRequest(std::shared_ptr<ClientData>& clientData, std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	try
	{
		std::unique_lock<std::mutex> requestLock(clientData->requestMutex);
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, parameters, data);

		for(int32_t i = 0; i < 5; i++)
		{
			clientData->rpcResponse.reset();
			int32_t totallySentBytes = 0;
			while (totallySentBytes < (signed)data.size())
			{
				int32_t sentBytes = ::send(clientData->fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
				if(sentBytes == -1)
				{
					GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
					break;
				}
				totallySentBytes += sentBytes;
			}
			clientData->requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(5000), [&]{ return clientData->rpcResponse || _stopServer; });
			if(clientData->rpcResponse) return clientData->rpcResponse;
			else if(_stopServer) return BaseLib::Variable::createError(-32500, "Server is being stopped.");
			else if(i == 4) _out.printError("Error: No response received to RPC request. Method: " + methodName);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineServer::sendResponse(std::shared_ptr<ClientData>& clientData, BaseLib::PVariable& variable)
{
	try
	{
		std::vector<char> data;
		_rpcEncoder->encodeResponse(variable, data);
		int32_t totallySentBytes = 0;
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sentBytes = ::send(clientData->fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
			if(sentBytes == -1)
			{
				GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
				break;
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
}

void ScriptEngineServer::mainThread()
{
	try
	{
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
				for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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

			if(!select(maxfd + 1, &readFileDescriptor, NULL, NULL, &timeout))
			{
				if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.scriptEngineServerMaxConnections() * 100 / 112) collectGarbage();
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
				std::shared_ptr<ClientData> clientData = std::shared_ptr<ClientData>(new ClientData(clientFileDescriptor));
				clientData->id = _currentClientID++;
				_clients[clientData->id] = clientData;
				continue;
			}

			std::shared_ptr<ClientData> clientData;
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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

std::shared_ptr<ScriptEngineServer::ScriptEngineProcess> ScriptEngineServer::getFreeProcess()
{
	try
	{
		std::lock_guard<std::mutex> processGuard(_newProcessMutex);
		{
			std::lock_guard<std::mutex> processGuard(_processMutex);
			for(std::map<int32_t, std::shared_ptr<ScriptEngineProcess>>::iterator i = _processes.begin(); i != _processes.end(); ++i)
			{
				if(i->second->scriptCount < GD::bl->threadManager.getMaxThreadCount() / GD::bl->settings.scriptEngineMaxThreadsPerScript() && (GD::bl->settings.scriptEngineMaxScriptsPerProcess() == -1 || i->second->scriptCount < (unsigned)GD::bl->settings.scriptEngineMaxScriptsPerProcess()))
				{
					i->second->scriptCount++;
					return i->second;
				}
			}
		}
		std::shared_ptr<ScriptEngineProcess> process(new ScriptEngineProcess());
		std::vector<std::string> arguments{ "-sre" };
		process->pid = GD::bl->hf.system(GD::executablePath + "/" + GD::executableFile, arguments);
		if(process->pid != -1)
		{
			process->scriptCount++;
			{
				std::lock_guard<std::mutex> processGuard(_processMutex);
				_processes[process->pid] = process;
			}

			std::mutex requestMutex;
			std::unique_lock<std::mutex> requestLock(requestMutex);
			process->requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(5000), [&]{ return (bool)(process->clientData); });

			if(!process->clientData)
			{
				std::lock_guard<std::mutex> processGuard(_processMutex);
				_processes.erase(process->pid);
				return std::shared_ptr<ScriptEngineProcess>();
			}
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

void ScriptEngineServer::readClient(std::shared_ptr<ClientData>& clientData)
{
	try
	{
		int32_t bytesRead = read(clientData->fileDescriptor->descriptor, &(clientData->buffer[0]), clientData->buffer.size() - 1);
		if(bytesRead <= 0)
		{
			_out.printInfo("Info: Connection to script server's client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
			closeClientConnection(clientData);
			return;
		}

		if(bytesRead < 8) return;
		if(bytesRead > (signed)clientData->buffer.size() - 1) bytesRead = clientData->buffer.size() - 1;
		clientData->buffer.at(bytesRead) = 0;

		if(clientData->receivedDataLength == 0 && !strncmp(&(clientData->buffer[0]), "Bin", 3))
		{
			clientData->packetIsRequest = !((clientData->buffer[3]) & 1);
			GD::bl->hf.memcpyBigEndian((char*)&(clientData->packetLength), &(clientData->buffer[4]), 4);
			if(clientData->packetLength == 0) return;
			if(clientData->packetLength > 10485760)
			{
				_out.printError("Error: Packet with data larger than 10 MiB received.");
				return;
			}
			clientData->packet.clear();
			clientData->packet.reserve(clientData->packetLength + 9);
			clientData->packet.insert(clientData->packet.end(), &(clientData->buffer[0]), &(clientData->buffer[0]) + bytesRead);
			if(clientData->packetLength > (unsigned)bytesRead - 8) clientData->receivedDataLength = bytesRead - 8;
			else
			{
				clientData->receivedDataLength = 0;
				std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(clientData, clientData->packet, clientData->packetIsRequest));
				enqueue(0, queueEntry);
			}
		}
		else if(clientData->receivedDataLength > 0)
		{
			if(clientData->receivedDataLength + bytesRead > clientData->packetLength)
			{
				_out.printError("Error: Packet length is wrong.");
				clientData->receivedDataLength = 0;
				return;
			}
			clientData->packet.insert(clientData->packet.end(), &(clientData->buffer[0]), &(clientData->buffer[0]) + bytesRead);
			clientData->receivedDataLength += bytesRead;
			if(clientData->receivedDataLength == clientData->packetLength)
			{
				clientData->packet.push_back('\0');
				clientData->packetLength = 0;
				std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(clientData, clientData->packet, clientData->packetIsRequest));
				enqueue(0, queueEntry);
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

void ScriptEngineServer::executeScript(BaseLib::ScriptEngine::PScriptInfo scriptInfo)
{
	try
	{
		std::shared_ptr<ScriptEngineServer::ScriptEngineProcess> process = getFreeProcess();
		if(!process)
		{
			_out.printError("Error: Could not get free process. Not executing script.");
			return;
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
