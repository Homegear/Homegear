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

#include "Client.h"
#include "../GD/GD.h"
#include "../MQTT/MQTT.h"
#include "../../Modules/Base/BaseLib.h"

namespace RPC
{
Client::Client()
{
	_servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>());
}

Client::~Client()
{
	dispose();
}

void Client::dispose()
{
	if(_disposing) return;
	_disposing = true;
	reset();
	int32_t i = 0;
	while(_invokeBroadcastThreads.size() > 0 && i < 300)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i++;
		if(i == 299) GD::out.printError("Error: RPC client: Waiting for \"invoke broadcast threads\" timed out.");
	}
}

void Client::init()
{
	//GD::bl needs to be valid, before _client is created.
	_client.reset(new RPCClient());
	_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));
}

void Client::initServerMethods(std::pair<std::string, std::string> address)
{
	try
	{
		//Wait a little before sending these methods
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		systemListMethods(address);
		std::shared_ptr<RemoteRPCServer> server = getServer(address);
		if(!server) return; //server is empty when connection timed out
		listDevices(address);
		sendUnknownDevices(address);
		server = getServer(address);
		if(server) server->initialized = true;
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

void Client::startInvokeBroadcastThread(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		_invokeBroadcastThreadsMutex.lock();
		if(_invokeBroadcastThreads.size() > GD::bl->settings.rpcClientThreadMax())
		{
			_invokeBroadcastThreadsMutex.unlock();
			GD::out.printCritical("Critical: More than " + std::to_string(GD::bl->settings.rpcClientThreadMax()) + " RPC broadcast threads are running. Server processing is too slow for the amount of requests. Dropping packet.", false);
			return;
		}
		int32_t threadID = _currentInvokeBroadcastThreadID++;
		std::shared_ptr<std::thread> t = std::shared_ptr<std::thread>(new std::thread(&Client::invokeBroadcastThread, this, threadID, server, methodName, parameters));
		BaseLib::Threads::setThreadPriority(GD::bl.get(), t->native_handle(), GD::bl->settings.rpcClientThreadPriority(), GD::bl->settings.rpcClientThreadPolicy());
		_invokeBroadcastThreads[threadID] = t;
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
    _invokeBroadcastThreadsMutex.unlock();
}

void Client::invokeBroadcastThread(uint32_t threadID, std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(_client) _client->invokeBroadcast(server, methodName, parameters);
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
    removeBroadcastThread(threadID);
}

void Client::removeBroadcastThread(uint32_t threadID)
{
	try
	{
		_invokeBroadcastThreadsMutex.lock();
		if(_invokeBroadcastThreads.find(threadID) != _invokeBroadcastThreads.end())
		{
			_invokeBroadcastThreads.at(threadID)->detach();
			_invokeBroadcastThreads.erase(threadID);
		}
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
    _invokeBroadcastThreadsMutex.unlock();
}

void Client::broadcastEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values)
{
	try
	{
		if(!valueKeys || !values || valueKeys->size() != values->size()) return;
		if(GD::mqtt->enabled())
		{
			for(uint32_t i = 0; i < valueKeys->size(); i++)
			{
				std::shared_ptr<std::pair<std::string, std::vector<char>>> message(new std::pair<std::string, std::vector<char>>());
				message->first = "event/" + std::to_string(id) + '/' + std::to_string(channel) + '/' + valueKeys->at(i);
				_jsonEncoder->encode(values->at(i), message->second);
				GD::mqtt->queueMessage(message);
			}
		}
		std::string methodName("event");
		std::vector<std::shared_ptr<RemoteRPCServer>> _serversToRemove;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if((*server)->removed)
			{
				_serversToRemove.push_back(*server);
				continue;
			}
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && ((*server)->knownMethods.find("event") == (*server)->knownMethods.end() || (*server)->knownMethods.find("system.multicall") == (*server)->knownMethods.end()))) continue;
			if((*server)->subscribePeers && (*server)->subscribedPeers.find(id) == (*server)->subscribedPeers.end()) continue;
			if((*server)->webSocket || (*server)->json)
			{
				//No system.multicall
				for(uint32_t i = 0; i < valueKeys->size(); i++)
				{
					std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
					parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
					if((*server)->useID)
					{
						parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)id)));
						parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
					}
					else parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(deviceAddress)));
					parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(valueKeys->at(i))));
					parameters->push_back(values->at(i));
					startInvokeBroadcastThread((*server), "event", parameters);
				}
			}
			else
			{
				std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
				std::shared_ptr<BaseLib::RPC::Variable> array(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
				std::shared_ptr<BaseLib::RPC::Variable> method;
				for(uint32_t i = 0; i < valueKeys->size(); i++)
				{
					method.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
					array->arrayValue->push_back(method);
					method->structValue->insert(BaseLib::RPC::RPCStructElement("methodName", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(methodName))));
					std::shared_ptr<BaseLib::RPC::Variable> params(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
					method->structValue->insert(BaseLib::RPC::RPCStructElement("params", params));
					params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
					if((*server)->useID)
					{
						params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)id)));
						params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
					}
					else params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(deviceAddress)));
					params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(valueKeys->at(i))));
					params->arrayValue->push_back(values->at(i));
				}
				parameters->push_back(array);
				//Sadly some clients only support multicall and not "event" directly for single events. That's why we use multicall even when there is only one value.
				startInvokeBroadcastThread((*server), "system.multicall", parameters);
			}
		}
		_serversMutex.unlock();
		if(!_serversToRemove.empty())
		{
			for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _serversToRemove.begin(); server != _serversToRemove.end(); ++server)
			{
				removeServer((*server)->address);
			}
		}
		return;
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
    _serversMutex.unlock();
}

