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

#include "IpcServer.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Ipc
{

IpcServer::IpcServer() : IQueue(GD::bl.get(), 3, 100000)
{
	_out.init(GD::bl.get());
	_out.setPrefix("IPC Server: ");

	_shuttingDown = false;
	_stopServer = false;

	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true, true));
	_dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
	_dummyClientInfo->ipcServer = true;
	_dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
	std::vector<uint64_t> groups{ 3 };
	_dummyClientInfo->acls->fromGroups(groups);
	_dummyClientInfo->user = "SYSTEM (3)";

	_rpcMethods.emplace("devTest", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDevTest()));
	_rpcMethods.emplace("system.getCapabilities", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemGetCapabilities()));
	_rpcMethods.emplace("system.listMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemListMethods(GD::rpcServers[0].getServer())));
	_rpcMethods.emplace("system.methodHelp", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodHelp(GD::rpcServers[0].getServer())));
	_rpcMethods.emplace("system.methodSignature", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodSignature(GD::rpcServers[0].getServer())));
	_rpcMethods.emplace("system.multicall", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMulticall(GD::rpcServers[0].getServer())));
	_rpcMethods.emplace("activateLinkParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCActivateLinkParamset()));
	_rpcMethods.emplace("abortEventReset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerEvent()));
	_rpcMethods.emplace("addDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDevice()));
	_rpcMethods.emplace("addEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddEvent()));
	_rpcMethods.emplace("addLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddLink()));
	_rpcMethods.emplace("copyConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCopyConfig()));
	_rpcMethods.emplace("clientServerInitialized", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCClientServerInitialized()));
	_rpcMethods.emplace("createDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateDevice()));
	_rpcMethods.emplace("deleteData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteData()));
	_rpcMethods.emplace("deleteDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteDevice()));
	_rpcMethods.emplace("deleteMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteMetadata()));
	_rpcMethods.emplace("deleteNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteNodeData()));
	_rpcMethods.emplace("deleteSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteSystemVariable()));
	_rpcMethods.emplace("enableEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCEnableEvent()));
	_rpcMethods.emplace("executeMiscellaneousDeviceMethod", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCExecuteMiscellaneousDeviceMethod()));
	_rpcMethods.emplace("getAllConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllConfig()));
	_rpcMethods.emplace("getAllMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllMetadata()));
	_rpcMethods.emplace("getAllScripts", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllScripts()));
	_rpcMethods.emplace("getAllSystemVariables", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllSystemVariables()));
	_rpcMethods.emplace("getAllValues", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllValues()));
	_rpcMethods.emplace("getConfigParameter", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetConfigParameter()));
	_rpcMethods.emplace("getData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetData()));
	_rpcMethods.emplace("getDeviceDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceDescription()));
	_rpcMethods.emplace("getDeviceInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceInfo()));
	_rpcMethods.emplace("getEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetEvent()));
	_rpcMethods.emplace("getInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstallMode()));
	_rpcMethods.emplace("getKeyMismatchDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetKeyMismatchDevice()));
	_rpcMethods.emplace("getLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkInfo()));
	_rpcMethods.emplace("getLinkPeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkPeers()));
	_rpcMethods.emplace("getLinks", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinks()));
	_rpcMethods.emplace("getMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetMetadata()));
	_rpcMethods.emplace("getName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetName()));
	_rpcMethods.emplace("getNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetNodeData()));
	_rpcMethods.emplace("getFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetFlowData()));
	_rpcMethods.emplace("getGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetGlobalData()));
	_rpcMethods.emplace("getNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetNodeVariable()));
	_rpcMethods.emplace("getPairingInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPairingInfo()));
	_rpcMethods.emplace("getParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamset()));
	_rpcMethods.emplace("getParamsetDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetDescription()));
	_rpcMethods.emplace("getParamsetId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetId()));
	_rpcMethods.emplace("getPeerId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPeerId()));
	_rpcMethods.emplace("getServiceMessages", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetServiceMessages()));
	_rpcMethods.emplace("getSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariable()));
	_rpcMethods.emplace("getUpdateStatus", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetUpdateStatus()));
	_rpcMethods.emplace("getValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetValue()));
	_rpcMethods.emplace("getVariableDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariableDescription()));
	_rpcMethods.emplace("getVersion", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVersion()));
	_rpcMethods.emplace("init", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCInit()));
	_rpcMethods.emplace("listBidcosInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListBidcosInterfaces()));
	_rpcMethods.emplace("listClientServers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListClientServers()));
	_rpcMethods.emplace("listDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListDevices()));
	_rpcMethods.emplace("listEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListEvents()));
	_rpcMethods.emplace("listFamilies", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListFamilies()));
	_rpcMethods.emplace("listInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListInterfaces()));
	_rpcMethods.emplace("listKnownDeviceTypes", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListKnownDeviceTypes()));
	_rpcMethods.emplace("listTeams", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListTeams()));
	_rpcMethods.emplace("logLevel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCLogLevel()));
	_rpcMethods.emplace("nodeOutput", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCNodeOutput()));
	_rpcMethods.emplace("ping", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPing()));
	_rpcMethods.emplace("putParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPutParamset()));
	_rpcMethods.emplace("removeEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveEvent()));
	_rpcMethods.emplace("removeLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveLink()));
	_rpcMethods.emplace("reportValueUsage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCReportValueUsage()));
	_rpcMethods.emplace("rssiInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRssiInfo()));
	_rpcMethods.emplace("runScript", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRunScript()));
	_rpcMethods.emplace("searchDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchDevices()));
	_rpcMethods.emplace("searchInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchInterfaces()));
	_rpcMethods.emplace("setData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetData()));
	_rpcMethods.emplace("setId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetId()));
	_rpcMethods.emplace("setInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInstallMode()));
	_rpcMethods.emplace("setInterface", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInterface()));
	_rpcMethods.emplace("setLanguage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLanguage()));
	_rpcMethods.emplace("setLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLinkInfo()));
	_rpcMethods.emplace("setMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetMetadata()));
	_rpcMethods.emplace("setName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetName()));
	_rpcMethods.emplace("setNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetNodeData()));
	_rpcMethods.emplace("setFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetFlowData()));
	_rpcMethods.emplace("setGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetGlobalData()));
	_rpcMethods.emplace("setNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetNodeVariable()));
	_rpcMethods.emplace("setSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetSystemVariable()));
	_rpcMethods.emplace("setTeam", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetTeam()));
	_rpcMethods.emplace("setValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetValue()));
	_rpcMethods.emplace("subscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSubscribePeers()));
	_rpcMethods.emplace("triggerEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerEvent()));
	_rpcMethods.emplace("triggerRpcEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerRpcEvent()));
	_rpcMethods.emplace("unsubscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUnsubscribePeers()));
	_rpcMethods.emplace("updateFirmware", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateFirmware()));
	_rpcMethods.emplace("writeLog", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCWriteLog()));

	//{{{ Rooms
		_rpcMethods.emplace("addDeviceToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDeviceToRoom()));
		_rpcMethods.emplace("createRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateRoom()));
		_rpcMethods.emplace("deleteRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteRoom()));
		_rpcMethods.emplace("getDevicesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInRoom()));
		_rpcMethods.emplace("getRooms", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRooms()));
		_rpcMethods.emplace("removeDeviceFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveDeviceFromRoom()));
		_rpcMethods.emplace("updateRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateRoom()));
	//}}}

	//{{{ Categories
		_rpcMethods.emplace("addCategoryToDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToDevice()));
		_rpcMethods.emplace("createCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateCategory()));
		_rpcMethods.emplace("deleteCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteCategory()));
		_rpcMethods.emplace("getCategories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetCategories()));
		_rpcMethods.emplace("getDevicesInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInCategory()));
		_rpcMethods.emplace("removeCategoryFromDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromDevice()));
		_rpcMethods.emplace("updateCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateCategory()));
	//}}}

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>>("registerRpcMethod", std::bind(&IpcServer::registerRpcMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
}

IpcServer::~IpcServer()
{
	if(!_stopServer) stop();
}

void IpcServer::collectGarbage()
{
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<PIpcClientData> clientsToRemove;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			try
			{
				for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
		for(std::vector<PIpcClientData>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				_clients.erase((*i)->id);
			}
			_out.printInfo("Info: IPC client " + std::to_string((*i)->id) + " removed.");
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

bool IpcServer::start()
{
	try
	{
		stop();
		_socketPath = GD::bl->settings.socketPath() + "homegearIPC.sock";
		_shuttingDown = false;
		_stopServer = false;
		if(!getFileDescriptor(true)) return false;
		uint32_t ipcThreadCount = GD::bl->settings.ipcThreadCount();
		if(ipcThreadCount < 5) ipcThreadCount = 5;
		startQueue(0, false, ipcThreadCount, 0, SCHED_OTHER);
		startQueue(1, false, ipcThreadCount, 0, SCHED_OTHER);
		startQueue(2, false, ipcThreadCount, 0, SCHED_OTHER);
		GD::bl->threadManager.start(_mainThread, true, &IpcServer::mainThread, this);
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

void IpcServer::stop()
{
	try
	{
		_shuttingDown = true;
		_out.printDebug("Debug: Waiting for IPC server's client threads to finish.");
		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				clients.push_back(i->second);
				closeClientConnection(i->second);
			}
		}

		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		while(_clients.size() > 0)
		{
			for(std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
			{
				std::unique_lock<std::mutex> waitLock((*i)->waitMutex);
				waitLock.unlock();
				(*i)->requestConditionVariable.notify_all();
			}
			collectGarbage();
			if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		stopQueue(0);
		stopQueue(1);
		stopQueue(2);
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

void IpcServer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				closeClientConnection(i->second);
			}
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

void IpcServer::broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, BaseLib::PArray values)
{
	try
	{
		if(_shuttingDown) return;
        if(!_dummyClientInfo->acls->checkEventServerMethodAccess("event")) return;

        if(_dummyClientInfo->acls->variablesRoomsCategoriesDevicesReadSet())
        {
            std::shared_ptr<BaseLib::Systems::Peer> peer;
            std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
            for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
            {
                std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
                if(central) peer = central->getPeer(id);
                if(peer) break;
            }

            if(!peer) return;

            std::shared_ptr<std::vector<std::string>> newVariables;
            BaseLib::PArray newValues;
            newVariables->reserve(variables->size());
            newValues->reserve(values->size());
            for(int32_t i = 0; i < (int32_t)variables->size(); i++)
            {
                if(id == 0)
                {
                    if(_dummyClientInfo->acls->checkSystemVariableReadAccess(variables->at(i)))
                    {
                        newVariables->push_back(variables->at(i));
                        newValues->push_back(values->at(i));
                    }
                }
                else if(peer && _dummyClientInfo->acls->checkVariableReadAccess(peer, channel, variables->at(i)))
                {
                    newVariables->push_back(variables->at(i));
                    newValues->push_back(values->at(i));
                }
            }
            variables = newVariables;
            values = newValues;
        }

		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(*variables)), BaseLib::PVariable(new BaseLib::Variable(values))});
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastEvent", parameters);
			if(!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastEvent\". Queue is full.");
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

void IpcServer::broadcastNewDevices(BaseLib::PVariable deviceDescriptions)
{
	try
	{
		if(_shuttingDown) return;

        if(!_dummyClientInfo->acls->checkEventServerMethodAccess("newDevices")) return;

		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{deviceDescriptions});
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastNewDevices", parameters);
			if(!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastNewDevices\". Queue is full.");
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

void IpcServer::broadcastDeleteDevices(BaseLib::PVariable deviceInfo)
{
	try
	{
		if(_shuttingDown) return;

        if(!_dummyClientInfo->acls->checkEventServerMethodAccess("deleteDevices")) return;

		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{deviceInfo});
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastDeleteDevices", parameters);
			if(!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastDeleteDevices\". Queue is full.");
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

void IpcServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint)
{
	try
	{
		if(_shuttingDown) return;

        if(!_dummyClientInfo->acls->checkEventServerMethodAccess("updateDevice")) return;
        if(_dummyClientInfo->acls->roomsCategoriesDevicesReadSet())
        {
            std::shared_ptr<BaseLib::Systems::Peer> peer;
            std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
            for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
            {
                std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
                if(central) peer = central->getPeer(id);
                if(!peer || !_dummyClientInfo->acls->checkDeviceReadAccess(peer)) return;
            }
        }

		std::vector<PIpcClientData> clients;
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) continue;
				clients.push_back(i->second);
			}
		}

		for(std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i)
		{
			BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(hint))});
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastUpdateDevice", parameters);
			if(!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUpdateDevice\". Queue is full.");
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

void IpcServer::closeClientConnection(PIpcClientData client)
{
	try
	{
		if(!client) return;
		GD::bl->fileDescriptorManager.shutdown(client->fileDescriptor);
		client->closed = true;
		std::vector<std::string> rpcMethodsToRemove;
		std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
		for (auto& element : _clientsByRpcMethods)
		{
			if (element.second.second->id == client->id) rpcMethodsToRemove.push_back(element.first);
		}
		for (auto& rpcMethod : rpcMethodsToRemove)
		{
			_clientsByRpcMethods.erase(rpcMethod);
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

BaseLib::PVariable IpcServer::callRpcMethod(BaseLib::PRpcClientInfo clientInfo, std::string& methodName, BaseLib::PArray& parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return BaseLib::Variable::createError(-32011, "Unauthorized.");

		PIpcClientData clientData;
		{
			std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
			auto clientIterator = _clientsByRpcMethods.find(methodName);
			if (clientIterator == _clientsByRpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + methodName);
				return BaseLib::Variable::createError(-32601, ": Requested method not found.");
			}
			clientData = clientIterator->second.second;
		}

		BaseLib::PVariable response = sendRequest(clientData, methodName, parameters);
		if (response->errorStruct)
		{
			_out.printError("Error calling \"" + methodName + "\" on client " + std::to_string(clientData->id) + ": " + response->structValue->at("faultString")->stringValue);
		}
		return response;
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void IpcServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry || queueEntry->clientData->closed) return;

		if(queueEntry->type == QueueEntry::QueueEntryType::defaultType)
		{
			if(index == 0)
			{
				std::string methodName;
				BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

				if(parameters->size() != 3)
				{
					_out.printError("Error: Wrong parameter count while calling method " + methodName);
					return;
				}

				auto localMethodIterator = _localRpcMethods.find(methodName);
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

				auto methodIterator = _rpcMethods.find(methodName);
				if(methodIterator == _rpcMethods.end())
				{
					BaseLib::PVariable result = callRpcMethod(_dummyClientInfo, methodName, parameters->at(2)->arrayValue);
					sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
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

				if(queueEntry->clientData->closed) closeClientConnection(queueEntry->clientData); //unregisterIpcClient was called
			}
			else if(index == 1)
			{
				BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
				int32_t packetId = response->arrayValue->at(0)->integerValue;

				{
					std::lock_guard<std::mutex> responseGuard(queueEntry->clientData->rpcResponsesMutex);
					auto responseIterator = queueEntry->clientData->rpcResponses.find(packetId);
					if(responseIterator != queueEntry->clientData->rpcResponses.end())
					{
						PIpcResponse element = responseIterator->second;
						if(element)
						{
							element->response = response;
							element->packetId = packetId;
							element->finished = true;
						}
					}
				}
				std::unique_lock<std::mutex> waitLock(queueEntry->clientData->waitMutex);
				waitLock.unlock();
				queueEntry->clientData->requestConditionVariable.notify_all();
			}
		}
		else if(index == 2 && queueEntry->type == QueueEntry::QueueEntryType::broadcast) //Second queue for sending packets. Response is processed by first queue
		{
			BaseLib::PVariable response = sendRequest(queueEntry->clientData, queueEntry->methodName, queueEntry->parameters);
			if(response->errorStruct)
			{
				_out.printError("Error calling \"" + queueEntry->methodName + "\" on client " + std::to_string(queueEntry->clientData->id) + ": " + response->structValue->at("faultString")->stringValue);
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

BaseLib::PVariable IpcServer::send(PIpcClientData& clientData, std::vector<char>& data)
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
				if(clientData->fileDescriptor->descriptor != -1) GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
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

BaseLib::PVariable IpcServer::sendRequest(PIpcClientData& clientData, std::string methodName, BaseLib::PArray& parameters)
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

		PIpcResponse response;
		{
			std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
			auto result = clientData->rpcResponses.emplace(packetId, std::make_shared<IpcResponse>());
			if(result.second) response = result.first->second;
		}
		if(!response)
		{
			_out.printError("Critical: Could not insert response struct into map.");
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable result = send(clientData, data);
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
			clientData->rpcResponses.erase(packetId);
			return result;
		}

		int32_t i = 0;
		std::unique_lock<std::mutex> waitLock(clientData->waitMutex);
		while(!clientData->requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&]{
			return response->finished || clientData->closed || _stopServer;
		}))
		{
			i++;
			if(i == 5)
			{
				_out.printError("Error: IPC client with ID " + std::to_string(clientData->id) + " is not responding... Closing connection.");
				closeClientConnection(clientData);
				break;
			}
		}

		if(!response->finished || response->response->arrayValue->size() != 2 || response->packetId != packetId)
		{
			_out.printError("Error: No or invalid response received to RPC request. Method: " + methodName);
			result = BaseLib::Variable::createError(-1, "No response received.");
		}
		else result = response->response->arrayValue->at(1);

		{
			std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
			clientData->rpcResponses.erase(packetId);
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

void IpcServer::sendResponse(PIpcClientData& clientData, BaseLib::PVariable& threadId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{ threadId, packetId, variable })));
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

void IpcServer::mainThread()
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
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;
			fd_set readFileDescriptor;
			int32_t maxfd = 0;
			FD_ZERO(&readFileDescriptor);
			{
				auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
				fileDescriptorGuard.lock();
				maxfd = _serverFileDescriptor->descriptor;
				FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);

				{
					std::lock_guard<std::mutex> stateGuard(_stateMutex);
					for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
			}

			result = select(maxfd + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(result == 0)
			{
				if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.ipcServerMaxConnections() * 100 / 112) collectGarbage();
				continue;
			}
			else if(result == -1)
			{
				if(errno == EINTR) continue;
				_out.printError("Error: select returned -1: " + std::string(strerror(errno)));
				continue;
			}

			if (FD_ISSET(_serverFileDescriptor->descriptor, &readFileDescriptor) && !_shuttingDown)
			{
				sockaddr_un clientAddress;
				socklen_t addressSize = sizeof(addressSize);
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientAddress, &addressSize));
				if(!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;
				_out.printInfo("Info: Connection accepted. Client number: " + std::to_string(clientFileDescriptor->id));

				if(_clients.size() > GD::bl->settings.ipcServerMaxConnections())
				{
					collectGarbage();
					if(_clients.size() > GD::bl->settings.ipcServerMaxConnections())
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
				PIpcClientData clientData = std::make_shared<IpcClientData>(clientFileDescriptor);
				clientData->id = _currentClientId++;
				_clients.emplace(clientData->id, clientData);
				continue;
			}

			PIpcClientData clientData;
			{
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				for(std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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

void IpcServer::readClient(PIpcClientData& clientData)
{
	try
	{
		int32_t processedBytes = 0;
		int32_t bytesRead = 0;
		bytesRead = read(clientData->fileDescriptor->descriptor, clientData->buffer.data(), clientData->buffer.size());
		if(bytesRead <= 0) //read returns 0, when connection is disrupted.
		{
			_out.printInfo("Info: Connection to IPC server's client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
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
					std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(clientData, clientData->binaryRpc->getData());
					if(!enqueue(clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request ? 0 : 1, queueEntry)) printQueueFullError(_out, "Error: Could not queue incoming RPC packet. Queue is full.");
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

bool IpcServer::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		if(!BaseLib::Io::directoryExists(GD::bl->settings.socketPath().c_str()))
		{
			if(errno == ENOENT) _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise IPC won't work.");
			else _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath() + ". The script engine won't work: " + strerror(errno));
			_stopServer = true;
			return false;
		}
		bool isDirectory = false;
		int32_t result = BaseLib::Io::isDirectory(GD::bl->settings.socketPath(), isDirectory);
		if(result != 0 || !isDirectory)
		{
			_out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise IPC won't work.");
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

std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> IpcServer::getRpcMethods()
{
	try
	{
		std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> rpcMethods;
		std::lock_guard<std::mutex> rpcMethodsGuard(_clientsByRpcMethodsMutex);
		for (auto& element : _clientsByRpcMethods)
		{
			rpcMethods.emplace(element.first, element.second.first);
		}
		return rpcMethods;
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>();
}

// {{{ RPC methods
BaseLib::PVariable IpcServer::registerRpcMethod(PIpcClientData& clientData, int32_t threadId, BaseLib::PArray& parameters)
{
	try
	{
		if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Method expects at least one parameter. " + std::to_string(parameters->size()) + " given.");
		if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");

		std::string methodName = parameters->at(0)->stringValue;
		BaseLib::Rpc::PRpcMethod rpcMethod = std::make_shared<BaseLib::Rpc::RpcMethod>();
		if(parameters->size() == 2)
		{
			for (auto& signature : *(parameters->at(1)->arrayValue))
			{
				BaseLib::VariableType returnType = BaseLib::VariableType::tVoid;
				std::vector<BaseLib::VariableType> parameterTypes;
				if (!signature->arrayValue->empty())
				{
					returnType = signature->arrayValue->at(0)->type;
					for (uint32_t i = 1; i < signature->arrayValue->size(); i++)
					{
						parameterTypes.push_back(signature->arrayValue->at(i)->type);
					}
				}
				rpcMethod->addSignature(returnType, parameterTypes);
			}
		}

		std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
		auto clientIterator = _clientsByRpcMethods.find(methodName);
		if (clientIterator != _clientsByRpcMethods.end())
		{
			if (clientIterator->second.second->id != clientData->id)
			{
				_out.printError("Error: Client " + std::to_string(clientData->id) + " tried to register a RPC method \"" + methodName + "\" already registed by client " + std::to_string(clientIterator->second.second->id) + ".");
				return BaseLib::Variable::createError(-2, "RPC method is already registered by another client.");
			}
			return BaseLib::PVariable(new BaseLib::Variable());
		}
		_clientsByRpcMethods.emplace(methodName, std::make_pair(rpcMethod, clientData));
		_out.printInfo("Info: Client " + std::to_string(clientData->id) + " successfully registered RPC method \"" + methodName + "\".");

		return BaseLib::PVariable(new BaseLib::Variable());
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}
