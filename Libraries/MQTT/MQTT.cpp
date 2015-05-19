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

#include "MQTT.h"
#include "../GD/GD.h"
#include "../../Version.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"

int MQTTMessageArrived(void* context, char* topicName, int topicLength, MQTTClient_message* message)
{
    std::vector<char> payload((char*)message->payload, (char*)message->payload + message->payloadlen);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    GD::mqtt->messageReceived(payload);
    return 1;
}

MQTT::MQTT()
{
	try
	{

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

MQTT::~MQTT()
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

void MQTT::loadSettings()
{
	_settings.load(GD::bl->settings.mqttSettingsPath());
}

void MQTT::start()
{
	try
	{
		if(_started) return;
		_out.init(GD::bl.get());
		_out.setPrefix("MQTT Client: ");
		_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));
		_jsonDecoder = std::unique_ptr<BaseLib::RPC::JsonDecoder>(new BaseLib::RPC::JsonDecoder(GD::bl.get()));
		connect();
		_stopMessageProcessingThread = false;
		_messageProcessingMessageAvailable = false;
		_messageBufferHead = 0;
		_messageBufferTail = 0;
		_messageProcessingThread = std::thread(&MQTT::processMessages, this);
		BaseLib::Threads::setThreadPriority(GD::bl.get(), _messageProcessingThread.native_handle(), GD::bl->settings.rpcClientThreadPriority(), GD::bl->settings.rpcClientThreadPolicy());
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

void MQTT::stop()
{
	try
	{
		_started = false;
		_stopMessageProcessingThread = true;
		_messageProcessingMessageAvailable = true;
		_messageProcessingConditionVariable.notify_one();
		if(_messageProcessingThread.joinable()) _messageProcessingThread.join();
		disconnect();
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

void MQTT::connect()
{
	try
	{
		if(_started || _client) return;
		_started = true;
		_connectionOptions = new MQTTClient_connectOptions(MQTTClient_connectOptions_initializer);
		if(!_settings.username().empty())
		{
			((MQTTClient_connectOptions*)_connectionOptions)->username = _settings.username().c_str();
			((MQTTClient_connectOptions*)_connectionOptions)->password = _settings.password().c_str();
		}
		if(_settings.enableSSL())
		{
			_out.printInfo("Info: Enabling SSL for MQTT.");
#ifndef OPENSSL
			_out.printError("Error: MQTT library is compiled without OpenSSL support. Can't enable SSL. Aborting.");
			disconnect();
			return;
#endif
			if(_settings.caFile().empty())
			{
				_out.printError("Error: SSL is enabled but \"caFile\" is not set.");
				disconnect();
				return;
			}
			if(!BaseLib::HelperFunctions::fileExists(_settings.caFile()))
			{
				_out.printError("Error: CA certificate file does not exist or can't be opened.");
				disconnect();
				return;
			}
			if(_settings.certPath().empty())
			{
				_out.printError("Error: SSL is enabled but \"certPath\" is not set.");
				disconnect();
				return;
			}
			if(!BaseLib::HelperFunctions::fileExists(_settings.certPath()))
			{
				_out.printError("Error: Client certificate file does not exist or can't be opened.");
				disconnect();
				return;
			}
			if(_settings.keyPath().empty())
			{
				_out.printError("Error: SSL is enabled but \"keyPath\" is not set.");
				disconnect();
				return;
			}
			if(!BaseLib::HelperFunctions::fileExists(_settings.keyPath()))
			{
				_out.printError("Error: Client key file does not exist or can't be opened.");
				disconnect();
				return;
			}

			_sslOptions = new MQTTClient_SSLOptions({ {'M', 'Q', 'T', 'S'}, 0, _settings.caFile().c_str(), _settings.certPath().c_str(), _settings.keyPath().c_str(), nullptr, nullptr, _settings.verifyCertificate() });
			((MQTTClient_connectOptions*)_connectionOptions)->ssl = (MQTTClient_SSLOptions*)_sslOptions;
		}
		MQTTClient_create(&_client, std::string((_settings.enableSSL() ? "ssl://" : "tcp://") + _settings.brokerHostname() + ":" + _settings.brokerPort()).c_str(), _settings.clientName().c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
		((MQTTClient_connectOptions*)_connectionOptions)->keepAliveInterval = 10;
		((MQTTClient_connectOptions*)_connectionOptions)->cleansession = 1;
		MQTTClient_setCallbacks(_client, NULL, NULL, MQTTMessageArrived, NULL);
		int32_t result = MQTTClient_connect(_client, (MQTTClient_connectOptions*)_connectionOptions);
		if (result != MQTTCLIENT_SUCCESS)
		{
			disconnect();
			std::string reason;
			//see http://www.eclipse.org/paho/files/mqttdoc/Cclient/_m_q_t_t_client_8h.html#aaa8ae61cd65c9dc0846df10122d7bd4e
			if(result == 1) reason = "Connection refused: Unacceptable protocol version";
			else if(result == 2) reason = "Connection refused: Identifier rejected";
			else if(result == 3) reason = "Connection refused: Server unavailable";
			else if(result == 4) reason = "Connection refused: Bad user name or password";
			else if(result == 5) reason = "Connection refused: Not authorized";
			else reason = "Unknown reason. Check broker log for more details. A few common reasons: The broker is not running, the broker is not reachable (wrong IP address, wrong port, firewall, ...), the broker requires SSL and SSL is not enabled in Homegear's mqtt.conf.";
			_out.printError("Error: Failed to connect to message broker: " + reason);
			return;
		}
		_out.printInfo("Info: Successfully connected to message broker.");
		MQTTClient_subscribe(_client, std::string("homegear/" + _settings.homegearId() + "/rpc/#").c_str(), 1);
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

void MQTT::disconnect()
{
	try
	{
		_started = false;
		if(_client)
		{
			MQTTClient_disconnect(_client, 10000);
			MQTTClient_destroy(&_client);
			_client = nullptr;
		}
		if(_connectionOptions)
		{
			delete((MQTTClient_connectOptions*)_connectionOptions);
			_connectionOptions = nullptr;
		}
		if(_sslOptions)
		{
			delete((MQTTClient_SSLOptions*)_sslOptions);
			_sslOptions = nullptr;
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

void MQTT::messageReceived(std::vector<char>& message)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> result = _jsonDecoder->decode(message);
		std::string methodName;
		std::string clientId;
		int32_t messageId = 0;
		std::shared_ptr<BaseLib::RPC::Variable> parameters;
		if(result->type == BaseLib::RPC::VariableType::rpcStruct)
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
			else parameters.reset(new BaseLib::RPC::Variable());
		}
		else
		{
			_out.printWarning("Warning: Could not decode MQTT RPC packet.");
			return;
		}
		GD::out.printInfo("Info: MQTT RPC call received. Method: " + methodName);
		std::shared_ptr<BaseLib::RPC::Variable> response = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
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

void MQTT::queueMessage(std::shared_ptr<std::pair<std::string, std::vector<char>>>& message)
{
	try
	{
		if(!_started) return;
		_messageBufferMutex.lock();
		int32_t tempHead = _messageBufferHead + 1;
		if(tempHead >= _messageBufferSize) tempHead = 0;
		if(tempHead == _messageBufferTail)
		{
			_out.printError("Error: More than " + std::to_string(_messageBufferSize) + " packets are queued to be processed. Your packet processing is too slow. Dropping packet.");
			_messageBufferMutex.unlock();
			return;
		}

		_messageBuffer[_messageBufferHead] = message;
		_messageBufferHead++;
		if(_messageBufferHead >= _messageBufferSize)
		{
			_messageBufferHead = 0;
		}
		_messageProcessingMessageAvailable = true;
		_messageBufferMutex.unlock();

		_messageProcessingConditionVariable.notify_one();
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

void MQTT::publish(const std::string& topic, const std::vector<char>& data)
{
	try
	{
		std::string fullTopic = "homegear/" + _settings.homegearId() + "/" + topic;
		MQTTClient_deliveryToken token;
		MQTTClient_message message = MQTTClient_message_initializer;
		message.payload = (void*)&data.at(0);
		message.payloadlen = data.size();
		message.qos = 1;
		message.retained = 0;
		for(int32_t i = 0; i < 3; i++)
		{
			if(_client == nullptr) connect();
			if(_client == nullptr)
			{
				GD::out.printError("Could not publish message, because we are not connected to a message broker. Topic: " + fullTopic + " Data: " + std::string(&data.at(0), data.size()));
				return;
			}
			if(GD::bl->debugLevel >= 5) _out.printDebug("Publishing message with topic " + fullTopic + ": " + std::string(&data.at(0), data.size()));
			MQTTClient_publishMessage(_client, fullTopic.c_str(), &message, &token);
			if(MQTTClient_waitForCompletion(_client, token, 5000) != MQTTCLIENT_SUCCESS)
			{
				if(i == 2) GD::out.printError("Could not publish message with topic " + fullTopic + " and data: " + std::string(&data.at(0), data.size()));
				disconnect();
				continue;
			}
			break;
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

void MQTT::processMessages()
{
	while(!_stopMessageProcessingThread)
	{
		std::unique_lock<std::mutex> lock(_messageProcessingThreadMutex);
		try
		{
			_messageBufferMutex.lock();
			if(_messageBufferHead == _messageBufferTail) //Only lock, when there is really no packet to process. This check is necessary, because the check of the while loop condition is outside of the mutex
			{
				_messageBufferMutex.unlock();
				_messageProcessingConditionVariable.wait(lock, [&]{ return _messageProcessingMessageAvailable; });
			}
			else _messageBufferMutex.unlock();
			if(_stopMessageProcessingThread)
			{
				lock.unlock();
				return;
			}

			while(_messageBufferHead != _messageBufferTail)
			{
				_messageBufferMutex.lock();
				std::shared_ptr<std::pair<std::string, std::vector<char>>> message = _messageBuffer[_messageBufferTail];
				_messageBuffer[_messageBufferTail].reset();
				_messageBufferTail++;
				if(_messageBufferTail >= _messageBufferSize) _messageBufferTail = 0;
				if(_messageBufferHead == _messageBufferTail) _messageProcessingMessageAvailable = false; //Set here, because otherwise it might be set to "true" in publish and then set to false again after the while loop
				_messageBufferMutex.unlock();
				if(message) publish(message->first, message->second);
			}
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
		lock.unlock();
	}
}