void Client::systemListMethods(std::pair<std::string, std::string> address)
{
	try
	{
		std::shared_ptr<RemoteRPCServer> server = getServer(address);
		if(!server) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>> { std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->id)) });
		std::shared_ptr<BaseLib::RPC::Variable> result = _client->invoke(server, "system.listMethods", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printWarning("Warning: Error calling XML RPC method \"system.listMethods\" on server " + address.first + " with port " + address.second + ". Error struct: ");
			result->print();
			return;
		}
		if(result->type != BaseLib::RPC::VariableType::rpcArray) return;
		server->knownMethods.clear();
		for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end(); ++i)
		{
			if((*i)->type == BaseLib::RPC::VariableType::rpcString)
			{
				std::pair<std::string, bool> method;
				if((*i)->stringValue.empty()) continue;
				method.first = (*i)->stringValue;
				//openHAB prepends some methods with "CallbackHandler."
				if(method.first.size() > 16 && method.first.substr(0, 16) == "CallbackHandler.") method.first = method.first.substr(16);
				GD::out.printDebug("Debug: Adding method " + method.first);
				method.second = true;
				server->knownMethods.insert(method);
			}
		}
		return;
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

void Client::listDevices(std::pair<std::string, std::string> address)
{
	try
	{
		std::shared_ptr<RemoteRPCServer> server = getServer(address);
		if(!server) return;
		if(!server->knownMethods.empty() && server->knownMethods.find("listDevices") == server->knownMethods.end()) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>> { std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->id)) });
		std::shared_ptr<BaseLib::RPC::Variable> result = _client->invoke(server, "listDevices", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printError("Error calling XML RPC method \"listDevices\" on server " + address.first + " with port " + address.second + ". Error struct: ");
			result->print();
			return;
		}
		if(result->type != BaseLib::RPC::VariableType::rpcArray) return;
		server->knownDevices->clear();
		for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end(); ++i)
		{
			if((*i)->type == BaseLib::RPC::VariableType::rpcStruct)
			{
				uint64_t device = 0;
				std::string serialNumber;
				if((*i)->structValue->find("ID") != (*i)->structValue->end())
				{
					device = (*i)->structValue->at("ID")->integerValue;
					if(device == 0) continue;
				}
				else if((*i)->structValue->find("ADDRESS") != (*i)->structValue->end())
				{
					serialNumber = (*i)->structValue->at("ADDRESS")->stringValue;
				}
				else continue;
				if(device == 0) //Client doesn't support ID's
				{
					if(serialNumber.empty()) break;
					for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
					{
						std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
						if(central)
						{
							device = central->getPeerIDFromSerial(serialNumber);
							if(device > 0) break;
						}
					}
				}
				server->knownDevices->insert(device);
			}
		}
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

