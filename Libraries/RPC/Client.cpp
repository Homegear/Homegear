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
}

void Client::init()
{
	//GD::bl needs to be valid, before _client is created.
	_client.reset(new RpcClient());
	_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));
}

void Client::initServerMethods(std::pair<std::string, std::string> address)
{
	try
	{
		//Wait a little before sending these methods
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		systemListMethods(address);
		std::shared_ptr<RemoteRpcServer> server = getServer(address);
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

void Client::broadcastEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values)
{
	try
	{
		if(GD::bl->booting)
		{
			GD::out.printInfo("Info: Not broadcasting event as I'm still starting up.");
			return;
		}
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
		_serversMutex.lock();
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			std::cerr << "Moin1 " << server->second->address.first << std::endl;
			if(server->second->removed || (!server->second->socket->connected() && server->second->keepAlive && !server->second->reconnectInfinitely) || (!server->second->initialized && BaseLib::HelperFunctions::getTimeSeconds() - server->second->creationTime > 120)) continue;
			std::cerr << "Moin2 " << server->second->address.first << std::endl;
			if(!server->second->initialized || (!server->second->knownMethods.empty() && (server->second->knownMethods.find("event") == server->second->knownMethods.end() || server->second->knownMethods.find("system.multicall") == server->second->knownMethods.end()))) continue;
			std::cerr << "Moin3 " << server->second->address.first << std::endl;
			if(server->second->subscribePeers && server->second->subscribedPeers.find(id) == server->second->subscribedPeers.end()) continue;
			std::cerr << "Moin4 " << server->second->address.first << std::endl;
			if(server->second->webSocket || server->second->json)
			{
				//No system.multicall
				for(uint32_t i = 0; i < valueKeys->size(); i++)
				{
					std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
					parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
					if(server->second->useID)
					{
						parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)id)));
						parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
					}
					else parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(deviceAddress)));
					parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(valueKeys->at(i))));
					parameters->push_back(values->at(i));
					server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("event", parameters)));
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
					params->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
					if(server->second->useID)
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
				server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("system.multicall", parameters)));
			}
		}
		_serversMutex.unlock();
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
		std::shared_ptr<RemoteRpcServer> server = getServer(address);
		if(!server) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>> { std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->id)) });
		std::shared_ptr<BaseLib::RPC::Variable> result = _client->invoke(server, "system.listMethods", parameters);
		if(result->errorStruct)
		{
			if(server->removed || (!server->socket->connected() && server->keepAlive && !server->reconnectInfinitely)) return;
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
		std::shared_ptr<RemoteRpcServer> server = getServer(address);
		if(!server) return;
		if(!server->knownMethods.empty() && server->knownMethods.find("listDevices") == server->knownMethods.end()) return;
		std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>> { std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->id)) });
		std::shared_ptr<BaseLib::RPC::Variable> result = _client->invoke(server, "listDevices", parameters);
		if(result->errorStruct)
		{
			if(server->removed || (!server->socket->connected() && server->keepAlive && !server->reconnectInfinitely)) return;
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
		std::shared_ptr<RemoteRpcServer> server = getServer(address);
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("error") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(level)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(message)));
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("error", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("newDevices") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			parameters->push_back(deviceDescriptions);
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("newDevices", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("newEvent") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			parameters->push_back(eventDescription);
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("newEvent", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("deleteDevices") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			if(server->second->useID) parameters->push_back(deviceInfo);
			else parameters->push_back(deviceAddresses);
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("deleteDevices", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("deleteEvent") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(variable)));
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("deleteEvent", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("updateDevice") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			if(server->second->useID)
			{
				parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)id)));
				parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(channel)));
			}
			else parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(address)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)hint)));
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("updateDevice", parameters)));
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin(); server != _servers.end(); ++server)
		{
			if(!server->second->initialized || (!server->second->knownMethods.empty() && server->second->knownMethods.find("updateEvent") == server->second->knownMethods.end())) continue;
			std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters(new std::list<std::shared_ptr<BaseLib::RPC::Variable>>());
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(server->second->id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(id)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)type)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)peerID)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)channel)));
			parameters->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(variable)));
			server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>>(new std::pair<std::string, std::shared_ptr<BaseLib::RPC::RPCList>>("updateEvent", parameters)));
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
		_servers.clear();
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

