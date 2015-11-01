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

uint32_t Mqtt::getResponse(const std::vector<char>& packet, std::vector<char>& responseBuffer, bool reconnect, bool errors)
{
	_getResponseMutex.lock();
	try
	{
		for(int32_t i = 0; i < 5; i++)
		{
			if(!_socket->connected() && reconnect)
			{
				_getResponseMutex.unlock();
				connect();
				_getResponseMutex.lock();
			}
			if(!_socket->connected())
			{
				if(errors) _out.printError("Error: Could not send packet to MQTT server, because we are not connected.");
				_getResponseMutex.unlock();
				return 0;
			}
			try
			{
				_socket->proofwrite(packet);

				uint32_t bytes = _socket->proofread(&responseBuffer.at(0), responseBuffer.size());
				_getResponseMutex.unlock();
				return bytes;
			}
			catch(BaseLib::SocketClosedException&)
			{
				if(errors) _out.printError("Error: Socket closed while sending packet.");
			}
			catch(BaseLib::SocketTimeOutException& ex) { _socket->close(); }
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
	_getResponseMutex.unlock();
	return 0;
}

void Mqtt::ping()
{
	try
	{
		std::vector<char> ping { (char)0xC0, 0 };
		std::vector<char> pong(5);
		uint32_t bytes = 0;
		int32_t i = 0;
		while(_started)
		{
			if(_connected)
			{
				bytes = getResponse(ping, pong);
				if(bytes != 2 || pong[0] != (char)0xD0 || pong[1] != 0)
				{
					if(bytes != 2) _out.printError("Error: PINGRESP packet has wrong size.");
					else _out.printError("Error: PINGRESP packet has wrong content.");
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

void Mqtt::connect()
{
	_connectMutex.lock();
	for(int32_t i = 0; i < 5; i++)
	{
		try
		{
			if(_socket->connected())
			{
				_connectMutex.unlock();
				return;
			}
			_connected = false;
			_socket.reset(new BaseLib::SocketOperations(GD::bl.get(), _settings.brokerHostname(), _settings.brokerPort(), _settings.enableSSL(), _settings.caFile(), _settings.verifyCertificate()));
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
			uint32_t bytes = getResponse(connectPacket, response, false, false);
			bool retry = false;
			if(bytes != 4 || response[0] != 0x20 || response[1] != 0x02 || response[2] != 0 || response[3] != 0)
			{
				if(bytes == 0) {}
				else if(bytes != 4) _out.printError("Error: CONNACK packet has wrong size.");
				else if(response[0] != 0x20 || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
				else if(response[3] != 1) printConnectionError(response[3]);
				retry = true;
			}
			else
			{
				_connected = true;
				_out.printInfo("Info: Successfully connected to MQTT server using protocol version 4.");
				_connectMutex.unlock();
				return;
			}
			if(retry)
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
				bytes = getResponse(connectPacket, response, false);
				if(bytes != 4 || response[0] != 0x20 || response[1] != 0x02 || response[2] != 0 || response[3] != 0)
				{
					if(bytes == 0) _out.printError("Error: Connection to MQTT server with protocol version 3 failed.");
					else if(bytes != 4) _out.printError("Error: CONNACK packet has wrong size.");
					else if(response[0] != 0x20 || response[1] != 0x02 || response[2] != 0) _out.printError("Error: CONNACK has wrong content.");
					else printConnectionError(response[3]);
				}
				else
				{
					_connected = true;
					_out.printInfo("Info: Successfully connected to MQTT server using protocol version 3.");
					_connectMutex.unlock();
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
}

void Mqtt::disconnect()
{
	_getResponseMutex.lock();
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
	_getResponseMutex.unlock();
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
		payload.reserve(topic.size() + 2 + 2 + data.size());
		payload.push_back(topic.size() >> 8);
		payload.push_back(topic.size() & 0xFF);
		payload.insert(payload.end(), topic.begin(), topic.end());
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
		uint32_t bytes = 0;
		int32_t j = 0;
		std::vector<char> response(7);
		for(int32_t i = 0; i < 10; i++)
		{
			if(!_socket->connected()) connect();
			if(!_started) break;
			if(i == 1) packet[0] |= 8;
			bytes = getResponse(packet, response);
			if(bytes != 4 || response[0] != 0x40 || response[1] != 2 || response[2] != (id >> 8) || response[3] != (id & 0xFF))
			{
				if(bytes != 4) _out.printError("Error: PUBACK packet has wrong size.");
				else if(response[2] != (id >> 8) || response[3] != (id & 0xFF)) _out.printError("Error: PUBACK packet has wrong packet id.");
				else _out.printError("Error: PUBACK packet has wrong content.");
				_socket->close();
			}
			else return;

			j = 0;
			while(_started && j < 20)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				j++;
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