void Client::sendUnknownDevices(std::pair<std::string, std::string> address)
{
	try
	{
		std::shared_ptr<RemoteRPCServer> server = getServer(address);
		if(!server) return;
		if(!server->knownMethods.empty() && server->knownMethods.find("newDevices") == server->knownMethods.end()) return;
		std::shared_ptr<BaseLib::RPC::Variable> devices(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> result = central->listDevices(-1, true, std::map<std::string, bool>(), server->knownDevices);
			if(!result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}
		if(devices->arrayValue->empty()) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>{ std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->id)), devices });
		std::shared_ptr<BaseLib::RPC::Variable> result = _client->invoke(server, "newDevices", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printError("Error calling XML RPC method \"newDevices\" on server " + address.first + " with port " + address.second + ". Error struct: ");
			result->print();
			return;
		}
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

void Client::broadcastError(int32_t level, std::string message)
{
	try
	{
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("error") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(level)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(message)));
			startInvokeBroadcastThread((*server), "error", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastNewDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceDescriptions)
{
	try
	{
		if(!deviceDescriptions) return;
		_serversMutex.lock();
		std::string methodName("newDevices");
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("newDevices") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			parameters->push_back(deviceDescriptions);
			startInvokeBroadcastThread((*server), "newDevices", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastNewEvent(std::shared_ptr<BaseLib::RPC::Variable> eventDescription)
{
	try
	{
		if(!eventDescription) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("newEvent") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			parameters->push_back(eventDescription);
			startInvokeBroadcastThread((*server), "newEvent", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastDeleteDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceAddresses, std::shared_ptr<BaseLib::RPC::Variable> deviceInfo)
{
	try
	{
		if(!deviceAddresses || !deviceInfo) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("deleteDevices") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			if((*server)->useID) parameters->push_back(deviceInfo);
			else parameters->push_back(deviceAddresses);
			startInvokeBroadcastThread((*server), "deleteDevices", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastDeleteEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable)
{
	try
	{
		if(id.empty()) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("deleteEvent") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(variable)));
			startInvokeBroadcastThread((*server), "deleteEvent", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastUpdateDevice(uint64_t id, int32_t channel, std::string address, Hint::Enum hint)
{
	try
	{
		if(id == 0 || address.empty()) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("updateDevice") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			if((*server)->useID)
			{
				parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)id)));
				parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
			}
			else parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(address)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)hint)));
			startInvokeBroadcastThread((*server), "updateDevice", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::broadcastUpdateEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable)
{
	try
	{
		if(id.empty()) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("updateEvent") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*server)->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(variable)));
			startInvokeBroadcastThread((*server), "updateEvent", parameters);
		}
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
    _serversMutex.unlock();
}

void Client::reset()
{
	try
	{
		_serversMutex.lock();
		_servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>());
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
    _serversMutex.unlock();
}

