/* Copyright 2013-2016 Sathya Laufer
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

#ifndef MQTT_H_
#define MQTT_H_

#include "homegear-base/BaseLib.h"

#include "MqttSettings.h"

class Mqtt : public BaseLib::IQueue
{
public:
	Mqtt();
	virtual ~Mqtt();

	bool enabled() { return _settings.enabled(); }
	void start();
	void stop();
	void loadSettings();

	/**
	 * Queues a message for publishing to the MQTT broker.
	 *
	 * @param message The message to queue. The first part of the pair is the topic, the second part the data.
	 */
	void queueMessage(std::shared_ptr<std::pair<std::string, std::vector<char>>>& message);

	/**
	 * Processes a message received from a message broker.
	 *
	 * @param payload The content of the message.
	 */
	void messageReceived(std::vector<char>& payload);
private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(std::shared_ptr<std::pair<std::string, std::vector<char>>>& message) { this->message = message; }
		virtual ~QueueEntry() {}

		std::shared_ptr<std::pair<std::string, std::vector<char>>> message;
	};

	class Request
	{
	public:
		std::mutex mutex;
		std::condition_variable conditionVariable;
		bool mutexReady = false;
		std::vector<char> response;
		uint8_t getResponseControlByte() { return _responseControlByte; }

		Request(uint8_t responseControlByte) { _responseControlByte = responseControlByte; };
		virtual ~Request() {};
	private:
		uint8_t _responseControlByte;
	};

	class RequestByType
	{
	public:
		std::mutex mutex;
		std::condition_variable conditionVariable;
		bool mutexReady = false;
		std::vector<char> response;

		RequestByType() {};
		virtual ~RequestByType() {};
	};

	BaseLib::Output _out;
	MqttSettings _settings;
	std::unique_ptr<BaseLib::RPC::JsonEncoder> _jsonEncoder;
	std::unique_ptr<BaseLib::RPC::JsonDecoder> _jsonDecoder;
	std::unique_ptr<BaseLib::SocketOperations> _socket;
	std::thread _pingThread;
	std::thread _listenThread;
	bool _reconnecting = false;
	std::mutex _reconnectThreadMutex;
	std::thread _reconnectThread;
	std::mutex _connectMutex;
	bool _started = false;
	bool _connected = false;
	int16_t _packetId = 1;
	std::mutex _requestsMutex;
	std::map<int16_t, std::shared_ptr<Request>> _requests;
	std::mutex _requestsByTypeMutex;
	std::map<uint8_t, std::shared_ptr<RequestByType>> _requestsByType;

	Mqtt(const Mqtt&);
	Mqtt& operator=(const Mqtt&);
	void connect();
	void reconnect();
	void disconnect();
	void processMessages();
	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
	std::vector<char> getLengthBytes(uint32_t length);
	uint32_t getLength(std::vector<char> packet, uint32_t& lengthBytes);
	void printConnectionError(char resultCode);

	/**
	 * Publishes data to the MQTT broker.
	 *
	 * @param topic The topic without Homegear prefix ("/homegear/UNIQUEID/") and without starting "/" (e.g. c/d).
	 * @param data The data to publish.
	 */
	void publish(const std::string& topic, const std::vector<char>& data);
	void ping();
	void getResponseByType(const std::vector<char>& packet, std::vector<char>& responseBuffer, uint8_t responseType, bool errors = true);
	void getResponse(const std::vector<char>& packet, std::vector<char>& responseBuffer, uint8_t responseType, int16_t packetId, bool errors = true);
	void listen();
	void processData(std::vector<char>& data);
	void processPublish(std::vector<char>& data);
	void subscribe(std::string topic);
	void send(const std::vector<char>& data);
};

#endif
