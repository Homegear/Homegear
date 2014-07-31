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

#include "Client.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

namespace RPC
{
Client::Client()
{
	_servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>());
}

Client::~Client()
{

}

void Client::init()
{
	//GD::bl needs to be valid, before _client is created.
	_client.reset(new RPCClient());
}

void Client::initServerMethods(std::pair<std::string, std::string> address)
{
	try
	{
		//Wait a little before sending these methods
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

void Client::broadcastEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		if(!valueKeys || !values || valueKeys->size() != values->size()) return;
		std::string methodName("event"); //We can't just create the methods BaseLib::RPC::RPCVariable with new BaseLib::RPC::RPCVariable("methodName", "event") because "event" is not a string object. That's why we create the string object here.
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
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
			std::shared_ptr<BaseLib::RPC::RPCVariable> method;
			for(uint32_t i = 0; i < valueKeys->size(); i++)
			{
				method.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
				array->arrayValue->push_back(method);
				method->structValue->insert(BaseLib::RPC::RPCStructElement("methodName", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(methodName))));
				std::shared_ptr<BaseLib::RPC::RPCVariable> params(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
				method->structValue->insert(BaseLib::RPC::RPCStructElement("params", params));
				params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
				if((*server)->useID)
				{
					params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)id)));
					params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel)));
				}
				else params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(deviceAddress)));
				params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(valueKeys->at(i))));
				params->arrayValue->push_back(values->at(i));
			}
			parameters->push_back(array);
			//Sadly some clients only support multicall and not "event" directly for single events. That's why we use multicall even when there is only one value.
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "system.multicall", parameters);
			BaseLib::Threads::setThreadPriority(GD::bl.get(), t.native_handle(), GD::bl->settings.rpcClientThreadPriority(), GD::bl->settings.rpcClientThreadPolicy());
			t.detach();
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
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>> { std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(server->id)) });
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = _client->invoke(server, "system.listMethods", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printWarning("Warning: Error calling XML RPC method system.listMethods on server " + address.first + " with port " + address.second + ". Error struct: ");
			result->print();
			return;
		}
		if(result->type != BaseLib::RPC::RPCVariableType::rpcArray) return;
		server->knownMethods.clear();
		for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end(); ++i)
		{
			if((*i)->type == BaseLib::RPC::RPCVariableType::rpcString)
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
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>> { std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(server->id)) });
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = _client->invoke(server, "listDevices", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printError("Error calling XML RPC method listDevices on server " + address.first + " with port " + address.second + ". Error struct: ");
			result->print();
			return;
		}
		if(result->type != BaseLib::RPC::RPCVariableType::rpcArray) return;
		server->knownDevices->clear();
		for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end(); ++i)
		{
			if((*i)->type == BaseLib::RPC::RPCVariableType::rpcStruct)
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
		std::shared_ptr<BaseLib::RPC::RPCVariable> devices(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->listDevices(true, std::map<std::string, bool>(), server->knownDevices);
			if(!result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}
		if(devices->arrayValue->empty()) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>{ std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(server->id)), devices });
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = _client->invoke(server, "newDevices", parameters);
		if(result->errorStruct)
		{
			if(server->removed) return;
			GD::out.printError("Error calling XML RPC method newDevices on server " + address.first + " with port " + address.second + ". Error struct: ");
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

void Client::broadcastNewDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		if(!deviceDescriptions) return;
		_serversMutex.lock();
		std::string methodName("newDevices");
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("newDevices") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			parameters->push_back(deviceDescriptions);
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "newDevices", parameters);
			t.detach();
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

void Client::broadcastNewEvent(std::shared_ptr<BaseLib::RPC::RPCVariable> eventDescription)
{
	try
	{
		if(!eventDescription) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("newEvent") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			parameters->push_back(eventDescription);
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "newEvent", parameters);
			t.detach();
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

void Client::broadcastDeleteDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceAddresses, std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo)
{
	try
	{
		if(!deviceAddresses || !deviceInfo) return;
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
		{
			if(!(*server)->initialized || (!(*server)->knownMethods.empty() && (*server)->knownMethods.find("deleteDevices") == (*server)->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			if((*server)->useID) parameters->push_back(deviceInfo);
			else parameters->push_back(deviceAddresses);
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "deleteDevices", parameters);
			t.detach();
		}
		_serversMutex.unlock();
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
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(variable)));
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "deleteEvent", parameters);
			t.detach();
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
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			if((*server)->useID)
			{
				parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)id)));
				parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel)));
			}
			else parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(address)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)hint)));
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "updateDevice", parameters);
			t.detach();
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
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*server)->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(variable)));
			std::thread t(&RPCClient::invokeBroadcast, _client.get(), (*server), "updateEvent", parameters);
			t.detach();
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
		_serversMutex.unlock();
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

std::shared_ptr<RemoteRPCServer> Client::getServer(std::pair<std::string, std::string> address)
{
	try
	{
		_serversMutex.lock();
		std::shared_ptr<RemoteRPCServer> server;
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if((*i)->address == address) server = *i;
		}
		_serversMutex.unlock();
		return server;
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
    return std::shared_ptr<RemoteRPCServer>();
}

std::shared_ptr<BaseLib::RPC::RPCVariable> Client::listClientServers(std::string id)
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> servers(new std::vector<std::shared_ptr<RemoteRPCServer>>());
		_serversMutex.lock();
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
		{
			if(!id.empty() && (*i)->id != id) continue;
			servers->push_back(*i);
		}
		_serversMutex.unlock();
		if(servers->empty()) return BaseLib::RPC::RPCVariable::createError(-32602, "Server is unknown.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> serverInfos(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = servers->begin(); i != servers->end(); ++i)
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> serverInfo(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE_ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->id))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("HOSTNAME", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->hostname))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("PORT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->address.second))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("PATH", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->path))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("SSL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->useSSL))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("BINARY", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->binary))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("KEEP_ALIVE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->keepAlive))));
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("USEID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->useID))));
			if((*i)->settings)
			{
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("FORCESSL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->settings->forceSSL))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("AUTH_TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)(*i)->settings->authType))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("VERIFICATION_HOSTNAME", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->settings->hostname))));
				serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("VERIFY_CERTIFICATE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->settings->verifyCertificate))));
			}
			serverInfo->structValue->insert(BaseLib::RPC::RPCStructElement("LASTPACKETSENT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->lastPacketSent))));

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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> Client::clientServerInitialized(std::string id)
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
				return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(initialized));
			}
		}
		_serversMutex.unlock();
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(initialized));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

}
