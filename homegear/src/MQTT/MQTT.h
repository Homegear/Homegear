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

#ifndef MQTT_H_
#define MQTT_H_

#include "homegear-base/BaseLib.h"
#include "MQTTSettings.h"

class MQTT
{
public:
	MQTT();
	virtual ~MQTT();

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
	BaseLib::Output _out;
	MQTTSettings _settings;
	std::unique_ptr<BaseLib::RPC::JsonEncoder> _jsonEncoder;
	std::unique_ptr<BaseLib::RPC::JsonDecoder> _jsonDecoder;

	static const int32_t _messageBufferSize = 1000;
	std::mutex _messageBufferMutex;
	int32_t _messageBufferHead = 0;
	int32_t _messageBufferTail = 0;
	std::shared_ptr<std::pair<std::string, std::vector<char>>> _messageBuffer[_messageBufferSize];
	std::mutex _messageProcessingThreadMutex;
	std::thread _messageProcessingThread;
	bool _messageProcessingMessageAvailable = false;
	std::condition_variable _messageProcessingConditionVariable;
	bool _stopMessageProcessingThread = false;
	bool _started = false;

	void* _connectionOptions = nullptr;
	void* _sslOptions = nullptr;
	void* _client = nullptr;

	MQTT(const MQTT&);
	MQTT& operator=(const MQTT&);
	void connect();
	void disconnect();
	void processMessages();

	/**
	 * Publishes data to the MQTT broker.
	 *
	 * @param topic The topic without Homegear prefix ("/homegear/UNIQUEID/") and without starting "/" (e.g. c/d).
	 * @param data The data to publish.
	 */
	void publish(const std::string& topic, const std::vector<char>& data);
};

#endif
