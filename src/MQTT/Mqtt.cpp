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

#include "Mqtt.h"
#include "../GD/GD.h"

Mqtt::Mqtt() : BaseLib::IQueue(GD::bl.get())
{
	try
	{
		_socket.reset(new BaseLib::SocketOperations(GD::bl.get()));
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

Mqtt::~Mqtt()
{
	try
	{
		stop();
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

void Mqtt::loadSettings()
{
	_settings.load(GD::bl->settings.mqttSettingsPath());
}

void Mqtt::start()
{
	try
	{
		if(_started) return;
		_started = true;

		startQueue(0, 0, SCHED_OTHER);

		signal(SIGPIPE, SIG_IGN);

		_out.init(GD::bl.get());
		_out.setPrefix("MQTT Client: ");
		_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));
		_jsonDecoder = std::unique_ptr<BaseLib::RPC::JsonDecoder>(new BaseLib::RPC::JsonDecoder(GD::bl.get()));
		_socket.reset(new BaseLib::SocketOperations(GD::bl.get(), _settings.brokerHostname(), _settings.brokerPort(), _settings.enableSSL(), _settings.caFile(), _settings.verifyCertificate(), _settings.certPath(), _settings.keyPath()));
		if(_listenThread.joinable()) _listenThread.join();
		_listenThread = std::thread(&Mqtt::listen, this);
		connect();
		if(_pingThread.joinable()) _pingThread.join();
		_pingThread = std::thread(&Mqtt::ping, this);
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

void Mqtt::stop()
{
	try
	{
		_started = false;
		stopQueue(0);
		disconnect();
		if(_pingThread.joinable()) _pingThread.join();
		if(_listenThread.joinable()) _listenThread.join();
		_reconnectThreadMutex.lock();
		if(_reconnectThread.joinable()) _reconnectThread.join();
		_reconnectThreadMutex.unlock();
		_socket.reset(new BaseLib::SocketOperations(GD::bl.get()));
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

uint32_t Mqtt::getLength(std::vector<char> packet, uint32_t& lengthBytes)
{
	// From section 2.2.3 of the MQTT specification version 3.1.1
	uint32_t multiplier = 1;
	uint32_t value = 0;
	uint32_t pos = 1;
	char encodedByte = 0;
	lengthBytes = 0;
	do
	{
		if(pos >= packet.size()) return 0;
		encodedByte = packet[pos];
		lengthBytes++;
		value += ((uint32_t)(encodedByte & 127)) * multiplier;
		multiplier *= 128;
		pos++;
		if(multiplier > 128 * 128 * 128) return 0;
	} while ((encodedByte & 128) != 0);
	return value;
}

std::vector<char> Mqtt::getLengthBytes(uint32_t length)
{
	// From section 2.2.3 of the MQTT specification version 3.1.1
	std::vector<char> result;
	do
	{
		char byte = length % 128;
		length = length / 128;
		if (length > 0) byte = byte | 128;
		result.push_back(byte);
	} while(length > 0);
	return result;
}

void Mqtt::printConnectionError(char resultCode)
{
	switch(resultCode)
	{
	case 0: //No error
		break;
	case 1:
		_out.printError("Error: Connection refused. Unacceptable protocol version.");
		break;
	case 2:
		_out.printError("Error: Connection refused. Client identifier rejected. Please change the client identifier in mqtt.conf.");
		break;
	case 3:
		_out.printError("Error: Connection refused. Server unavailable.");
		break;
	case 4:
		_out.printError("Error: Connection refused. Bad username or password.");
		break;
	case 5:
		_out.printError("Error: Connection refused. Unauthorized.");
		break;
	default:
		_out.printError("Error: Connection refused. Unknown error: " + std::to_string(resultCode));
		break;
	}
}

void Mqtt::getResponseByType(const std::vector<char>& packet, std::vector<char>& responseBuffer, uint8_t responseType, bool errors)
{
	try
	{
		if(!_socket->connected())
		{
			if(errors) _out.printError("Error: Could not send packet to MQTT server, because we are not connected.");
			return;
		}
		std::shared_ptr<RequestByType> request(new RequestByType());
		_requestsByTypeMutex.lock();
		_requestsByType[responseType] = request;
		_requestsByTypeMutex.unlock();
		std::unique_lock<std::mutex> lock(request->mutex);
		try
		{
			_socket->proofwrite(packet);

			if(!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(5000), [&] { return request->mutexReady; }))
			{
				if(errors) _out.printError("Error: No response received to packet: " + GD::bl->hf.getHexString(packet));
			}
			responseBuffer = request->response;

			_requestsByTypeMutex.lock();
			_requestsByType.erase(responseType);
			_requestsByTypeMutex.unlock();
			return;
		}
		catch(BaseLib::SocketClosedException&)
		{
			if(errors) _out.printError("Error: Socket closed while sending packet.");
		}
		catch(BaseLib::SocketTimeOutException& ex) { _socket->close(); }
	}
	catch(const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_requestsByTypeMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_requestsByTypeMutex.unlock();
	}
	catch(...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_requestsByTypeMutex.unlock();
	}
}

void Mqtt::getResponse(const std::vector<char>& packet, std::vector<char>& responseBuffer, uint8_t responseType, int16_t packetId, bool errors)
{
	try
	{
		if(!_socket->connected())
		{
			_out.printError("Error: Could not send packet to MQTT server, because we are not connected.");
			return;
		}
		std::shared_ptr<Request> request(new Request(responseType));
		_requestsMutex.lock();
		_requests[packetId] = request;
		_requestsMutex.unlock();
		std::unique_lock<std::mutex> lock(request->mutex);
		try
		{
			send(packet);

			if(!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(5000), [&] { return request->mutexReady; }))
			{
				if(errors) _out.printError("Error: No response received to packet: " + GD::bl->hf.getHexString(packet));
			}
			responseBuffer = request->response;

			_requestsMutex.lock();
			_requests.erase(packetId);
			_requestsMutex.unlock();
			return;
		}
		catch(BaseLib::SocketClosedException&)
		{
			_out.printError("Error: Socket closed while sending packet.");
		}
		catch(BaseLib::SocketTimeOutException& ex) { _socket->close(); }
	}
	catch(const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_requestsMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_requestsMutex.unlock();
	}
	catch(...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_requestsMutex.unlock();
	}
}

void Mqtt::ping()
{
	try
	{
		std::vector<char> ping { (char)0xC0, 0 };
		std::vector<char> pong(5);
		int32_t i = 0;
		while(_started)
		{
			if(_connected)
			{
				getResponseByType(ping, pong, 0xD0);
				if(pong.empty())
				{
					_out.printError("Error: No PINGRESP received.");
					_socket->close();
				}
			}

			i = 0;
			while(_started && i < 20)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				i++;
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

void Mqtt::listen()
{
	std::vector<char> data;
	int32_t bufferMax = 2048;
	std::vector<char> buffer(bufferMax);
	uint32_t bytesReceived = 0;
	uint32_t length = 0;
	uint32_t dataLength = 0;
	uint32_t lengthBytes = 0;
	while(_started)
	{
		try
		{
			if(!_socket->connected())
			{
				if(!_started) return;
				reconnect();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			try
			{
				do
				{
					bytesReceived = _socket->proofread(&buffer[0], bufferMax);
					if(bytesReceived > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + bytesReceived);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read packet: Too much data.");
							break;
						}
					}
					if(length == 0)
					{
						length = getLength(data, lengthBytes);
						dataLength = length + lengthBytes + 1;
					}

					if(dataLength > 0 && data.size() > dataLength)
					{
						//Multiple MQTT packets in one TCP packet
						std::vector<char> data2(&data.at(0), &data.at(dataLength - 1));
						processData(data2);
						data2 = std::vector<char>(&data.at(dataLength), &data.at(dataLength + (data.size() - dataLength - 1)));
						data = data2;
					}
					if(bytesReceived == (unsigned)bufferMax)
					{
						//Check if packet size is exactly a multiple of bufferMax
						if(data.size() == dataLength) break;
					}
				} while(bytesReceived == (unsigned)bufferMax);
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				_socket->close();
				_out.printWarning("Warning: Connection to MQTT server closed.");
				continue;
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				_socket->close();
				_out.printError("Error: " + ex.what());
				continue;
			}

			if(data.empty()) continue;
			if(data.size() > 1000000)
			{
				data.clear();
				length = 0;
				continue;
			}

			processData(data);
			data.clear();
			length = 0;
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
		data.clear();
		length = 0;
	}
}

void Mqtt::processData(std::vector<char>& data)
{
	try
	{
		int16_t id = 0;
		uint8_t type = 0;
		if(data.size() == 2 && data.at(0) == (char)0xD0 && data.at(1) == 0)
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received ping response.");
			type = 0xD0;
		}
		else if(data.size() == 4 && data[0] == 0x20 && data[1] == 2 && data[2] == 0 && data[3] == 0) //CONNACK
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received CONNACK.");
			type = 0x20;
		}
		else if(data.size() == 4 && data[0] == 0x40 && data[1] == 2) //PUBACK
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received PUBACK.");
			id = (((uint16_t)data[2]) << 8) + (uint8_t)data[3];
		}
		else if(data.size() == 5 && data[0] == (char)0x90 && data[1] == 3) //SUBACK
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Received SUBACK.");
			id = (((uint16_t)data[2]) << 8) + (uint8_t)data[3];
		}
		if(type != 0)
		{
			_requestsByTypeMutex.lock();
			std::map<uint8_t, std::shared_ptr<RequestByType>>::iterator requestIterator = _requestsByType.find(type);
			if(requestIterator != _requestsByType.end())
			{
				std::shared_ptr<RequestByType> request = requestIterator->second;
				_requestsByTypeMutex.unlock();
				request->response = data;
				{
					std::lock_guard<std::mutex> lock(request->mutex);
					request->mutexReady = true;
				}
				request->conditionVariable.notify_one();
				return;
			}
			else _requestsByTypeMutex.unlock();
		}
		if(id != 0)
		{
			_requestsMutex.lock();
			std::map<int16_t, std::shared_ptr<Request>>::iterator requestIterator = _requests.find(id);
			if(requestIterator != _requests.end())
			{
				std::shared_ptr<Request> request = requestIterator->second;
				_requestsMutex.unlock();
				if(data[0] == (char)request->getResponseControlByte())
				{
					request->response = data;
					{
						std::lock_guard<std::mutex> lock(request->mutex);
						request->mutexReady = true;
					}
					request->conditionVariable.notify_one();
					return;
				}
			}
			else _requestsMutex.unlock();
		}
		if(data.size() > 4 && (data[0] & 0xF0) == 0x30) //PUBLISH
		{
			processPublish(data);
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

void Mqtt::processPublish(std::vector<char>& data)
{
	try
	{
		uint32_t lengthBytes = 0;
		getLength(data, lengthBytes);
		if(1 + lengthBytes >= data.size() - 1)
		{
			_out.printError("Error: Invalid packet format: " + BaseLib::HelperFunctions::getHexString(data));
			return;
		}
		uint32_t idPos = 1 + lengthBytes + 2 + (data[1 + lengthBytes] << 8) + data[1 + lengthBytes + 1];
		if(idPos >= data.size())
		{
			_out.printError("Error: Invalid packet format: " + BaseLib::HelperFunctions::getHexString(data));
			return;
		}
		if((data[0] & 0x06) == 4)
		{
			_out.printError("Error: Received publish packet with QoS 2. That was not requested.");
		}
		else if((data[0] & 0x06) == 2)
		{
			std::vector<char> puback { 0x40, 2, data[idPos], data[idPos + 1] };
			send(puback);
		}
		std::string topic(&data[1 + lengthBytes + 2], idPos - (1 + lengthBytes + 2));
		if(idPos + 2 >= data.size())
		{
			_out.printError("Error: Packet has no data: " + BaseLib::HelperFunctions::getHexString(data));
			return;
		}
		std::string payload(&data[idPos + 2], data.size() - (idPos + 2));
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(topic, '/');
		if(parts.size() == 6 && parts.at(2) == "value")
		{
			uint64_t peerId = BaseLib::Math::getNumber(parts.at(3));
			int32_t channel = BaseLib::Math::getNumber(parts.at(4));
			GD::out.printInfo("Info: MQTT RPC call received. Method: setValue");
			BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
			parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
			parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
			parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(5))));
			BaseLib::PVariable value = _jsonDecoder->decode(payload);
			if(value && value->arrayValue->size() > 0)
			{
				parameters->arrayValue->push_back(value->arrayValue->at(0));
			}
			BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod("setValue", parameters);
		}
		else if(parts.size() == 3 && parts.at(2) == "rpc")
		{
			BaseLib::PVariable result = _jsonDecoder->decode(payload);
			std::string methodName;
			std::string clientId;
			int32_t messageId = 0;
			BaseLib::PVariable parameters;
			if(result->type == BaseLib::VariableType::tStruct)
			{
				if(result->structValue->find("method") == result->structValue->end())
				{
					_out.printWarning("Warning: Could not decode JSON RPC packet from MQTT payload.");
					return;
				}
				methodName = result->structValue->at("method")->stringValue;
				if(result->structValue->find("id") != result->structValue->end()) messageId = result->structValue->at("id")->integerValue;
				if(result->structValue->find("clientid") != result->structValue->end()) clientId = result->structValue->at("clientid")->stringValue;
				if(result->structValue->find("params") != result->structValue->end()) parameters = result->structValue->at("params");
				else parameters.reset(new BaseLib::Variable());
			}
			else
			{
				_out.printWarning("Warning: Could not decode MQTT RPC packet.");
				return;
			}
			GD::out.printInfo("Info: MQTT RPC call received. Method: " + methodName);
			BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
			std::shared_ptr<std::pair<std::string, std::vector<char>>> responseData(new std::pair<std::string, std::vector<char>>());
			_jsonEncoder->encodeMQTTResponse(methodName, response, messageId, responseData->second);
			responseData->first = (!clientId.empty()) ? clientId + "/rpcResult" : "rpcResult";
			queueMessage(responseData);
		}
		else
		{
			_out.printWarning("Unknown topic: " + topic);
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

void Mqtt::send(const std::vector<char>& data)
{
	try
	{
		if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Sending: " + BaseLib::HelperFunctions::getHexString(data));
		_socket->proofwrite(data);
	}
	catch(BaseLib::SocketClosedException&)
	{
		_out.printError("Error: Socket closed while sending packet.");
	}
	catch(BaseLib::SocketTimeOutException& ex) { _socket->close(); }
	catch(BaseLib::SocketOperationException& ex) { _socket->close(); }
}

void Mqtt::subscribe(std::string topic)
{
	try
	{
		std::vector<char> payload;
		payload.reserve(200);
		int16_t id = 0;
		while(id == 0) id = _packetId++;
		payload.push_back(id >> 8);
		payload.push_back(id & 0xFF);
		payload.push_back(topic.size() >> 8);
		payload.push_back(topic.size() & 0xFF);
		payload.insert(payload.end(), topic.begin(), topic.end());
		payload.push_back(1); //QoS
		std::vector<char> lengthBytes = getLengthBytes(payload.size());
		std::vector<char> subscribePacket;
		subscribePacket.reserve(1 + lengthBytes.size() + payload.size());
		subscribePacket.push_back(0x82); //Control packet type
		subscribePacket.insert(subscribePacket.end(), lengthBytes.begin(), lengthBytes.end());
		subscribePacket.insert(subscribePacket.end(), payload.begin(), payload.end());
		for(int32_t i = 0; i < 3; i++)
		{
			try
			{
				std::vector<char> response;
				getResponse(subscribePacket, response, 0x90, id, false);
				if(response.size() == 0 || (response.at(4) != 0 && response.at(4) != 1))
				{
					//Ignore => mosquitto does not send SUBACK
				}
				else break;
			}
			catch(BaseLib::SocketClosedException&)
			{
				_out.printError("Error: Socket closed while sending packet.");
				break;
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				_socket->close();
				break;
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

void Mqtt::reconnect()
{
	if(!_started) return;
	_reconnectThreadMutex.lock();
	try
	{
		if(_reconnecting)
		{
			_reconnectThreadMutex.unlock();
			return;
		}
		_reconnecting = true;
		if(_reconnectThread.joinable())	_reconnectThread.join();
		_reconnectThread = std::thread(&Mqtt::connect, this);
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
    _reconnectThreadMutex.unlock();
}

void Mqtt::connect()
{
	_reconnecting = true;
	_connectMutex.lock();
	for(int32_t i = 0; i < 5; i++)
	{
		try
		{
			if(_socket->connected() || !_started)
			{
				_connectMutex.unlock();
				_reconnecting = false;
				return;
			}
			_connected = false;
			_socket->setReadTimeout(5000000);
			_socket->open();
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
			if(!_settings.username().empty()) payload.at(7) |= 0x80;
			if(!_settings.password().empty()) payload.at(7) |= 0x40;
			payload.push_back(0); //Keep alive MSB (in seconds)
			payload.push_back(0x3C); //Keep alive LSB
			std::string temp = _settings.clientName();
			if(temp.empty()) temp = "Homegear";
			payload.push_back(temp.size() >> 8);
			payload.push_back(temp.size() & 0xFF);
			payload.insert(payload.end(), temp.begin(), temp.end());
			if(!_settings.username().empty())
			{
				temp = _settings.username();
				payload.push_back(temp.size() >> 8);
				payload.push_back(temp.size() & 0xFF);
				payload.insert(payload.end(), temp.begin(), temp.end());
			}
			if(!_settings.password().empty())
			{
				temp = _settings.password();
				payload.push_back(temp.size() >> 8);
				payload.push_back(temp.size() & 0xFF);
				payload.insert(payload.end(), temp.begin(), temp.end());
			}
			std::vector<char> lengthBytes = getLengthBytes(payload.size());
			std::vector<char> connectPacket;
			connectPacket.reserve(1 + lengthBytes.size() + payload.size());
			connectPacket.push_back(0x10); //Control packet type
			connectPacket.insert(connectPacket.end(), lengthBytes.begin(), lengthBytes.end());
			connectPacket.insert(connectPacket.end(), payload.begin(), payload.end());
			std::vector<char> response(10);
			getResponseByType(connectPacket, response, 0x20, false);
			bool retry = false;
			if(response.size() != 4)
			{
				if(response.size() == 0) {}
				else if(response.size() != 4) _out.printError("Error: CONNACK packet has wrong size.");
				else if(response[0] != 0x20 || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
				else if(response[3] != 1) printConnectionError(response[3]);
				retry = true;
			}
			else
			{
				_out.printInfo("Info: Successfully connected to MQTT server using protocol version 4.");
				_connected = true;
				_connectMutex.unlock();
				subscribe("homegear/" + _settings.homegearId() + "/rpc/#");
				subscribe("homegear/" + _settings.homegearId() + "/value/#");
				subscribe("homegear/" + _settings.homegearId() + "/config/#");
				_reconnecting = false;
				return;
			}
			if(retry && _started)
			{
				_socket->open();
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
				if(!_settings.username().empty()) payload.at(7) |= 0x80;
				if(!_settings.password().empty()) payload.at(7) |= 0x40;
				payload.push_back(0); //Keep alive MSB (in seconds)
				payload.push_back(0x3C); //Keep alive LSB
				temp = _settings.clientName();
				if(temp.empty()) temp = "Homegear";
				payload.push_back(temp.size() >> 8);
				payload.push_back(temp.size() & 0xFF);
				payload.insert(payload.end(), temp.begin(), temp.end());
				if(!_settings.username().empty())
				{
					temp = _settings.username();
					payload.push_back(temp.size() >> 8);
					payload.push_back(temp.size() & 0xFF);
					payload.insert(payload.end(), temp.begin(), temp.end());
				}
				if(!_settings.password().empty())
				{
					temp = _settings.password();
					payload.push_back(temp.size() >> 8);
					payload.push_back(temp.size() & 0xFF);
					payload.insert(payload.end(), temp.begin(), temp.end());
				}
				std::vector<char> lengthBytes = getLengthBytes(payload.size());
				std::vector<char> connectPacket;
				connectPacket.reserve(1 + lengthBytes.size() + payload.size());
				connectPacket.push_back(0x10); //Control packet type
				connectPacket.insert(connectPacket.end(), lengthBytes.begin(), lengthBytes.end());
				connectPacket.insert(connectPacket.end(), payload.begin(), payload.end());
				getResponseByType(connectPacket, response, 0x20);
				if(response.size() != 4)
				{
					if(response.size() == 0) _out.printError("Error: Connection to MQTT server with protocol version 3 failed.");
					else if(response.size() != 4) _out.printError("Error: CONNACK packet has wrong size.");
					else if(response[0] != 0x20 || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
					else printConnectionError(response[3]);
				}
				else
				{
					_out.printInfo("Info: Successfully connected to MQTT server using protocol version 3.");
					_connected = true;
					_connectMutex.unlock();
					subscribe("homegear/" + _settings.homegearId() + "/rpc/#");
					subscribe("homegear/" + _settings.homegearId() + "/value/#");
					subscribe("homegear/" + _settings.homegearId() + "/config/#");
					_reconnecting = false;
					return;
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
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
	_connectMutex.unlock();
	_reconnecting = false;
}

void Mqtt::disconnect()
{
	try
	{
		_connected = false;
		std::vector<char> disconnect = { (char)0xE0, 0 };
		if(_socket->connected()) _socket->proofwrite(disconnect);
		_socket->close();
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

void Mqtt::messageReceived(std::vector<char>& message)
{
	try
	{
		BaseLib::PVariable result = _jsonDecoder->decode(message);
		std::string methodName;
		std::string clientId;
		int32_t messageId = 0;
		BaseLib::PVariable parameters;
		if(result->type == BaseLib::VariableType::tStruct)
		{
			if(result->structValue->find("method") == result->structValue->end())
			{
				_out.printWarning("Warning: Could not decode JSON RPC packet from MQTT payload.");
				return;
			}
			methodName = result->structValue->at("method")->stringValue;
			if(result->structValue->find("id") != result->structValue->end()) messageId = result->structValue->at("id")->integerValue;
			if(result->structValue->find("clientid") != result->structValue->end()) clientId = result->structValue->at("clientid")->stringValue;
			if(result->structValue->find("params") != result->structValue->end()) parameters = result->structValue->at("params");
			else parameters.reset(new BaseLib::Variable());
		}
		else
		{
			_out.printWarning("Warning: Could not decode MQTT RPC packet.");
			return;
		}
		GD::out.printInfo("Info: MQTT RPC call received. Method: " + methodName);
		BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
		std::shared_ptr<std::pair<std::string, std::vector<char>>> responseData(new std::pair<std::string, std::vector<char>>());
		_jsonEncoder->encodeMQTTResponse(methodName, response, messageId, responseData->second);
		responseData->first = (!clientId.empty()) ? clientId + "/rpcResult" : "rpcResult";
		queueMessage(responseData);
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

void Mqtt::queueMessage(std::shared_ptr<std::pair<std::string, std::vector<char>>>& message)
{
	try
	{
		if(!_started || !message) return;
		std::shared_ptr<BaseLib::IQueueEntry> entry(new QueueEntry(message));
		if(!enqueue(0, entry)) _out.printError("Error: Too many packets are queued to be processed. Your packet processing is too slow. Dropping packet.");
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

void Mqtt::publish(const std::string& topic, const std::vector<char>& data)
{
	try
	{
		if(data.empty() || !_started) return;
		std::string fullTopic = "homegear/" + _settings.homegearId() + "/" + topic;
		std::vector<char> packet;
		std::vector<char> payload;
		payload.reserve(fullTopic.size() + 2 + 2 + data.size());
		payload.push_back(fullTopic.size() >> 8);
		payload.push_back(fullTopic.size() & 0xFF);
		payload.insert(payload.end(), fullTopic.begin(), fullTopic.end());
		int16_t id = 0;
		while(id == 0) id = _packetId++;
		payload.push_back(id >> 8);
		payload.push_back(id & 0xFF);
		payload.insert(payload.end(), data.begin(), data.end());
		std::vector<char> lengthBytes = getLengthBytes(payload.size());
		packet.reserve(1 + lengthBytes.size() + payload.size());
		packet.push_back(0x32);
		packet.insert(packet.end(), lengthBytes.begin(), lengthBytes.end());
		packet.insert(packet.end(), payload.begin(), payload.end());
		int32_t j = 0;
		std::vector<char> response(7);
		if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Publishing topic " + fullTopic);
		for(int32_t i = 0; i < 25; i++)
		{
			if(!_socket->connected()) connect();
			if(!_started) break;
			if(i == 1) packet[0] |= 8;
			getResponse(packet, response, 0x40, id, true);
			if(response.empty())
			{
				//_socket->close();
				//reconnect();
				if(i >= 5) _out.printWarning("Warning: No PUBACK received.");
			}
			else return;

			j = 0;
			while(_started && j < 5)
			{

				if(i < 5)
				{
					j += 5;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				else
				{
					j++;
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
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

void Mqtt::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry || !queueEntry->message) return;
		publish(queueEntry->message->first, queueEntry->message->second);
	}
	catch(const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
