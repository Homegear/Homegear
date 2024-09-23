/* Copyright 2013-2020 Homegear GmbH
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

#include "Mqtt.h"

#include <memory>
#include "../GD/GD.h"

namespace Homegear {

Mqtt::Mqtt() : BaseLib::IQueue(GD::bl.get(), 2, 1000) {
  try {
    _packetId = 1;
    _started = false;
    _reconnecting = false;
    _connected = false;
    C1Net::TcpSocketInfo tcp_socket_info;
    auto dummy_socket = std::make_shared<C1Net::Socket>(-1);
    _socket = std::make_unique<C1Net::TcpSocket>(tcp_socket_info, dummy_socket);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Mqtt::~Mqtt() {
  try {
    stop();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::getClientName() {
  try {
    _clientName = _settings.clientName();
    if (!GD::bl->db) {
      _out.printError("Error: Database required but not available at this execution point.");
      return;
    }
    if (_clientName.empty() && !GD::bl->db->getHomegearVariableString(DatabaseController::HomegearVariables::uniqueid, _clientName)) {
      _clientName = BaseLib::HelperFunctions::getTimeUuid();
      GD::bl->db->setHomegearVariableString(DatabaseController::HomegearVariables::uniqueid, _clientName);
      _out.printInfo("Info: Created new unique client name: " + _clientName);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void Mqtt::loadSettings() {
  _settings.load(GD::bl->settings.mqttSettingsPath());
  auto parts = BaseLib::HelperFunctions::splitAll(_settings.bmxTopic() ? _settings.bmxPrefix() : _settings.prefix(), '/');
  _prefixParts = parts.size() > 0 ? parts.size() - 1 : 0;
  getClientName();
}

void Mqtt::start() {
  try {
    if (GD::rpcServers.empty()) {
      _out.printError("Error: Could not start MQTT client as there are no RPC servers available.");
      return;
    }

    if (_started) return;
    _started = true;

    _dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
    _dummyClientInfo->mqttClient = true;
    _dummyClientInfo->initInterfaceId = "mqtt";
    _dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
    std::vector<uint64_t> groups{6};
    _dummyClientInfo->acls->fromGroups(groups);
    _dummyClientInfo->user = "SYSTEM (6)";

    startQueue(0, false, 1, 0, SCHED_OTHER);
    startQueue(1, false, _settings.processingThreadCount(), 0, SCHED_OTHER);

    _out.init(GD::bl.get());
    _out.setPrefix("MQTT Client: ");
    _jsonEncoder = std::make_unique<BaseLib::Rpc::JsonEncoder>(GD::bl.get());
    _jsonDecoder = std::make_unique<BaseLib::Rpc::JsonDecoder>(GD::bl.get());

    C1Net::TcpSocketInfo tcp_socket_info;
    C1Net::TcpSocketHostInfo tcp_socket_host_info;
    tcp_socket_host_info.auto_connect = false;

    if (_settings.bmxTopic()) {
      tcp_socket_host_info.host = _settings.bmxOrgId() + '.' + _settings.bmxHostname();
      tcp_socket_host_info.port = BaseLib::Math::getUnsignedNumber(_settings.bmxPort());
      tcp_socket_host_info.tls = _settings.enableSSL();
      tcp_socket_host_info.verify_certificate = _settings.verifyCertificate();
      tcp_socket_host_info.ca_file = _settings.caFile();
      tcp_socket_host_info.client_cert_file = _settings.certPath();
      tcp_socket_host_info.client_key_file = _settings.keyPath();
    } else {
      tcp_socket_host_info.host = _settings.brokerHostname();
      tcp_socket_host_info.port = BaseLib::Math::getUnsignedNumber(_settings.brokerPort());
      tcp_socket_host_info.tls = _settings.enableSSL();
      tcp_socket_host_info.verify_certificate = _settings.verifyCertificate();
      tcp_socket_host_info.ca_file = _settings.caFile();
      tcp_socket_host_info.client_cert_file = _settings.certPath();
      tcp_socket_host_info.client_key_file = _settings.keyPath();
    }

    _socket = std::make_unique<C1Net::TcpSocket>(tcp_socket_info, tcp_socket_host_info);

    GD::bl->threadManager.join(_listenThread);
    GD::bl->threadManager.start(_listenThread, true, &Mqtt::listen, this);
    GD::bl->threadManager.join(_pingThread);
    GD::bl->threadManager.start(_pingThread, true, &Mqtt::ping, this);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::stop() {
  try {
    _started = false;
    stopQueue(1);
    stopQueue(0);
    disconnect();
    GD::bl->threadManager.join(_pingThread);
    GD::bl->threadManager.join(_listenThread);
    _reconnectThreadMutex.lock();
    GD::bl->threadManager.join(_reconnectThread);
    _reconnectThreadMutex.unlock();
    _socket->Shutdown();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

uint32_t Mqtt::getLength(std::vector<char> packet, uint32_t &lengthBytes) {
  // From section 2.2.3 of the MQTT specification version 3.1.1
  uint32_t multiplier = 1;
  uint32_t value = 0;
  uint32_t pos = 1;
  char encodedByte = 0;
  lengthBytes = 0;
  do {
    if (pos >= packet.size()) return 0;
    encodedByte = packet[pos];
    lengthBytes++;
    value += ((uint32_t)(encodedByte & 127)) * multiplier;
    multiplier *= 128;
    pos++;
    if (multiplier > 128 * 128 * 128) return 0;
  } while ((encodedByte & 128) != 0);
  return value;
}

std::vector<char> Mqtt::getLengthBytes(uint32_t length) {
  // From section 2.2.3 of the MQTT specification version 3.1.1
  std::vector<char> result;
  do {
    char byte = length % 128;
    length = length / 128;
    if (length > 0) byte = byte | 128;
    result.push_back(byte);
  } while (length > 0);
  return result;
}

void Mqtt::printConnectionError(char resultCode) {
  switch (resultCode) {
    case 0: //No error
      break;
    case 1: _out.printError("Error: Connection refused. Unacceptable protocol version.");
      break;
    case 2: _out.printError("Error: Connection refused. Client identifier rejected. Please change the client identifier in mqtt.conf.");
      break;
    case 3: _out.printError("Error: Connection refused. Server unavailable.");
      break;
    case 4: _out.printError("Error: Connection refused. Bad username or password.");
      break;
    case 5: _out.printError("Error: Connection refused. Unauthorized.");
      break;
    default: _out.printError("Error: Connection refused. Unknown error: " + std::to_string(resultCode));
      break;
  }
}

void Mqtt::getResponseByType(const std::vector<char> &packet, std::vector<char> &responseBuffer, uint8_t responseType, bool errors) {
  try {
    std::lock_guard<std::mutex> getResponseGuard(_getResponseMutex);
    if (!_socket->Connected()) {
      if (errors) _out.printError("Error: Could not send packet to MQTT server, because we are not connected.");
      return;
    }
    std::shared_ptr<RequestByType> request(new RequestByType());
    _requestsByTypeMutex.lock();
    _requestsByType[responseType] = request;
    _requestsByTypeMutex.unlock();
    std::unique_lock<std::mutex> lock(request->mutex);
    try {
      _socket->Send((uint8_t *)packet.data(), packet.size());

      if (!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(5000), [&] { return request->mutexReady; })) {
        if (errors) _out.printError("Error: No response received to packet: " + GD::bl->hf.getHexString(packet));
      }
      responseBuffer = request->response;

      _requestsByTypeMutex.lock();
      _requestsByType.erase(responseType);
      _requestsByTypeMutex.unlock();
      return;
    }
    catch (const C1Net::ClosedException &) {
      if (errors) _out.printError("Error: Socket closed while sending packet.");
    }
    catch (const C1Net::TimeoutException &ex) { _socket->Shutdown(); }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    _requestsByTypeMutex.unlock();
  }
}

void Mqtt::getResponse(const std::vector<char> &packet, std::vector<char> &responseBuffer, uint8_t responseType, int16_t packetId, bool errors) {
  try {
    std::lock_guard<std::mutex> getResponseGuard(_getResponseMutex);
    if (!_socket->Connected()) {
      _out.printError("Error: Could not send packet to MQTT server, because we are not connected.");
      return;
    }

    std::shared_ptr<Request> request(new Request(responseType));
    _requestsMutex.lock();
    _requests[packetId] = request;
    _requestsMutex.unlock();
    std::unique_lock<std::mutex> lock(request->mutex);
    try {
      send(packet);

      if (!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(5000), [&] { return request->mutexReady; })) {
        if (errors) _out.printError("Error: No response received to packet: " + GD::bl->hf.getHexString(packet));
      }
      responseBuffer = request->response;

      _requestsMutex.lock();
      _requests.erase(packetId);
      _requestsMutex.unlock();
      return;
    }
    catch (const C1Net::ClosedException &) {
      _out.printError("Error: Socket closed while sending packet.");
    }
    catch (const C1Net::TimeoutException &ex) { _socket->Shutdown(); }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    _requestsMutex.unlock();
  }
}

void Mqtt::ping() {
  try {
    std::vector<char> ping{(char)MQTT_PACKET_PINGREQ, 0};
    std::vector<char> pong(5);
    int32_t i = 0;
    while (_started) {
      if (_connected) {
        getResponseByType(ping, pong, MQTT_PACKET_PINGRESP, false);
        if (pong.empty()) {
          _out.printError("Error: No PINGRESP received.");
          _socket->Shutdown();
        }
      }

      i = 0;
      while (_started && i < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        i++;
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::listen() {
  std::vector<char> data;
  int32_t bufferMax = 2048;
  std::vector<char> buffer(bufferMax);
  uint32_t bytesReceived = 0;
  uint32_t length = 0;
  uint32_t dataLength = 0;
  uint32_t lengthBytes = 0;
  bool more_data = false;
  while (_started) {
    try {
      if (!_socket->Connected()) {
        if (!_started) return;
        reconnect();
        for (int32_t i = 0; i < 300; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          if (_socket->Connected() || !_started) break;
        }
        continue;
      }
      try {
        do {
          bytesReceived = _socket->Read((uint8_t *)buffer.data(), bufferMax, more_data);
          if (bytesReceived > 0) {
            data.insert(data.end(), buffer.data(), buffer.data() + bytesReceived);
            if (data.size() > 1000000) {
              _out.printError("Could not read packet: Too much data.");
              break;
            }
            if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: MQTT packet received: " + BaseLib::HelperFunctions::getHexString(data));
          }
          if (length == 0) {
            length = getLength(data, lengthBytes);
            dataLength = length + lengthBytes + 1;
          }

          while (length > 0 && data.size() > dataLength) {
            //Multiple MQTT packets in one TCP packet
            std::vector<char> data2(&data.at(0), &data.at(0) + dataLength);
            processData(data2);
            data2 = std::vector<char>(&data.at(dataLength), &data.at(dataLength) + (data.size() - dataLength));
            data = std::move(data2);
            length = getLength(data, lengthBytes);
            dataLength = length + lengthBytes + 1;
          }
          if (bytesReceived == (unsigned)bufferMax) {
            //Check if packet size is exactly a multiple of bufferMax
            if (data.size() == dataLength) break;
          }
        } while (bytesReceived == (unsigned)bufferMax || dataLength > data.size());
      }
      catch (const C1Net::ClosedException &ex) {
        _socket->Shutdown();
        if (_started) _out.printWarning("Warning: Connection to MQTT server closed.");
        continue;
      }
      catch (const C1Net::TimeoutException &ex) {
        continue;
      }
      catch (const C1Net::Exception &ex) {
        _socket->Shutdown();
        _out.printError("Error: " + std::string(ex.what()));
        continue;
      }

      if (data.empty()) continue;
      if (data.size() > 1000000) {
        data.clear();
        length = 0;
        continue;
      }

      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(data));

      processData(data);
      data.clear();
      length = 0;
    }
    catch (const std::exception &ex) {
      _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    data.clear();
    length = 0;
  }
}

void Mqtt::processData(std::vector<char> &data) {
  try {
    int16_t id = 0;
    uint8_t type = 0;
    if (data.size() == 2 && data.at(0) == (char)MQTT_PACKET_PINGRESP && data.at(1) == 0) {
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received ping response.");
      type = MQTT_PACKET_PINGRESP;
    } else if (data.size() == 4 && data[0] == MQTT_PACKET_CONNACK && data[1] == 2 && data[2] == 0 && data[3] == 0) //CONNACK
    {
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received CONNACK.");
      type = MQTT_PACKET_CONNACK;
    } else if (data.size() == 4 && data[0] == MQTT_PACKET_PUBACK && data[1] == 2) //PUBACK
    {
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received PUBACK.");
      id = (((uint16_t)data[2]) << 8) + (uint8_t)data[3];
    } else if (data.size() == 5 && data[0] == (char)MQTT_PACKET_SUBACK && data[1] == 3) //SUBACK
    {
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received SUBACK.");
      id = (((uint16_t)data[2]) << 8) + (uint8_t)data[3];
    }
    if (type != 0) {
      _requestsByTypeMutex.lock();
      std::map<uint8_t, std::shared_ptr<RequestByType>>::iterator requestIterator = _requestsByType.find(type);
      if (requestIterator != _requestsByType.end()) {
        std::shared_ptr<RequestByType> request = requestIterator->second;
        _requestsByTypeMutex.unlock();
        request->response = data;
        {
          std::lock_guard<std::mutex> lock(request->mutex);
          request->mutexReady = true;
        }
        request->conditionVariable.notify_one();
        return;
      } else _requestsByTypeMutex.unlock();
    }
    if (id != 0) {
      _requestsMutex.lock();
      std::map<int16_t, std::shared_ptr<Request>>::iterator requestIterator = _requests.find(id);
      if (requestIterator != _requests.end()) {
        std::shared_ptr<Request> request = requestIterator->second;
        _requestsMutex.unlock();
        if (data[0] == (char)request->getResponseControlByte()) {
          request->response = data;
          {
            std::lock_guard<std::mutex> lock(request->mutex);
            request->mutexReady = true;
          }
          request->conditionVariable.notify_one();
          return;
        }
      } else _requestsMutex.unlock();
    }
    if (data.size() > 4 && (data[0] & 0xF0) == MQTT_PACKET_PUBLISH) //PUBLISH
    {
      std::shared_ptr<BaseLib::IQueueEntry> entry(new QueueEntryReceived(data));
      if (!enqueue(1, entry)) printQueueFullError(_out, "Error: Too many received packets are queued to be processed. Your packet processing is too slow. Dropping packet.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::processPublish(std::vector<char> &data) {
  try {
    uint32_t lengthBytes = 0;
    uint32_t length = getLength(data, lengthBytes);
    if (1 + lengthBytes >= data.size() - 1 || length == 0) {
      _out.printError("Error: Invalid packet format: " + BaseLib::HelperFunctions::getHexString(data));
      return;
    }
    uint8_t qos = data[0] & 6;
    uint32_t topicLength = 1 + lengthBytes + 2 + (((uint16_t)data[1 + lengthBytes]) << 8) + (uint8_t)data[1 + lengthBytes + 1];
    uint32_t payloadPos = (qos > 0) ? topicLength + 2 : topicLength;
    if (payloadPos >= data.size()) {
      _out.printError("Error: Packet has no payload: " + BaseLib::HelperFunctions::getHexString(data));
      return;
    }
    if (qos == 4) {
      _out.printError("Error: Received publish packet with QoS 2. That was not requested.");
    } else if (qos == 2) {
      std::vector<char> puback{MQTT_PACKET_PUBACK, 2, data[topicLength], data[topicLength + 1]};
      send(puback);
    }
    std::string topic(data.data() + 1 + lengthBytes + 2, topicLength - (1 + lengthBytes + 2));
    std::string payload(data.data() + payloadPos, data.size() - payloadPos);
    std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(topic, '/');
    if (parts.size() == _prefixParts + 5 && (parts.at(_prefixParts + 1) == "value" || parts.at(_prefixParts + 1) == "set")) {
      uint64_t peerId = BaseLib::Math::getUnsignedNumber64(parts.at(_prefixParts + 2));
      int32_t channel = BaseLib::Math::getNumber(parts.at(_prefixParts + 3));

      BaseLib::PVariable value;
      try {
        if (payload.front() != '{' && payload.front() != '[') {
          std::vector<char> fixedPayload;
          bool isString = payload != "true" && payload != "false" && payload != "null" && payload.front() != '"' && !BaseLib::Math::isNumber(payload);
          if (isString) {
            value = std::make_shared<BaseLib::Variable>(payload);
          } else {
            fixedPayload.reserve(payload.size() + 2);
            fixedPayload.push_back('[');
            fixedPayload.insert(fixedPayload.end(), payload.begin(), payload.end());
            fixedPayload.push_back(']');
            value = _jsonDecoder->decode(fixedPayload);
            if (value) value = value->arrayValue->at(0);
          }
        } else {
          value = _jsonDecoder->decode(payload);
          if (value) {
            if (!value->arrayValue->empty()) {
              value = value->arrayValue->at(0);
            } else if (value->type == BaseLib::VariableType::tStruct && value->structValue->size() == 1 && value->structValue->begin()->first == "value") {
              value = value->structValue->begin()->second;
            } else _out.printError("Error: MQTT payload has unknown format.");
          }
        }
      }
      catch (std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, std::string(ex.what()) + " Payload was: " + BaseLib::HelperFunctions::getHexString(payload));
        return;
      }
      if (!value) {
        _out.printError("Error: value is nullptr.");
        return;
      }

      if (peerId == 0 && channel < 0) {
        _out.printInfo("Info: MQTT RPC call received. Method: setSystemVariable");
        BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
        parameters->arrayValue->reserve(2);
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(_prefixParts + 4))));
        parameters->arrayValue->push_back(value);
        std::string methodName = "setSystemVariable";
        BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameters);
      } else if (peerId != 0 && channel < 0) {
        _out.printInfo("Info: MQTT RPC call received. Method: setMetadata");
        BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
        parameters->arrayValue->reserve(3);
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(_prefixParts + 4))));
        parameters->arrayValue->push_back(value);
        std::string methodName = "setMetadata";
        BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameters);
      } else {
        _out.printInfo("Info: MQTT RPC call received. Method: setValue");
        BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
        parameters->arrayValue->reserve(4);
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
        parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(_prefixParts + 4))));
        parameters->arrayValue->push_back(value);
        std::string methodName = "setValue";
        BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameters);
      }
    } else if (parts.size() == _prefixParts + 5 && parts.at(_prefixParts + 1) == "config") {
      uint64_t peerId = BaseLib::Math::getNumber(parts.at(_prefixParts + 2));
      int32_t channel = BaseLib::Math::getNumber(parts.at(_prefixParts + 3));
      _out.printInfo("Info: MQTT RPC call received. Method: putParamset");
      BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
      parameters->arrayValue->reserve(4);
      parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
      parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
      parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(_prefixParts + 4))));
      BaseLib::PVariable value;
      try {
        value = _jsonDecoder->decode(payload);
      }
      catch (std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, std::string(ex.what()) + " Payload was: " + BaseLib::HelperFunctions::getHexString(payload));
        return;
      }
      if (value) parameters->arrayValue->push_back(value);
      std::string methodName = "putParamset";
      BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameters);
    } else if (parts.size() == _prefixParts + 2 && parts.at(_prefixParts + 1) == "rpc") {
      BaseLib::PVariable result;
      try {
        result = _jsonDecoder->decode(payload);
      }
      catch (std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, std::string(ex.what()) + " Payload was: " + BaseLib::HelperFunctions::getHexString(payload));
        return;
      }
      std::string methodName;
      std::string clientId;
      int32_t messageId = 0;
      BaseLib::PVariable parameters;
      if (result->type == BaseLib::VariableType::tStruct) {
        if (result->structValue->find("method") == result->structValue->end()) {
          _out.printWarning("Warning: Could not decode JSON RPC packet from MQTT payload.");
          return;
        }
        methodName = result->structValue->at("method")->stringValue;
        if (result->structValue->find("id") != result->structValue->end()) messageId = result->structValue->at("id")->integerValue;
        if (result->structValue->find("clientid") != result->structValue->end()) clientId = result->structValue->at("clientid")->stringValue;
        if (result->structValue->find("params") != result->structValue->end()) parameters = result->structValue->at("params");
        else parameters.reset(new BaseLib::Variable());
      } else {
        _out.printWarning("Warning: Could not decode MQTT RPC packet.");
        return;
      }
      _out.printInfo("Info: MQTT RPC call received. Method: " + methodName);
      BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameters);
      std::shared_ptr<MqttMessage> responseData(new MqttMessage());
      _jsonEncoder->encodeMQTTResponse(methodName, response, messageId, responseData->message);
      responseData->topic = (!clientId.empty()) ? clientId + "/rpcResult" : "rpcResult";
      responseData->retain = false;
      queueMessage(responseData);
    } else {
      _out.printWarning("Unknown topic: " + topic);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::send(const std::vector<char> &data) {
  try {
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Sending: " + BaseLib::HelperFunctions::getHexString(data));
    _socket->Send((uint8_t *)data.data(), data.size());
  }
  catch (const C1Net::ClosedException &) {
    _out.printError("Error: Socket closed while sending packet.");
  }
  catch (const C1Net::TimeoutException &ex) { _socket->Shutdown(); }
  catch (const C1Net::Exception &ex) { _socket->Shutdown(); }
}

void Mqtt::subscribe(std::string topic) {
  try {
    if (GD::bl->debugLevel >= 4) _out.printInfo("Info: Subscribing to topic " + topic);
    std::vector<char> payload;
    payload.reserve(200);
    int16_t id = 0;
    while (id == 0) id = _packetId++;
    payload.push_back(id >> 8);
    payload.push_back(id & 0xFF);
    payload.push_back(topic.size() >> 8);
    payload.push_back(topic.size() & 0xFF);
    payload.insert(payload.end(), topic.begin(), topic.end());
    payload.push_back(_settings.qos());
    std::vector<char> lengthBytes = getLengthBytes(payload.size());
    std::vector<char> subscribePacket;
    subscribePacket.reserve(1 + lengthBytes.size() + payload.size());
    subscribePacket.push_back(MQTT_PACKET_SUBSCRIBE); //Control packet type
    subscribePacket.insert(subscribePacket.end(), lengthBytes.begin(), lengthBytes.end());
    subscribePacket.insert(subscribePacket.end(), payload.begin(), payload.end());
    for (int32_t i = 0; i < 3; i++) {
      try {
        std::vector<char> response;
        getResponse(subscribePacket, response, MQTT_PACKET_SUBACK, id, false);
        if (response.size() == 0 || (response.at(4) != 0 && response.at(4) != 1 && response.at(4) != 2)) {
          //Ignore for Mosquitto, it does not send SUBACK
          if (_settings.bmxTopic()) {
            //IBM Bluemix sends SUBACK, so we need to check if SUB failed
            if (response.at(4) == 0x80)  //Failure
            {
              _out.printError("Error: Subscribe request failed for: " + topic);
            }
          }
        } else break;
      }
      catch (const C1Net::ClosedException &ex) {
        _out.printError("Error: Socket closed while sending packet.");
        break;
      }
      catch (const C1Net::TimeoutException &ex) {
        _socket->Shutdown();
        break;
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::reconnect() {
  if (!_started) return;
  try {
    std::lock_guard<std::mutex> reconnectThreadGuard(_reconnectThreadMutex);
    if (_reconnecting || _socket->Connected()) return;
    _reconnecting = true;
    GD::bl->threadManager.join(_reconnectThread);
    GD::bl->threadManager.start(_reconnectThread, true, &Mqtt::connect, this);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::connect() {
  _reconnecting = true;
  _connectMutex.lock();
  for (int32_t i = 0; i < 5; i++) {
    try {
      if (_socket->Connected() || !_started) {
        _connectMutex.unlock();
        _reconnecting = false;
        return;
      }
      _connected = false;
      _socket->Open();
      std::vector<char> payload;
      payload.reserve(200);
      payload.push_back(0); //String size MSB
      payload.push_back(4); //String size LSB
      payload.push_back('M');
      payload.push_back('Q');
      payload.push_back('T');
      payload.push_back('T');
      payload.push_back(4); //Protocol level
      payload.push_back(2); //Connect flags (Clean session)
      if (_settings.bmxTopic()) {
        if (!_settings.bmxUsername().empty()) payload.at(7) |= 0x80;
        if (!_settings.bmxToken().empty()) payload.at(7) |= 0x40;
      } else {
        if (!_settings.username().empty()) payload.at(7) |= 0x80;
        if (!_settings.password().empty()) payload.at(7) |= 0x40;
      }
      payload.push_back(0); //Keep alive MSB (in seconds)
      payload.push_back(0x3C); //Keep alive LSB

      std::string temp;
      if (_settings.bmxTopic()) {
        //IBM Bluemix Watson IOT Platform uses different client naming, so we will not use the clientName field.
        temp = "g:" + _settings.bmxOrgId() + ':' + _settings.bmxGwTypeId() + ':' + _settings.bmxDeviceId();
      } else temp = _settings.clientName();

      if (temp.empty()) temp = "Homegear";
      payload.push_back(temp.size() >> 8);
      payload.push_back(temp.size() & 0xFF);
      payload.insert(payload.end(), temp.begin(), temp.end());

      if (_settings.bmxTopic()) {
        if (!_settings.bmxUsername().empty()) {
          temp = _settings.bmxUsername();
          payload.push_back(temp.size() >> 8);
          payload.push_back(temp.size() & 0xFF);
          payload.insert(payload.end(), temp.begin(), temp.end());
        }
        if (!_settings.bmxToken().empty()) {
          temp = _settings.bmxToken();
          payload.push_back(temp.size() >> 8);
          payload.push_back(temp.size() & 0xFF);
          payload.insert(payload.end(), temp.begin(), temp.end());
        }
      } else {
        if (!_settings.username().empty()) {
          temp = _settings.username();
          payload.push_back(temp.size() >> 8);
          payload.push_back(temp.size() & 0xFF);
          payload.insert(payload.end(), temp.begin(), temp.end());
        }
        if (!_settings.password().empty()) {
          temp = _settings.password();
          payload.push_back(temp.size() >> 8);
          payload.push_back(temp.size() & 0xFF);
          payload.insert(payload.end(), temp.begin(), temp.end());
        }
      }

      std::vector<char> lengthBytes = getLengthBytes(payload.size());
      std::vector<char> connectPacket;
      connectPacket.reserve(1 + lengthBytes.size() + payload.size());
      connectPacket.push_back(MQTT_PACKET_CONNECT); //Control packet type
      connectPacket.insert(connectPacket.end(), lengthBytes.begin(), lengthBytes.end());
      connectPacket.insert(connectPacket.end(), payload.begin(), payload.end());
      std::vector<char> response(10);
      getResponseByType(connectPacket, response, MQTT_PACKET_CONNACK, false);
      bool retry = false;
      if (response.size() != 4) {
        if (response.size() == 0) {}
        else if (response.size() != 4) _out.printError("Error: CONNACK packet has wrong size.");
        else if (response[0] != MQTT_PACKET_CONNACK || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
        else if (response[3] != 1) printConnectionError(response[3]);
        retry = true;
      } else {
        _out.printInfo("Info: Successfully connected to MQTT server using protocol version 4.");
        _connected = true;
        _connectMutex.unlock();
        if (_settings.bmxTopic()) {
          //Subscribe format for IBM Bluemix Watson IOT Platform is pre-set by IBM, we have to adhere gateway commands
          subscribe(_settings.bmxPrefix() + _settings.bmxGwTypeId() + "/id/" + _settings.bmxDeviceId() + "/cmd/+/fmt/+");
          subscribe("iotdm-1/response");
        } else {
          subscribe(_settings.prefix() + _settings.homegearId() + "/rpc/#");
          subscribe(_settings.prefix() + _settings.homegearId() + "/set/#");
          subscribe(_settings.prefix() + _settings.homegearId() + "/value/#");
          subscribe(_settings.prefix() + _settings.homegearId() + "/config/#");
        }
        _reconnecting = false;
        return;
      }
      if (retry && _started) {
        _socket->Open();
        payload.clear();
        payload.reserve(200);
        payload.push_back(0); //String size MSB
        payload.push_back(6); //String size LSB
        payload.push_back('M');
        payload.push_back('Q');
        payload.push_back('I');
        payload.push_back('s');
        payload.push_back('d');
        payload.push_back('p');
        payload.push_back(3); //Protocol level
        payload.push_back(2); //Connect flags (Clean session)

        if (_settings.bmxTopic()) {
          if (!_settings.bmxUsername().empty()) payload.at(7) |= 0x80;
          if (!_settings.bmxToken().empty()) payload.at(7) |= 0x40;
        } else {
          if (!_settings.username().empty()) payload.at(7) |= 0x80;
          if (!_settings.password().empty()) payload.at(7) |= 0x40;
        }

        payload.push_back(0); //Keep alive MSB (in seconds)
        payload.push_back(0x3C); //Keep alive LSB

        if (_settings.bmxTopic()) {
          //IBM Bluemix Watson IOT Platform uses different client naming, so we will not use the clientName field.
          temp = "g:" + _settings.bmxOrgId() + ':' + _settings.bmxGwTypeId() + ':' + _settings.bmxDeviceId();
        } else temp = _settings.clientName();

        if (temp.empty()) temp = "Homegear";
        payload.push_back(temp.size() >> 8);
        payload.push_back(temp.size() & 0xFF);
        payload.insert(payload.end(), temp.begin(), temp.end());

        if (_settings.bmxTopic()) {
          if (!_settings.bmxUsername().empty()) {
            temp = _settings.bmxUsername();
            payload.push_back(temp.size() >> 8);
            payload.push_back(temp.size() & 0xFF);
            payload.insert(payload.end(), temp.begin(), temp.end());
          }
          if (!_settings.bmxToken().empty()) {
            temp = _settings.bmxToken();
            payload.push_back(temp.size() >> 8);
            payload.push_back(temp.size() & 0xFF);
            payload.insert(payload.end(), temp.begin(), temp.end());
          }
        } else {
          if (!_settings.username().empty()) {
            temp = _settings.username();
            payload.push_back(temp.size() >> 8);
            payload.push_back(temp.size() & 0xFF);
            payload.insert(payload.end(), temp.begin(), temp.end());
          }
          if (!_settings.password().empty()) {
            temp = _settings.password();
            payload.push_back(temp.size() >> 8);
            payload.push_back(temp.size() & 0xFF);
            payload.insert(payload.end(), temp.begin(), temp.end());
          }
        }

        std::vector<char> lengthBytes = getLengthBytes(payload.size());
        std::vector<char> connectPacket;
        connectPacket.reserve(1 + lengthBytes.size() + payload.size());
        connectPacket.push_back(MQTT_PACKET_CONNECT); //Control packet type
        connectPacket.insert(connectPacket.end(), lengthBytes.begin(), lengthBytes.end());
        connectPacket.insert(connectPacket.end(), payload.begin(), payload.end());
        getResponseByType(connectPacket, response, MQTT_PACKET_CONNACK, false);
        if (response.size() != 4) {
          if (response.size() == 0) _out.printError("Error: Connection to MQTT server with protocol version 3 failed.");
          else if (response.size() != 4) _out.printError("Error: CONNACK packet has wrong size.");
          else if (response[0] != MQTT_PACKET_CONNACK || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
          else printConnectionError(response[3]);
        } else {
          _out.printInfo("Info: Successfully connected to MQTT server using protocol version 3.");
          _connected = true;
          _connectMutex.unlock();
          if (_settings.bmxTopic()) {
            //subscribe format for IBM Bluemix Watson IOT Platform is pre-set by IBM, we have to adhere gateway commands
            subscribe(_settings.bmxPrefix() + _settings.bmxGwTypeId() + "/id/" + _settings.bmxDeviceId() + "/cmd/+/fmt/+");
            subscribe("iotdm-1/response");
          } else {
            subscribe(_settings.prefix() + _settings.homegearId() + "/rpc/#");
            subscribe(_settings.prefix() + _settings.homegearId() + "/set/#");
            subscribe(_settings.prefix() + _settings.homegearId() + "/value/#");
            subscribe(_settings.prefix() + _settings.homegearId() + "/config/#");
          }
          _reconnecting = false;
          return;
        }
      }

    }
    catch (const std::exception &ex) {
      _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
  _connectMutex.unlock();
  _reconnecting = false;
}

void Mqtt::disconnect() {
  try {
    std::lock_guard<std::mutex> getResponseGuard(_getResponseMutex);
    _connected = false;
    std::vector<char> disconnect = {(char)MQTT_PACKET_DISCONN, 0};
    if (_socket->Connected()) _socket->Send((uint8_t *)disconnect.data(), disconnect.size());
    _socket->Shutdown();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::queueMessage(const std::string &topic, const std::string &payload) {

  if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: queueMessage(topic, payload)-> topic:" + topic + " payload:" + payload);

  try {
    std::shared_ptr<MqttMessage> message = std::make_shared<MqttMessage>();
    message->topic = std::move(topic);
    message->message.insert(message->message.end(), payload.begin(), payload.end());
    queueMessage(message);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::queueMessage(const std::string &source, uint64_t peerId, int32_t channel, const std::string &key, const BaseLib::PVariable &value) {

  if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: queueMessage(peerId, channel, key, value) -> peerId=" + std::to_string(peerId) + ", channel=" + std::to_string(channel) + ", key=" + key + ", value=" + value->stringValue);

  try {
    bool retain = key.compare(0, 5, "PRESS") != 0;

    if (_settings.bmxTopic()) {
      //Topic has to be set to: id/deviceName/evt/eventName/fmt/json
      //Never send different message formats to Bluemix IOT platform as it will drop the connection
      std::shared_ptr<MqttMessage> messageJson = std::make_shared<MqttMessage>();
      messageJson->topic = "id/" + std::to_string(peerId) + "/evt/ch-" + std::to_string(channel) + "/fmt/json";
      BaseLib::PVariable structValue = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      structValue->structValue->emplace(key, value);
      structValue->structValue->emplace("eventSource", std::make_shared<BaseLib::Variable>(source));
      _jsonEncoder->encode(structValue, messageJson->message);
      messageJson->retain = retain;
      queueMessage(messageJson);
    } else {
      std::shared_ptr<MqttMessage> messageJson1;
      if (_settings.jsonTopic()) {
        messageJson1.reset(new MqttMessage());
        messageJson1->topic = "json/" + std::to_string(peerId) + '/' + std::to_string(channel) + '/' + key;
        _jsonEncoder->encode(value, messageJson1->message);
        messageJson1->retain = retain;
        queueMessage(messageJson1);
      }

      if (_settings.plainTopic()) {
        std::shared_ptr<MqttMessage> messagePlain(new MqttMessage());
        messagePlain->topic = "plain/" + std::to_string(peerId) + '/' + std::to_string(channel) + '/' + key;
        if (value->type == BaseLib::VariableType::tString) {
          messagePlain->message.clear();
          messagePlain->message.insert(messagePlain->message.end(), value->stringValue.begin(), value->stringValue.end());
        } else if (value->type == BaseLib::VariableType::tBinary) {
          messagePlain->message.clear();
          messagePlain->message.insert(messagePlain->message.end(), value->binaryValue.begin(), value->binaryValue.end());
        } else {
          if (messageJson1) messagePlain->message.insert(messagePlain->message.end(), messageJson1->message.begin() + 1, messageJson1->message.end() - 1);
          else {
            _jsonEncoder->encode(value, messagePlain->message);
            messagePlain->message = std::vector<char>(messagePlain->message.begin() + 1, messagePlain->message.end() - 1);
          }
        }
        messagePlain->retain = retain;
        queueMessage(messagePlain);
      }

      if (_settings.jsonobjTopic()) {
        std::shared_ptr<MqttMessage> messageJson2(new MqttMessage());
        messageJson2->topic = "jsonobj/" + std::to_string(peerId) + '/' + std::to_string(channel) + '/' + key;
        BaseLib::PVariable structValue = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        structValue->structValue->emplace("value", value);
        structValue->structValue->emplace("timestamp", std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getTime()));
        structValue->structValue->emplace("eventSource", std::make_shared<BaseLib::Variable>(source));
        _jsonEncoder->encode(structValue, messageJson2->message);
        messageJson2->retain = retain;
        queueMessage(messageJson2);
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::queueMessage(const std::string &source, uint64_t peerId, int32_t channel, const std::vector<std::string> &keys, const std::vector<BaseLib::PVariable> &values) {
  if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: queueMessage(peerId, channel, keys, values) -> peerId=" + std::to_string(peerId) + ", channel=" + std::to_string(channel) + ", keys, values");
  try {
    if (keys.empty() || keys.size() != values.size()) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("event")) return;

    std::shared_ptr<MqttMessage> messageJson2;
    BaseLib::PVariable jsonObj;
    if (_settings.bmxTopic()) {
      //Topic has to be set to: id/deviceName/evt/eventName/fmt/json
      messageJson2.reset(new MqttMessage());
      messageJson2->topic = "id/" + std::to_string(peerId) + "/evt/ch-" + std::to_string(channel) + "/fmt/json";
      jsonObj = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    } else if (_settings.jsonobjTopic()) {
      messageJson2.reset(new MqttMessage());
      messageJson2->topic = "jsonobj/" + std::to_string(peerId) + '/' + std::to_string(channel);
      jsonObj = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    }

    bool checkAcls = _dummyClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet();
    std::shared_ptr<BaseLib::Systems::Peer> peer;
    if (checkAcls && peerId != 0) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central) peer = central->getPeer(peerId);
        if (peer) break;
      }
    }

    for (int32_t i = 0; i < (signed)keys.size(); i++) {
      if (checkAcls) {
        if (peerId == 0) {
          if (_dummyClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesReadSet()) {
            auto systemVariable = GD::systemVariableController->getInternal(keys.at(i));
            if (!systemVariable || !_dummyClientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;
          }
        } else if (!peer || !_dummyClientInfo->acls->checkVariableReadAccess(peer, channel, keys.at(i))) continue;
      }

      bool retain = keys.at(i).compare(0, 5, "PRESS") != 0;

      std::shared_ptr<MqttMessage> messageJson1;
      if (_settings.bmxTopic()) {
        jsonObj->structValue->emplace(keys.at(i), values.at(i));
        if (!retain) messageJson2->retain = false;
      } else {
        //never send different format of message to bluemix IOT platform as it will drop the Connection
        //if we are using bluemix formatting we have to disable all other data formatting
        if (_settings.jsonTopic()) {
          messageJson1.reset(new MqttMessage());
          messageJson1->topic = "json/" + std::to_string(peerId) + '/' + std::to_string(channel) + '/' + keys.at(i);
          _jsonEncoder->encode(values.at(i), messageJson1->message);
          messageJson1->retain = retain;
          queueMessage(messageJson1);
        }

        if (_settings.plainTopic()) {
          std::shared_ptr<MqttMessage> messagePlain(new MqttMessage());
          messagePlain->topic = "plain/" + std::to_string(peerId) + '/' + std::to_string(channel) + '/' + keys.at(i);
          if (values.at(i)->type == BaseLib::VariableType::tString) {
            messagePlain->message.clear();
            messagePlain->message.insert(messagePlain->message.end(), values.at(i)->stringValue.begin(), values.at(i)->stringValue.end());
          } else if (values.at(i)->type == BaseLib::VariableType::tBinary) {
            messagePlain->message.clear();
            messagePlain->message.insert(messagePlain->message.end(), values.at(i)->binaryValue.begin(), values.at(i)->binaryValue.end());
          } else {
            if (messageJson1) messagePlain->message.insert(messagePlain->message.end(), messageJson1->message.begin() + 1, messageJson1->message.end() - 1);
            else {
              _jsonEncoder->encode(values.at(i), messagePlain->message);
              messagePlain->message = std::vector<char>(messagePlain->message.begin() + 1, messagePlain->message.end() - 1);
            }
          }
          messagePlain->retain = retain;
          queueMessage(messagePlain);
        }

        if (_settings.jsonobjTopic()) {
          jsonObj->structValue->emplace(keys.at(i), values.at(i));
          if (!retain) messageJson2->retain = false;
        }
      }
    }

    if ((_settings.jsonobjTopic() || _settings.bmxTopic()) && !jsonObj->structValue->empty()) {
      _jsonEncoder->encode(jsonObj, messageJson2->message);
      queueMessage(messageJson2);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::queueMessage(std::shared_ptr<MqttMessage> &message) {
  if (GD::bl->debugLevel >= 4) _out.printDebug("Debug: queueMessage (message) topic: " + message->topic + " message:" + std::string(message->message.begin(), message->message.end()));
  try {
    if (!_started || !message) return;
    std::shared_ptr<BaseLib::IQueueEntry> entry(new QueueEntrySend(message));
    if (!enqueue(0, entry)) printQueueFullError(_out, "Error: Too many packets are queued to be processed. Your packet processing is too slow. Dropping packet.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::publish(const std::string &topic, const std::vector<char> &data, bool retain) {
  try {
    if (topic.empty() || data.empty() || !_started) return;

    std::vector<char> packet;
    std::vector<char> payload;
    std::string fullTopic;

    if (_settings.bmxTopic()) {
      //Format for IBM Bluemix topic in gateway mode is: iot-2/type/mydevice/id/device1/evt/status/fmt/json
      //Format of topic received by method is: id/deviceName/evt/eventName/fmt/json
      fullTopic = _settings.bmxPrefix() + _settings.bmxDevTypeId() + '/' + topic;
      payload.reserve(fullTopic.size() + 2 + 2 + data.size());  // fixed header (2) + varheader (2) + topic + payload.
    } else {
      fullTopic = _settings.prefix() + _settings.homegearId() + "/" + topic;
      payload.reserve(fullTopic.size() + 2 + 2 + data.size());  // fixed header (2) + varheader (2) + topic + payload.
    }

    payload.push_back(fullTopic.size() >> 8);
    payload.push_back(fullTopic.size() & 0xFF);
    payload.insert(payload.end(), fullTopic.begin(), fullTopic.end());
    int16_t id = 0;
    while (id == 0) id = _packetId++;
    payload.push_back(id >> 8);
    payload.push_back(id & 0xFF);

    std::vector<char> lengthBytes;

    payload.insert(payload.end(), data.begin(), data.end());
    lengthBytes = getLengthBytes(payload.size());

    packet.reserve(1 + lengthBytes.size() + payload.size());
    retain && _settings.retain() ? packet.push_back(0x31) : packet.push_back(0x30);
    if (_settings.qos() == 1) packet.back() |= 2;
    packet.insert(packet.end(), lengthBytes.begin(), lengthBytes.end());
    packet.insert(packet.end(), payload.begin(), payload.end());
    int32_t j = 0;
    std::vector<char> response(7);
    if (GD::bl->debugLevel >= 4) _out.printInfo("MQTT Client Info: Publishing topic: " + fullTopic);
    for (int32_t i = 0; i < 25; i++) {
      if (_reconnecting) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (!_started) return;
        continue;
      }
      if (!_socket->Connected()) reconnect();
      if (!_started) break;
      if (_settings.qos() == 0) {
        send(packet);
        return;
      }
      if (i == 1) packet[0] |= 8;
      getResponse(packet, response, MQTT_PACKET_PUBACK, id, true);
      if (response.empty()) {
        //_socket->close();
        //reconnect();
        if (i >= 5) _out.printWarning("MQTT client warning: No PUBACK received.");
      } else return;

      j = 0;
      while (_started && j < 5) {

        if (i < 5) {
          j += 5;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
          j++;
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Mqtt::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    if (index == 0) //Send
    {
      std::shared_ptr<QueueEntrySend> queueEntry;
      queueEntry = std::dynamic_pointer_cast<QueueEntrySend>(entry);
      if (!queueEntry || !queueEntry->message) return;
      publish(queueEntry->message->topic, queueEntry->message->message, queueEntry->message->retain);
    } else {
      std::shared_ptr<QueueEntryReceived> queueEntry;
      queueEntry = std::dynamic_pointer_cast<QueueEntryReceived>(entry);
      if (!queueEntry) return;
      processPublish(queueEntry->data);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}