void Client::collectGarbage()
{
	try
	{
		std::vector<int32_t> serversToRemove;
		int32_t now = BaseLib::HelperFunctions::getTimeSeconds();
		_serversMutex.lock();
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator i = _servers.begin(); i != _servers.end(); ++i)
		{
			if(i->second->removed || (!i->second->socket->connected() && i->second->keepAlive && !i->second->reconnectInfinitely) || (!i->second->initialized && now - i->second->creationTime > 120)) serversToRemove.push_back(i->first);
		}
		for(std::vector<int32_t>::iterator i = serversToRemove.begin(); i != serversToRemove.end(); ++i)
		{
			_servers.erase(*i);
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

std::shared_ptr<RemoteRpcServer> Client::addServer(std::pair<std::string, std::string> address, std::string path, std::string id)
{
	try
	{
		std::shared_ptr<RemoteRpcServer> server(new RemoteRpcServer(_client));
		removeServer(address);
		collectGarbage();
		if(_servers.size() >= GD::bl->settings.rpcClientMaxServers())
		{
			GD::out.printCritical("Critical: Cannot connect to more than " + std::to_string(GD::bl->settings.rpcClientMaxServers()) + " RPC servers. You can increase this number in main.conf, if your computer is able to handle more connections.");
			return server;
		}
		_serversMutex.lock();
		server->creationTime = BaseLib::HelperFunctions::getTimeSeconds();
		server->address = address;
		server->path = path;
		server->id = id;
		server->uid = _serverId++;
		_servers[server->uid] = server;
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
    return std::shared_ptr<RemoteRpcServer>(new RemoteRpcServer(_client));
}

std::shared_ptr<RemoteRpcServer> Client::addWebSocketServer(std::shared_ptr<BaseLib::SocketOperations> socket, std::string clientId, std::string address)
{
	try
	{
		std::shared_ptr<RemoteRpcServer> server(new RemoteRpcServer(_client));
		std::pair<std::string, std::string> serverAddress;
		serverAddress.first = clientId;
		removeServer(serverAddress);
		collectGarbage();
		if(_servers.size() >= GD::bl->settings.rpcClientMaxServers())
		{
			GD::out.printCritical("Critical: Cannot connect to more than " + std::to_string(GD::bl->settings.rpcClientMaxServers()) + " RPC servers. You can increase this number in main.conf, if your computer is able to handle more connections.");
			return server;
		}
		_serversMutex.lock();
		server->creationTime = BaseLib::HelperFunctions::getTimeSeconds();
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
		_servers[server->uid] = server;
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
    return std::shared_ptr<RemoteRpcServer>(new RemoteRpcServer(_client));
}

void Client::removeServer(std::pair<std::string, std::string> server)
{
	try
	{
		_serversMutex.lock();
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::iterator i = _servers.begin(); i != _servers.end(); ++i)
		{
			if(i->second->address == server)
			{
				_servers.erase(i);
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
		_servers.erase(uid);
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

std::shared_ptr<RemoteRpcServer> Client::getServer(std::pair<std::string, std::string> address)
{
	try
	{
		_serversMutex.lock();
		std::shared_ptr<RemoteRpcServer> server;
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator i = _servers.begin(); i != _servers.end(); ++i)
		{
			if(i->second->address == address)
			{
				server = i->second;
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
    return std::shared_ptr<RemoteRpcServer>();
}

std::shared_ptr<BaseLib::RPC::Variable> Client::listClientServers(std::string id)
{
	try
	{
		std::vector<std::shared_ptr<RemoteRpcServer>> servers;
		_serversMutex.lock();
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator i = _servers.begin(); i != _servers.end(); ++i)
		{
			if(!id.empty() && i->second->id != id) continue;
			servers.push_back(i->second);
		}
		_serversMutex.unlock();
		if(servers.empty()) return BaseLib::RPC::Variable::createError(-32602, "Server is unknown.");
		std::shared_ptr<BaseLib::RPC::Variable> serverInfos(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::vector<std::shared_ptr<RemoteRpcServer>>::iterator i = servers.begin(); i != servers.end(); ++i)
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
		for(std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator i = _servers.begin(); i != _servers.end(); ++i)
		{
			if(i->second->id == id)
			{
				_serversMutex.unlock();
				if(i->second->removed) continue;
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
