/* Copyright 2013-2020 Homegear GmbH
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

#include "Client.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

#include <memory>

namespace Homegear::Rpc {

Client::Client() {
  _lifetick1.first = 0;
  _lifetick1.second = true;

  _uniqueEventId = 0;
}

Client::~Client() {
  dispose();
}

void Client::dispose() {
  if (_disposing) return;
  _disposing = true;
  reset();
}

void Client::init() {
  //GD::bl needs to be valid, before _client is created.
  _client.reset(new RpcClient());
  _jsonEncoder = std::make_unique<BaseLib::Rpc::JsonEncoder>(GD::bl.get());
}

bool Client::lifetick() {
  try {
    std::lock_guard<std::mutex> lifetickGuard(_lifetick1Mutex);
    if (!_lifetick1.second && BaseLib::HelperFunctions::getTime() - _lifetick1.first > 900000) {
      GD::out.printCritical("Critical: RPC client's lifetick was not updated for more than 900 seconds.");
      return false;
    }
    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void Client::initServerMethods(std::pair<std::string, std::string> address) {
  try {
    GD::out.printInfo("Info: Calling init methods on server \"" + address.first + "\".");
    std::shared_ptr<RemoteRpcServer> server = getServer(address);
    if (!server) return; //server is empty when connection timed out

    //Wait a little before sending these methods, CCU needs pretty long, before it accepts the request
    if (server->type == BaseLib::RpcClientType::ccu2) std::this_thread::sleep_for(std::chrono::milliseconds(20000));
    else std::this_thread::sleep_for(std::chrono::milliseconds(500));
    systemListMethods(address);
    server = getServer(address);
    if (!server) return; //server is empty when connection timed out
    if (server->sendNewDevices) {
      listDevices(address);
      sendUnknownDevices(address);
      server = getServer(address);
    }
    if (server) server->initialized = true;

    if (BaseLib::Io::fileExists(GD::bl->settings.workingDirectory() + "core")) {
      sendError(address,
                2,
                "Error: A core file exists in Homegear's working directory (\"" + GD::bl->settings.workingDirectory()
                    + "core"
                    + "\"). Please send this file to the Homegear team including information about your system (Linux distribution, CPU architecture), the Homegear version, the current log files and information what might've caused the error.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable Client::getLastEvents(std::set<uint64_t> ids, uint32_t timespan) {
  try {
    if (timespan > 86400000) return BaseLib::Variable::createError(-1, "\"timespan\" is invalid.");

    int64_t minTime = BaseLib::HelperFunctions::getTime() - timespan;
    BaseLib::PVariable events = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

    std::lock_guard<std::mutex> eventBufferGuard(_eventBufferMutex);
    for (int32_t i = _eventBufferPosition - 1; i != _eventBufferPosition; i--) {
      if (i < 0) i = _eventBuffer.size() - 1;
      EventInfo &info = _eventBuffer.at(i);
      if (info.time < minTime) break;
      if (!ids.empty() && ids.find(info.id) == ids.end()) continue;
      BaseLib::PVariable event = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      event->structValue->insert(BaseLib::StructElement("TIME",
                                                        std::make_shared<BaseLib::Variable>((int32_t)(info.time
                                                            / 1000))));
      event->structValue->insert(BaseLib::StructElement("UNIQUEID",
                                                        std::make_shared<BaseLib::Variable>(info.uniqueId)));
      event->structValue->insert(BaseLib::StructElement("PEERID", std::make_shared<BaseLib::Variable>(info.id)));
      event->structValue->insert(BaseLib::StructElement("CHANNEL", std::make_shared<BaseLib::Variable>(info.channel)));
      event->structValue->insert(BaseLib::StructElement("VARIABLE", std::make_shared<BaseLib::Variable>(info.name)));
      event->structValue->insert(BaseLib::StructElement("VALUE", info.value));
      events->arrayValue->push_back(event);
    }
    return events;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

BaseLib::PVariable Client::getNodeEvents() {
  try {
    BaseLib::PVariable events = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    std::lock_guard<std::mutex> nodeEventCacheGuard(_nodeEventCacheMutex);
    for (auto &nodeIterator: _nodeEventCache) {
      BaseLib::PVariable node = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      for (auto &topicIterator: nodeIterator.second) {
        node->structValue->emplace(topicIterator.first, topicIterator.second);
      }
      events->structValue->emplace(nodeIterator.first, node);
    }
    return events;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

void Client::broadcastNodeEvent(const std::string &nodeId, const std::string &topic, const BaseLib::PVariable &value, bool retain) {
  try {
    if (!GD::bl->booting) {
      std::lock_guard<std::mutex> serversGuard(_serversMutex);
      for (auto &server: _servers) {
        if (!server.second->nodeEvents) continue;
        if (server.second->removed ||
            (server.second->getServerClientInfo()->sendEventsToRpcServer && (server.second->getServerClientInfo()->closed || !server.second->getServerClientInfo()->socket->Connected())) ||
            (server.second->socket && !server.second->socket->Connected() && server.second->keepAlive && !server.second->reconnectInfinitely) ||
            (!server.second->initialized && BaseLib::HelperFunctions::getTimeSeconds() - server.second->creationTime > 120)) {
          continue;
        }
        if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("nodeEvent")) continue;
        if (server.second->webSocket || server.second->json) {
          std::shared_ptr<std::list<BaseLib::PVariable>> parameters = std::make_shared<std::list<BaseLib::PVariable>>();
          parameters->push_back(std::make_shared<BaseLib::Variable>(nodeId));
          parameters->push_back(std::make_shared<BaseLib::Variable>(topic));
          parameters->push_back(value);
          server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("nodeEvent", parameters));
        }
      }
    }

    if (retain) {
      std::lock_guard<std::mutex> nodeEventCacheGuard(_nodeEventCacheMutex);
      _nodeEventCache[nodeId][topic] = value;
      if (_nodeEventCache[nodeId].size() > 100) {
        GD::out.printWarning("Warning: Event cache of node \"" + nodeId + "\" is full. Clearing it.");
      }
      if (_nodeEventCache.size() > 100000) {
        GD::out.printError("Error: Event cache is full. Clearing it.");
        _nodeEventCache.clear();
      }
    }

    if (topic.compare(0, 13, "statusBottom/") == 0 || topic.compare(0, 7, "status/") == 0) {
      auto pair = BaseLib::HelperFunctions::splitFirst(topic, '/'); //Parameter nodeId might be empty (e. g. from Node-RED) so we need to get the node ID from topic.
      GD::nodeBlueServer->broadcastStatus(pair.second, value);
    }

    if (!GD::bl->booting && BaseLib::HelperFunctions::getTime() - _lastGarbageCollection > 60000) collectGarbage();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastEvent(const std::string &source,
                            uint64_t id,
                            int32_t channel,
                            const std::string &deviceAddress,
                            const std::shared_ptr<std::vector<std::string>> &valueKeys,
                            const std::shared_ptr<std::vector<BaseLib::PVariable>> &values) {
  try {
    if (GD::bl->booting) {
      if (GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Not broadcasting event as I'm still starting up.");
      return;
    }
    if (!valueKeys || !values || valueKeys->size() != values->size()) {
      return;
    }

    {
      std::lock_guard<std::mutex> lifetickGuard(_lifetick1Mutex);
      _lifetick1.first = BaseLib::HelperFunctions::getTime();
      _lifetick1.second = false;
    }

    if (GD::mqtt->enabled()) GD::mqtt->queueMessage(source, id, channel, *valueKeys, *values); //ACL check is in MQTT
    std::string methodName("event");
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (server.second->removed ||
          (server.second->getServerClientInfo()->sendEventsToRpcServer && (server.second->getServerClientInfo()->closed || !server.second->getServerClientInfo()->socket->Connected())) ||
          (server.second->socket && !server.second->socket->Connected() && server.second->keepAlive && !server.second->reconnectInfinitely))
        continue;
      //At least OpenHAB needs PONG to be send event when initialization is not complete
      if ((!server.second->initialized && valueKeys->at(0) != "PONG") ||
          (!server.second->knownMethods.empty() && (server.second->knownMethods.find("event") == server.second->knownMethods.end() ||
              server.second->knownMethods.find("system.multicall") == server.second->knownMethods.end())))
        continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("event")) continue;
      if (id > 0 && server.second->subscribePeers && server.second->subscribedPeers.find(id) == server.second->subscribedPeers.end())
        continue;

      bool checkAcls = server.second->getServerClientInfo()->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet();
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      if (checkAcls) {
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>
            families = GD::familyController->getFamilies();
        for (auto &family: families) {
          std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
          if (central) peer = central->getPeer(id);
          if (peer) break;
        }
      }

      if (server.second->webSocket || server.second->json) {
        //No system.multicall
        for (int32_t i = 0; i < (int32_t)valueKeys->size(); i++) {
          if (checkAcls) {
            if (id == 0) {
              if (server.second->getServerClientInfo()->acls->variablesBuildingPartsRoomsCategoriesRolesReadSet()) {
                auto systemVariable = GD::systemVariableController->getInternal(valueKeys->at(i));
                if (!systemVariable || !server.second->getServerClientInfo()->acls->checkSystemVariableReadAccess(systemVariable))
                  continue;
              }
            } else if (id == 0x50000000 || id == 0x50000001) {
              if (server.second->getServerClientInfo()->acls->variablesReadSet() && !server.second->getServerClientInfo()->acls->checkNodeBlueVariableReadAccess(valueKeys->at(i), channel)) {
                continue;
              }
            } else if (!peer || !server.second->getServerClientInfo()->acls->checkVariableReadAccess(peer,
                                                                                                     channel,
                                                                                                     valueKeys->at(i)))
              continue;
          }

          std::shared_ptr<std::list<BaseLib::PVariable>> parameters = std::make_shared<std::list<BaseLib::PVariable>>();
          parameters->push_back(std::make_shared<BaseLib::Variable>(source));
          if (server.second->newFormat) {
            parameters->push_back(std::make_shared<BaseLib::Variable>(id));
            parameters->push_back(std::make_shared<BaseLib::Variable>(channel));
          } else parameters->push_back(std::make_shared<BaseLib::Variable>(deviceAddress));
          parameters->push_back(std::make_shared<BaseLib::Variable>(valueKeys->at(i)));
          parameters->push_back(values->at(i));
          server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("event", parameters));
        }
      } else {
        std::shared_ptr<std::list<BaseLib::PVariable>> parameters = std::make_shared<std::list<BaseLib::PVariable>>();
        BaseLib::PVariable array = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        BaseLib::PVariable method;
        for (int32_t i = 0; i < (int32_t)valueKeys->size(); i++) {
          if (checkAcls) {
            if (id == 0) {
              if (server.second->getServerClientInfo()->acls->variablesBuildingPartsRoomsCategoriesRolesReadSet()) {
                auto systemVariable = GD::systemVariableController->getInternal(valueKeys->at(i));
                if (!systemVariable || !server.second->getServerClientInfo()->acls->checkSystemVariableReadAccess(systemVariable))
                  continue;
              }
            } else if (id == 0x50000000 || id == 0x50000001) {
              if (server.second->getServerClientInfo()->acls->variablesReadSet() && !server.second->getServerClientInfo()->acls->checkNodeBlueVariableReadAccess(valueKeys->at(i), channel)) {
                continue;
              }
            } else if (!peer || !server.second->getServerClientInfo()->acls->checkVariableReadAccess(peer, channel, valueKeys->at(i)))
              continue;
          }

          method.reset(new BaseLib::Variable(BaseLib::VariableType::tStruct));
          array->arrayValue->push_back(method);
          method->structValue->insert(BaseLib::StructElement("methodName",
                                                             std::make_shared<BaseLib::Variable>(methodName)));
          BaseLib::PVariable params = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          method->structValue->insert(BaseLib::StructElement("params", params));

          if (!server.second->getServerClientInfo()->sendEventsToRpcServer) {
            params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(server.second->id));
          } else params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(source));
          if (server.second->newFormat) {
            params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(id));
            params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channel));
          } else params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(deviceAddress));
          params->arrayValue->push_back(std::make_shared<BaseLib::Variable>(valueKeys->at(i)));
          params->arrayValue->push_back(values->at(i));
        }
        parameters->push_back(array);
        //Sadly some clients only support multicall and not "event" directly for single events. That's why we use multicall even when there is only one value.
        server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("system.multicall", parameters));
      }
    }

    for (uint32_t i = 0; i < valueKeys->size(); i++) {
      EventInfo info;
      info.time = BaseLib::HelperFunctions::getTime();
      info.uniqueId = _uniqueEventId++;
      info.id = id;
      info.channel = channel;
      info.name = valueKeys->at(i);
      info.value = values->at(i);
      std::lock_guard<std::mutex> eventBufferGuard(_eventBufferMutex);
      _eventBuffer.at(_eventBufferPosition) = std::move(info);
      _eventBufferPosition++;
      if (_eventBufferPosition >= (signed)_eventBuffer.size()) _eventBufferPosition = 0;
    }

    {
      std::lock_guard<std::mutex> lifetickGuard(_lifetick1Mutex);
      _lifetick1.second = true;
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastServiceMessage(const BaseLib::PServiceMessage &serviceMessage) {
  try {
    try {
      std::lock_guard<std::mutex> serversGuard(_serversMutex);
      for (auto &server: _servers) {
        if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty() && server.second->knownMethods.find("serviceMessage") == server.second->knownMethods.end())) continue;
        if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("serviceMessage")) continue;
        auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
        parameters->emplace_back(serviceMessage->serialize());
        server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("serviceMessage", parameters));
      }
    }
    catch (const std::exception &ex) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::systemListMethods(std::pair<std::string, std::string> &address) {
  try {
    std::shared_ptr<RemoteRpcServer> server = getServer(address);
    if (!server) return;
    auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
    std::string methodName = "system.listMethods";
    BaseLib::PVariable result = server->invoke(methodName, parameters);
    if (result->errorStruct) {
      if (server->removed
          || (server->getServerClientInfo()->sendEventsToRpcServer && server->getServerClientInfo()->closed)
          || (server->socket && !server->socket->Connected() && server->keepAlive && !server->reconnectInfinitely))
        return;
      GD::out.printWarning(
          "Warning: Error calling XML RPC method \"system.listMethods\" on server " + address.first + " with port "
              + address.second + ". Error struct: ");
      result->print(true, false);
      return;
    }
    if (result->type != BaseLib::VariableType::tArray) return;
    server->knownMethods.clear();
    for (std::vector<BaseLib::PVariable>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end();
         ++i) {
      if ((*i)->type == BaseLib::VariableType::tString) {
        std::pair<std::string, bool> method;
        if ((*i)->stringValue.empty()) continue;
        method.first = (*i)->stringValue;
        //openHAB prepends some methods with "CallbackHandler."
        if (method.first.size() > 16 && method.first.substr(0, 16) == "CallbackHandler.")
          method.first = method.first.substr(16);
        GD::out.printDebug("Debug: Adding method " + method.first);
        method.second = true;
        server->knownMethods.insert(method);
      }
    }
    if (server->knownMethods.empty() && server->path == "/fhem/HMRPC_hmrf/request") //fhem
    {
      server->knownMethods.insert(std::pair<std::string, bool>("system.multicall", true));
      server->knownMethods.insert(std::pair<std::string, bool>("newDevices", true));
      server->knownMethods.insert(std::pair<std::string, bool>("event", true));
    }
    return;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::listDevices(std::pair<std::string, std::string> &address) {
  try {
    std::shared_ptr<RemoteRpcServer> server = getServer(address);
    if (!server) return;
    if (!server->knownMethods.empty() && server->knownMethods.find("listDevices") == server->knownMethods.end()) return;
    if (!server->getServerClientInfo()->acls->checkEventServerMethodAccess("listDevices")) return;
    auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
    if (!server->getServerClientInfo()->sendEventsToRpcServer) {
      parameters->push_back(std::make_shared<BaseLib::Variable>(server->id));
    } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
    std::string methodName = "listDevices";
    BaseLib::PVariable result = server->invoke(methodName, parameters);
    if (result->errorStruct) {
      if (server->removed
          || (server->getServerClientInfo()->sendEventsToRpcServer && server->getServerClientInfo()->closed)
          || (server->socket && !server->socket->Connected() && server->keepAlive && !server->reconnectInfinitely))
        return;
      GD::out.printError(
          "Error calling XML RPC method \"listDevices\" on server " + address.first + " with port " + address.second
              + ". Error struct: ");
      result->print(true, false);
      return;
    }
    if (result->type != BaseLib::VariableType::tArray) return;
    server->knownDevices->clear();
    for (std::vector<BaseLib::PVariable>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end();
         ++i) {
      if ((*i)->type == BaseLib::VariableType::tStruct) {
        uint64_t device = 0;
        std::string serialNumber;
        if ((*i)->structValue->find("ID") != (*i)->structValue->end()) {
          device = (*i)->structValue->at("ID")->integerValue;
          if (device == 0) continue;
        } else if ((*i)->structValue->find("ADDRESS") != (*i)->structValue->end()) {
          serialNumber = (*i)->structValue->at("ADDRESS")->stringValue;
          serialNumber = BaseLib::HelperFunctions::splitFirst(serialNumber, ':').first;
        } else continue;
        if (device == 0) //Client doesn't support ID's
        {
          if (serialNumber.empty()) break;
          std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>
              families = GD::familyController->getFamilies();
          for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin();
               i != families.end(); ++i) {
            std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
            if (central) {
              device = central->getPeerIdFromSerial(serialNumber);
              if (device > 0) break;
            }
          }
        }
        if (device > 0) server->knownDevices->insert(device);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::sendUnknownDevices(std::pair<std::string, std::string> &address) {
  try {
    std::shared_ptr<RemoteRpcServer> server = getServer(address);
    if (!server) return;
    if (!server->knownMethods.empty() && server->knownMethods.find("newDevices") == server->knownMethods.end()) return;
    if (!server->getServerClientInfo()->acls->checkEventServerMethodAccess("newDevices")) return;

    bool checkAcls = server->getServerClientInfo()->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet();

    BaseLib::PVariable devices(new BaseLib::Variable(BaseLib::VariableType::tArray));
    std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
    for (auto &family: families) {
      std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
      if (!central) continue;
      BaseLib::PVariable result = central->listDevices(server->getServerClientInfo(), true, std::map<std::string, bool>(), server->knownDevices, checkAcls);
      if (!result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
    }
    if (devices->arrayValue->empty()) return;
    auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
    if (!server->getServerClientInfo()->sendEventsToRpcServer) {
      parameters->push_back(std::make_shared<BaseLib::Variable>(server->id));
    } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
    parameters->push_back(devices);
    std::string methodName = "newDevices";
    BaseLib::PVariable result = server->invoke(methodName, parameters);
    if (result->errorStruct) {
      if (server->removed || (server->getServerClientInfo()->sendEventsToRpcServer && server->getServerClientInfo()->closed)) return;
      GD::out.printError("Error calling XML RPC method \"newDevices\" on server " + address.first + " with port " + address.second + ". Error struct: ");
      result->print(true, false);
      return;
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::sendError(const std::pair<std::string, std::string> &address, int32_t level, const std::string &message) {
  try {
    std::shared_ptr<RemoteRpcServer> server = getServer(address);
    if (!server) return;
    if (!server->newFormat
        || (!server->knownMethods.empty() && server->knownMethods.find("error") == server->knownMethods.end()))
      return;
    if (!server->getServerClientInfo()->acls->checkEventServerMethodAccess("error")) return;
    std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
    if (!server->getServerClientInfo()->sendEventsToRpcServer) {
      parameters->push_back(std::make_shared<BaseLib::Variable>(server->id));
    } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
    parameters->push_back(std::make_shared<BaseLib::Variable>(level));
    parameters->push_back(std::make_shared<BaseLib::Variable>(message));
    server->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("error", parameters));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastError(int32_t level, const std::string &message) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || !server->second->newFormat || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("error") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("error")) continue;
      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      if (!server->second->getServerClientInfo()->sendEventsToRpcServer) {
        parameters->push_back(std::make_shared<BaseLib::Variable>(server->second->id));
      } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      parameters->push_back(std::make_shared<BaseLib::Variable>(level));
      parameters->push_back(std::make_shared<BaseLib::Variable>(message));
      server->second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("error",
                                                                                                           parameters));
    }
  }
  catch (const std::exception &ex) {
    //Don't use the output object here => could cause an endless loop because of error callback which could call broadcastError again.
    std::cout << "Error in file " << __FILE__ << " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__
              << ": " << ex.what() << std::endl;
    std::cerr << "Error in file " << __FILE__ << " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__
              << ": " << ex.what() << std::endl;
  }
}

void Client::broadcastNewDevices(std::vector<uint64_t> &ids, BaseLib::PVariable deviceDescriptions) {
  try {
    if (!deviceDescriptions || ids.empty()) return;
    GD::nodeBlueServer->broadcastNewDevices(ids, deviceDescriptions);
#ifndef NO_SCRIPTENGINE
    GD::scriptEngineServer->broadcastNewDevices(ids, deviceDescriptions);
#endif
    GD::ipcServer->broadcastNewDevices(ids, deviceDescriptions);

    std::vector<std::shared_ptr<BaseLib::Systems::Peer>> peers;
    peers.reserve(ids.size());
    std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
    for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin();
         i != families.end(); ++i) {
      std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
      if (central && central->peerExists((uint64_t)ids.front())) //All ids are from the same family
      {
        for (auto id: ids) {
          auto peer = central->getPeer((uint64_t)id);
          if (!peer) return;
          peers.push_back(peer);
        }
      }
    }
    if (peers.size() != ids.size()) return;

    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    std::string methodName("newDevices");
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("newDevices") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess(methodName)) continue;
      if (server->second->getServerClientInfo()->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
        bool abort = false;
        for (auto &peer: peers) {
          if (!server->second->getServerClientInfo()->acls->checkDeviceReadAccess(peer)) {
            abort = true;
            break;
          }
        }
        if (abort) continue;
      }

      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      if (!server->second->getServerClientInfo()->sendEventsToRpcServer) {
        parameters->push_back(std::make_shared<BaseLib::Variable>(server->second->id));
      } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      parameters->push_back(deviceDescriptions);
      server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::List>>>(new std::pair<
          std::string,
          std::shared_ptr<BaseLib::List>>("newDevices", parameters)));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastNewEvent(BaseLib::PVariable eventDescription) {
  try {
    if (!eventDescription) return;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || !server->second->newFormat || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("newEvent") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("newEvent")) continue;
      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      parameters->push_back(eventDescription);
      server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::List>>>(new std::pair<
          std::string,
          std::shared_ptr<BaseLib::List>>("newEvent", parameters)));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastDeleteDevices(std::vector<uint64_t> &ids,
                                    BaseLib::PVariable deviceAddresses,
                                    BaseLib::PVariable deviceInfo) {
  try {
    if (!deviceAddresses || !deviceInfo) return;
    GD::nodeBlueServer->broadcastDeleteDevices(deviceInfo);
#ifndef NO_SCRIPTENGINE
    GD::scriptEngineServer->broadcastDeleteDevices(deviceInfo);
#endif
    GD::ipcServer->broadcastDeleteDevices(deviceInfo);

    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("deleteDevices") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("deleteDevices")) continue;

      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      if (!server->second->getServerClientInfo()->sendEventsToRpcServer) {
        parameters->push_back(std::make_shared<BaseLib::Variable>(server->second->id));
      } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      if (server->second->newFormat) parameters->push_back(deviceInfo);
      else parameters->push_back(deviceAddresses);
      server->second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>(
          "deleteDevices",
          parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastDeleteEvent(std::string id,
                                  int32_t type,
                                  uint64_t peerID,
                                  int32_t channel,
                                  std::string variable) {
  try {
    if (id.empty()) return;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || !server->second->newFormat || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("deleteEvent") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("deleteEvent")) continue;
      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      parameters->push_back(std::make_shared<BaseLib::Variable>(id));
      parameters->push_back(std::make_shared<BaseLib::Variable>((int32_t)type));
      parameters->push_back(std::make_shared<BaseLib::Variable>((int32_t)peerID));
      parameters->push_back(std::make_shared<BaseLib::Variable>(channel));
      parameters->push_back(std::make_shared<BaseLib::Variable>(variable));
      server->second->queueMethod(std::make_shared<std::pair<std::string,
                                                             std::shared_ptr<BaseLib::List>>>("deleteEvent",
                                                                                              parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastUpdateDevice(uint64_t id, int32_t channel, std::string address, Hint::Enum hint) {
  try {
    if (id == 0 || address.empty()) return;
    GD::nodeBlueServer->broadcastUpdateDevice(id, channel, hint);
#ifndef NO_SCRIPTENGINE
    GD::scriptEngineServer->broadcastUpdateDevice(id, channel, hint);
#endif
    GD::ipcServer->broadcastUpdateDevice(id, channel, hint);

    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("updateDevice") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("updateDevice")) continue;
      bool checkAcls = server->second->getServerClientInfo()->acls->devicesReadSet();
      if (id > 0 && server->second->subscribePeers
          && server->second->subscribedPeers.find(id) == server->second->subscribedPeers.end())
        continue;

      if (server->second->getServerClientInfo()->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
        std::shared_ptr<BaseLib::Systems::Peer> peer;
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>
            families = GD::familyController->getFamilies();
        for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin();
             i != families.end(); ++i) {
          std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
          if (central) peer = central->getPeer(id);
          if (peer) break;
        }

        if (checkAcls && (!peer || !server->second->getServerClientInfo()->acls->checkDeviceReadAccess(peer))) continue;
      }

      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      if (!server->second->getServerClientInfo()->sendEventsToRpcServer) {
        parameters->push_back(std::make_shared<BaseLib::Variable>(server->second->id));
      } else parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      if (server->second->newFormat) {
        parameters->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)id)));
        parameters->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
      } else parameters->push_back(BaseLib::PVariable(new BaseLib::Variable(address)));
      parameters->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)hint)));
      server->second->queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<BaseLib::List>>>(new std::pair<
          std::string,
          std::shared_ptr<BaseLib::List>>("updateDevice", parameters)));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastVariableProfileStateChanged(uint64_t profileId, bool state) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator server = _servers.begin();
         server != _servers.end(); ++server) {
      if (!server->second->initialized || !server->second->newFormat || (!server->second->knownMethods.empty()
          && server->second->knownMethods.find("variableProfileStateChanged") == server->second->knownMethods.end()))
        continue;
      if (!server->second->getServerClientInfo()->acls->checkEventServerMethodAccess("variableProfileStateChanged")) continue;
      auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(profileId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(state));
      server->second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>(
          "variableProfileStateChanged",
          parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastRequestUiRefresh(const std::string &id) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty() && server.second->knownMethods.find("requestUiRefresh") == server.second->knownMethods.end())) continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("requestUiRefresh")) continue;
      auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(id));
      server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("requestUiRefresh", parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastUiNotificationCreated(uint64_t uiNotificationId) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty() && server.second->knownMethods.find("uiNotificationCreated") == server.second->knownMethods.end())) continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("uiNotificationCreated")) continue;
      auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));
      server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("uiNotificationCreated", parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastUiNotificationRemoved(uint64_t uiNotificationId) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty() && server.second->knownMethods.find("uiNotificationRemoved") == server.second->knownMethods.end())) continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("uiNotificationRemoved")) continue;
      auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));
      server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("uiNotificationRemoved", parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastUiNotificationAction(uint64_t uiNotificationId, const std::string &uiNotificationType, uint64_t buttonId) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty() && server.second->knownMethods.find("uiNotificationAction") == server.second->knownMethods.end())) continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("uiNotificationAction")) continue;
      auto parameters = std::make_shared<std::list<BaseLib::PVariable>>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationType));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(buttonId));
      server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("uiNotificationAction", parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastUpdateEvent(const std::string &id, int32_t type, uint64_t peerID, int32_t channel, const std::string &variable) {
  try {
    if (id.empty()) return;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->initialized || !server.second->newFormat || (!server.second->knownMethods.empty()
          && server.second->knownMethods.find("updateEvent") == server.second->knownMethods.end()))
        continue;
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("updateEvent")) continue;
      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      parameters->push_back(std::make_shared<BaseLib::Variable>("homegear"));
      parameters->push_back(std::make_shared<BaseLib::Variable>(id));
      parameters->push_back(std::make_shared<BaseLib::Variable>((int32_t)type));
      parameters->push_back(std::make_shared<BaseLib::Variable>((int32_t)peerID));
      parameters->push_back(std::make_shared<BaseLib::Variable>((int32_t)channel));
      parameters->push_back(std::make_shared<BaseLib::Variable>(variable));
      server.second->queueMethod(std::make_shared<std::pair<std::string,
                                                            std::shared_ptr<BaseLib::List>>>("updateEvent",
                                                                                             parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::broadcastPtyOutput(const std::string &token, const std::string &output) {
  try {
    if (token.empty() || output.empty()) return;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (!server.second->webSocket || !server.second->newFormat || !server.second->initialized || (!server.second->knownMethods.empty() && server.second->knownMethods.find("ptyOutput") == server.second->knownMethods.end())) {
        continue;
      }
      if (!server.second->getServerClientInfo()->acls->checkEventServerMethodAccess("ptyOutput")) {
        continue;
      }
      if (BaseLib::HelperFunctions::compareConstant(server.second->getServerClientInfo()->webSocketClientId, "hg-web-ssh-" + token) != 0) continue;
      std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>());
      parameters->push_back(std::make_shared<BaseLib::Variable>(output));
      server.second->queueMethod(std::make_shared<std::pair<std::string, std::shared_ptr<BaseLib::List>>>("ptyOutput", parameters));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::reset() {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    _servers.clear();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::collectGarbage() {
  try {
    std::vector<int32_t> serversToRemove;
    int64_t now = BaseLib::HelperFunctions::getTimeSeconds();
    bool nodeClientRemoved = false;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    _lastGarbageCollection = BaseLib::HelperFunctions::getTime();
    for (auto &server: _servers) {
      if (server.second->removed || (server.second->getServerClientInfo()->sendEventsToRpcServer
          && (server.second->getServerClientInfo()->closed || !server.second->getServerClientInfo()->socket->Connected()))
          || (server.second->socket && !server.second->socket->Connected() && server.second->keepAlive
              && !server.second->reconnectInfinitely) || (!server.second->initialized && now - server.second->creationTime > 120)) {
        if (server.second->socket) server.second->socket->Shutdown();
        serversToRemove.push_back(server.first);
        if (server.second->nodeEvents) {
          nodeClientRemoved = true;
          std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
          _nodeClients.erase(server.first);
        }
      }
    }
    for (int &server: serversToRemove) {
      _servers.erase(server);
    }
    if (nodeClientRemoved) {
      bool nodeClientsEmpty = false;
      {
        std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
        nodeClientsEmpty = _nodeClients.empty();
      }
      if (nodeClientsEmpty && GD::nodeBlueServer) GD::nodeBlueServer->disableNodeEvents();
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::string Client::getIPAddress(std::string address) {
  try {
    if (address.size() < 10) {
      std::cout << BaseLib::Output::getTimeString() << " " << "Error: Server's address too short: " << address
                << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error: Server's address too short: " << address
                << std::endl;
      return "";
    }
    if (address.compare(0, 7, "http://") == 0) address = address.substr(7);
    else if (address.compare(0, 8, "https://") == 0) address = address.substr(8);
    else if (address.compare(0, 9, "binary://") == 0) address = address.substr(9);
    else if (address.size() > 10 && address.compare(0, 10, "binarys://") == 0) address = address.substr(10);
    else if (address.size() > 13 && address.compare(0, 13, "xmlrpc_bin://") == 0) address = address.substr(13);
    if (address.empty()) {
      std::cout << BaseLib::Output::getTimeString() << " " << "Error: Server's address is empty." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error: Server's address is empty." << std::endl;
      return "";
    }
    //Remove "[" and "]" of IPv6 address
    if (address.front() == '[' && address.back() == ']') address = address.substr(1, address.size() - 2);
    if (address.empty()) {
      std::cout << BaseLib::Output::getTimeString() << " " << "Error: Server's address is empty." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error: Server's address is empty." << std::endl;
      return "";
    }

    if (GD::bl->settings.tunnelClients().find(address) != GD::bl->settings.tunnelClients().end()) return "localhost";
    return address;
  }
  catch (const std::exception &ex) {
    std::cout << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
  }
  return "";
}

std::shared_ptr<RemoteRpcServer> Client::addServer(const std::pair<std::string, std::string> &address,
                                                   BaseLib::PRpcClientInfo clientInfo,
                                                   std::string path,
                                                   std::string id) {
  try {
    auto server = std::make_shared<RemoteRpcServer>(_client, clientInfo);
    removeServer(address);
    collectGarbage();
    if (_servers.size() >= GD::bl->settings.rpcClientMaxServers()) {
      GD::out.printCritical(
          "Critical: Cannot connect to more than " + std::to_string(GD::bl->settings.rpcClientMaxServers())
              + " RPC servers. You can increase this number in main.conf, if your computer is able to handle more connections.");
      return server;
    }
    GD::out.printInfo("Info: Adding server \"" + address.first + "\".");
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    server->creationTime = BaseLib::HelperFunctions::getTimeSeconds();
    server->address = address;
    server->hostname = getIPAddress(server->address.first);
    server->path = path;
    server->id = id;
    server->uid = _serverId++;
    server->settings = GD::clientSettings.get(server->hostname);
    if (server->settings) {
      GD::out.printInfo("Info: Settings for host \"" + server->hostname + "\" found in \"rpcclients.conf\".");
      if (server->settings->keepAlive) {
        GD::out.printInfo("Info: Not closing connection to client after sending packets.");
        server->keepAlive = true;
      }
    }
    _servers[server->uid] = server;
    return server;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<RemoteRpcServer>(_client, clientInfo);
}

std::shared_ptr<RemoteRpcServer> Client::addSingleConnectionServer(const std::pair<std::string, std::string> &address,
                                                                   BaseLib::PRpcClientInfo clientInfo,
                                                                   std::string id) {
  try {
    auto server = std::make_shared<RemoteRpcServer>(_client, clientInfo);
    removeServer(address);
    collectGarbage();
    GD::out.printInfo("Info: Adding server \"" + address.first + "\".");
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    server->creationTime = BaseLib::HelperFunctions::getTimeSeconds();
    server->address = address;
    server->hostname = address.first;
    server->id = id;
    server->uid = _serverId++;
    server->settings = GD::clientSettings.get(server->hostname);
    _servers[server->uid] = server;
    if (server->settings)
      GD::out.printInfo("Info: Settings for host \"" + server->hostname + "\" found in \"rpcclients.conf\".");
    return server;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<RemoteRpcServer>(_client, clientInfo);
}

std::shared_ptr<RemoteRpcServer> Client::addWebSocketServer(std::shared_ptr<C1Net::TcpSocket> socket,
                                                            const std::string &clientId,
                                                            BaseLib::PRpcClientInfo clientInfo,
                                                            std::string address,
                                                            bool nodeEvents) {
  try {
    auto server = std::make_shared<RemoteRpcServer>(_client, clientInfo);
    std::pair<std::string, std::string> serverAddress;
    serverAddress.first = clientId;
    removeServer(serverAddress);
    collectGarbage();
    if (_servers.size() >= GD::bl->settings.rpcClientMaxServers()) {
      GD::out.printCritical(
          "Critical: Cannot connect to more than " + std::to_string(GD::bl->settings.rpcClientMaxServers())
              + " RPC servers. You can increase this number in main.conf, if your computer is able to handle more connections.");
      return server;
    }
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    server->creationTime = BaseLib::HelperFunctions::getTimeSeconds();
    server->address.first = clientId;
    server->hostname = address;
    server->uid = _serverId++;
    server->webSocket = true;
    server->autoConnect = false;
    server->initialized = true;
    if (!clientInfo->sendEventsToRpcServer) {
      server->socket = socket;
    }
    server->keepAlive = true;
    server->subscribePeers = true;
    server->nodeEvents = nodeEvents;
    server->newFormat = true;
    if (nodeEvents) {
      bool nodeClientsEmpty = false;
      {
        std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
        nodeClientsEmpty = _nodeClients.empty();
      }
      if (nodeClientsEmpty && GD::nodeBlueServer) GD::nodeBlueServer->enableNodeEvents();
      std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
      _nodeClients.emplace(server->uid);
    }
    server->settings = GD::clientSettings.get(server->hostname);
    _servers[server->uid] = server;
    if (server->settings) {
      GD::out.printInfo("Info: Settings for host \"" + server->hostname + "\" found in \"rpcclients.conf\".");
      if (!clientInfo->sendEventsToRpcServer) {
        server->socket->SetReadTimeout(server->settings->timeout);
        server->socket->SetWriteTimeout(server->settings->timeout);
      } else {
        clientInfo->socket->SetReadTimeout(server->settings->timeout);
        clientInfo->socket->SetWriteTimeout(server->settings->timeout);
      }
    }
    return server;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<RemoteRpcServer>(_client, clientInfo);
}

void Client::removeServer(const std::pair<std::string, std::string> &server) {
  try {
    std::unique_lock<std::mutex> serversGuard(_serversMutex);
    for (auto i = _servers.begin(); i != _servers.end(); ++i) {
      if (i->second->address == server) {
        GD::out.printInfo("Info: Removing server \"" + i->second->address.first + "\".");
        i->second->removed = true;
        std::shared_ptr<RemoteRpcServer> current_server = i->second;
        _servers.erase(i);
        serversGuard.unlock();
        //Close waits for all read/write operations to finish and can therefore block. That's why we unlock the mutex first.
        if (current_server->socket) current_server->socket->Shutdown();
        if (current_server->nodeEvents) {
          bool nodeClientsEmpty = false;
          {
            std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
            _nodeClients.erase(current_server->uid);
            nodeClientsEmpty = _nodeClients.empty();
          }
          if (nodeClientsEmpty && GD::nodeBlueServer) GD::nodeBlueServer->disableNodeEvents();
        }
        return;
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Client::removeServer(int32_t uid) {
  try {
    std::shared_ptr<RemoteRpcServer> server;
    {
      std::lock_guard<std::mutex> serversGuard(_serversMutex);
      auto serverIterator = _servers.find(uid);
      if (serverIterator != _servers.end()) {
        server = serverIterator->second;
        _servers.erase(serverIterator);
      }
    }
    if (server && server->nodeEvents) {
      bool nodeClientsEmpty = false;
      {
        std::lock_guard<std::mutex> nodeClientsGuard(_nodeClientsMutex);
        _nodeClients.erase(server->uid);
        nodeClientsEmpty = _nodeClients.empty();
      }
      if (nodeClientsEmpty && GD::nodeBlueServer) GD::nodeBlueServer->disableNodeEvents();
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::shared_ptr<RemoteRpcServer> Client::getServer(const std::pair<std::string, std::string> &address) {
  try {
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    std::shared_ptr<RemoteRpcServer> server;
    for (auto &element: _servers) {
      if (element.second->address == address) {
        server = element.second;
        break;
      }
    }
    return server;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

BaseLib::PVariable Client::listClientServers(const std::string &id) {
  try {
    std::vector<std::shared_ptr<RemoteRpcServer>> servers;
    {
      std::lock_guard<std::mutex> serversGuard(_serversMutex);
      for (std::map<int32_t, std::shared_ptr<RemoteRpcServer>>::const_iterator i = _servers.begin();
           i != _servers.end(); ++i) {
        if (i->second->removed
            || (i->second->getServerClientInfo()->sendEventsToRpcServer && i->second->getServerClientInfo()->closed))
          continue;
        if (!id.empty() && i->second->id != id) continue;
        servers.push_back(i->second);
      }
    }
    BaseLib::PVariable serverInfos(new BaseLib::Variable(BaseLib::VariableType::tArray));
    for (std::vector<std::shared_ptr<RemoteRpcServer>>::iterator i = servers.begin(); i != servers.end(); ++i) {
      BaseLib::PVariable serverInfo(new BaseLib::Variable(BaseLib::VariableType::tStruct));
      serverInfo->structValue->insert(BaseLib::StructElement("INTERFACE_ID",
                                                             std::make_shared<BaseLib::Variable>((*i)->id)));
      serverInfo->structValue->insert(BaseLib::StructElement("HOSTNAME",
                                                             std::make_shared<BaseLib::Variable>((*i)->hostname)));
      serverInfo->structValue->insert(BaseLib::StructElement("PORT",
                                                             std::make_shared<BaseLib::Variable>((*i)->address.second)));
      serverInfo->structValue->insert(BaseLib::StructElement("PATH", std::make_shared<BaseLib::Variable>((*i)->path)));
      serverInfo->structValue->insert(BaseLib::StructElement("SINGLE_CONNECTION",
                                                             std::make_shared<BaseLib::Variable>((*i)->getServerClientInfo()->sendEventsToRpcServer)));
      serverInfo->structValue->insert(BaseLib::StructElement("SSL", std::make_shared<BaseLib::Variable>((*i)->useSSL)));
      serverInfo->structValue->insert(BaseLib::StructElement("BINARY",
                                                             std::make_shared<BaseLib::Variable>((*i)->binary)));
      serverInfo->structValue->insert(BaseLib::StructElement("WEBSOCKET",
                                                             std::make_shared<BaseLib::Variable>((*i)->webSocket)));
      serverInfo->structValue->insert(BaseLib::StructElement("KEEP_ALIVE",
                                                             std::make_shared<BaseLib::Variable>((*i)->keepAlive)));
      serverInfo->structValue->insert(BaseLib::StructElement("NEW_FORMAT",
                                                             std::make_shared<BaseLib::Variable>((*i)->newFormat)));
      if ((*i)->settings) {
        serverInfo->structValue->insert(BaseLib::StructElement("FORCESSL",
                                                               std::make_shared<BaseLib::Variable>((*i)->settings->forceSSL)));
        serverInfo->structValue->insert(BaseLib::StructElement("AUTH_TYPE",
                                                               std::make_shared<BaseLib::Variable>((uint32_t)(*i)->settings->authType)));
        serverInfo->structValue->insert(BaseLib::StructElement("VERIFICATION_HOSTNAME",
                                                               std::make_shared<BaseLib::Variable>((*i)->settings->hostname)));
        serverInfo->structValue->insert(BaseLib::StructElement("VERIFY_CERTIFICATE",
                                                               std::make_shared<BaseLib::Variable>((*i)->settings->verifyCertificate)));
      }
      serverInfo->structValue->insert(BaseLib::StructElement("LASTPACKETSENT",
                                                             std::make_shared<BaseLib::Variable>((*i)->lastPacketSent)));

      serverInfos->arrayValue->push_back(serverInfo);
    }
    return serverInfos;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Client::clientServerInitialized(std::string id) {
  try {
    bool initialized = false;
    std::lock_guard<std::mutex> serversGuard(_serversMutex);
    for (auto &server: _servers) {
      if (server.second->id == id) {
        if (server.second->removed
            || (server.second->getServerClientInfo()->sendEventsToRpcServer && server.second->getServerClientInfo()->closed))
          continue;
        else initialized = true;
        return std::make_shared<BaseLib::Variable>(initialized);
      }
    }
    return std::make_shared<BaseLib::Variable>(initialized);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}