std::shared_ptr<RemoteRPCServer> Client::addServer(std::pair<std::string, std::string> address, std::string path, std::string id)
{
	try
	{
		removeServer(address);
		_serversMutex.lock();
		std::shared_ptr<RemoteRPCServer> server(new RemoteRPCServer());
		server->address = address;
		server->path = path;
		server->id = id;
		server->uid = _serverId++;
		_servers->push_back(server);
		_serversMutex.unlock();
		return server;
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
    _serversMutex.unlock();
    return std::shared_ptr<RemoteRPCServer>(new RemoteRPCServer());
}

std::shared_ptr<RemoteRPCServer> Client::addWebSocketServer(std::shared_ptr<BaseLib::SocketOperations> socket, std::string clientId, std::string address)
{
	try
	{
		std::pair<std::string, std::string> serverAddress;
		serverAddress.first = clientId;
		removeServer(serverAddress);
		_serversMutex.lock();
		std::shared_ptr<RemoteRPCServer> server(new RemoteRPCServer());
		server->address.first = clientId;
		server->hostname = address;
		server->uid = _serverId++;
		server->webSocket = true;
		server->autoConnect = false;
		server->initialized = true;
		server->socket = socket;
		server->keepAlive = true;
		server->subscribePeers = true;
		server->useID = true;
		_servers->push_back(server);
		_serversMutex.unlock();
		return server;
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
    _serversMutex.unlock();
    return std::shared_ptr<RemoteRPCServer>(new RemoteRPCServer());
}

void Client::removeServer(std::pair<std::string, std::string> server)
{
	try
	{
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if((*i)->address == server)
			{
				_servers->erase(i);
				_serversMutex.unlock();
				return;
			}
		}
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
    _serversMutex.unlock();
}

void Client::removeServer(int32_t uid)
{
	try
	{
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if((*i)->uid == uid)
			{
				_servers->erase(i);
				_serversMutex.unlock();
				return;
			}
		}
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
    _serversMutex.unlock();
}

std::shared_ptr<RemoteRPCServer> Client::getServer(std::pair<std::string, std::string> address)
{
	try
	{
		_serversMutex.lock();
		std::shared_ptr<RemoteRPCServer> server;
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if((*i)->address == address)
			{
				server = *i;
				break;
			}
		}
		_serversMutex.unlock();
		return server;
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
    _serversMutex.unlock();
    return std::shared_ptr<RemoteRPCServer>();
}

std::shared_ptr<BaseLib::RPC::Variable> Client::listClientServers(std::string id)
{
	try
	{
		std::vector<std::shared_ptr<RemoteRPCServer>> servers;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if(!id.empty() && (*i)->id != id) continue;
			servers.push_back(*i);
		}
		_serversMutex.unlock();
		if(servers.empty()) return BaseLib::RPC::Variable::createError(-32602, "Server is unknown.");
		std::shared_ptr<BaseLib::RPC::Variable> serverInfos(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = servers.begin(); i != servers.end(); ++i)
		{
			std::shared_ptr<BaseLib::RPC::Variable> serverInfo(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE_ID", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->id))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("HOSTNAME", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->hostname))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("PORT", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->address.second))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("PATH", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->path))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("SSL", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->useSSL))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("BINARY", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->binary))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("KEEP_ALIVE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->keepAlive))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("USEID", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->useID))));
			if((*i)->settings)
			{
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("FORCESSL", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->settings->forceSSL))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("AUTH_TYPE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)(*i)->settings->authType))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("VERIFICATION_HOSTNAME", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->settings->hostname))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("VERIFY_CERTIFICATE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->settings->verifyCertificate))));
			}
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("LASTPACKETSENT", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((*i)->lastPacketSent))));

			serverInfos->arrayValue->push_back(serverInfo);
		}
		return serverInfos;
	}
	catch(const std::exception& ex)
    {
		_serversMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_serversMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_serversMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> Client::clientServerInitialized(std::string id)
{
	try
	{
		bool initialized = false;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if((*i)->id == id)
			{
				_serversMutex.unlock();
				if((*i)->removed) removeServer((*i)->address);
				else initialized = true;
				return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(initialized));
			}
		}
		_serversMutex.unlock();
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(initialized));
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
    _serversMutex.unlock();
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

}
