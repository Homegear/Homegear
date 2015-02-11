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
		_out.init(GD::bl.get());
		_out.setPrefix("MQTT Client: ");
		connect();
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
		disconnect();
		_connectionOptions = new MQTTClient_connectOptions(MQTTClient_connectOptions_initializer);
		MQTTClient_create(&_client, std::string("tcp://" + _settings.brokerHostname() + ":" + _settings.brokerPort()).c_str(), _settings.clientName().c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
		((MQTTClient_connectOptions*)_connectionOptions)->keepAliveInterval = 20;
		((MQTTClient_connectOptions*)_connectionOptions)->cleansession = 1;
		if ((MQTTClient_connect(_client, (MQTTClient_connectOptions*)_connectionOptions)) != MQTTCLIENT_SUCCESS)
		{
			disconnect();
			_out.printError("Failed to connect to message broker.");
		}
		_out.printInfo("Successfully connected to message broker.");
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
		MQTTClient_disconnect(_client, 10000);
		MQTTClient_destroy(&_client);
		_client = nullptr;
		if(_connectionOptions) free(_connectionOptions);
		_connectionOptions = nullptr;
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
		topic = "homegear/" + _settings.homegearId() + "/" + topic;
		MQTTClient_deliveryToken token;
		MQTTClient_message message = MQTTClient_message_initializer;
		message.payload = &data.at(0);
		message.payloadlen = data.size();
		message.qos = 1;
		message.retained = 0;
		_sendMutex.lock();
		for(int32_t i = 0; i < 3; i++)
		{
			if(_client == nullptr) connect();
			if(_client == nullptr)
			{
				_sendMutex.unlock();
				GD::out.printError("Could not publish message, because we are not connected to a message broker. Topic: " + topic + " Data: " + std::string(&data.at(0), data.size()));
				return;
			}
			if(GD::bl->debugLevel >= 5) _out.printDebug("Publishing message: " + std::string(&data.at(0), data.size()));
			MQTTClient_publishMessage(_client, topic.c_str(), &message, &token);
			if(MQTTClient_waitForCompletion(_client, token, 5000) != MQTTCLIENT_SUCCESS)
			{
				if(i == 2) GD::out.printError("Could not publish message with topic " + topic + " and data: " + std::string(&data.at(0), data.size()));
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
	_sendMutex.unlock();
}
