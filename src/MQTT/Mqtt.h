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

#ifndef MQTT_H_
#define MQTT_H_

#include <homegear-base/BaseLib.h>
#include "MqttSettings.h"

#define MQTT_PACKET_CONNECT 0x10
#define MQTT_PACKET_CONNACK 0x20
#define MQTT_PACKET_PUBLISH 0x30
#define MQTT_PACKET_PUBACK  0x40
#define MQTT_PACKET_PUBREC  0x50
#define MQTT_PACKET_PUBREL  0x62
#define MQTT_PACKET_PUBCOMP 0x70
#define MQTT_PACKET_SUBSCRIBE 0x82
#define MQTT_PACKET_SUBACK  0x90
#define MQTT_PACKET_CONNACK 0x20
#define MQTT_PACKET_UNSUB   0xA2
#define MQTT_PACKET_UNSUBACK 0xB0
#define MQTT_PACKET_PINGREQ  0xC0
#define MQTT_PACKET_PINGRESP 0xD0
#define MQTT_PACKET_DISCONN  0xE0

namespace Homegear {

class Mqtt : public BaseLib::IQueue {
 public:
  struct MqttMessage {
    std::string topic;
    std::vector<char> message;
    bool retain = true;
  };

  Mqtt();

  virtual ~Mqtt();

  bool enabled() { return _settings.enabled(); }

  void start();

  void stop();

  void loadSettings();

  /**
   * Queues a message for publishing to the MQTT broker.
   *
   * @param message The message to queue.
   */
  void queueMessage(std::shared_ptr<MqttMessage> &message);

  /**
   * Queues a generic message for publishing to the MQTT broker.
   *
   * @param topic The MQTT topic.
   * @param payload The MQTT payload.
   */
  void queueMessage(const std::string &topic, const std::string &payload);

  /**
   * Queues a peer message for publishing to the MQTT broker.
   *
   * @param source The source of the event, either the ID of the client calling "setValue()" or "device".
   * @param peerId The id of the peer to queue the message for.
   * @param channel The channel of the peer to queue the message for.
   * @param key The name of the variable.
   * @param value The value of the variable.
   */
  void queueMessage(const std::string &source, uint64_t peerId, int32_t channel, const std::string &key, const BaseLib::PVariable &value);

  /**
   * Queues a peer message containing multiple variables for publishing to the MQTT broker.
   *
   * @param source The source of the event, either the ID of the client calling "setValue()" or "device".
   * @param peerId The id of the peer to queue the message for.
   * @param channel The channel of the peer to queue the message for.
   * @param keys The names of the variables.
   * @param values The values of the variables.
   */
  void queueMessage(const std::string &source, uint64_t peerId, int32_t channel, const std::vector<std::string> &keys, const std::vector<BaseLib::PVariable> &values);

 private:
  class QueueEntrySend : public BaseLib::IQueueEntry {
   public:
    QueueEntrySend() = default;

    explicit QueueEntrySend(std::shared_ptr<MqttMessage> &message) { this->message = message; }

    ~QueueEntrySend() override = default;

    std::shared_ptr<MqttMessage> message;
  };

  class QueueEntryReceived : public BaseLib::IQueueEntry {
   public:
    QueueEntryReceived() = default;

    explicit QueueEntryReceived(std::vector<char> &data) { this->data = data; }

    ~QueueEntryReceived() override = default;

    std::vector<char> data;
  };

  class Request {
   public:
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool mutexReady = false;
    std::vector<char> response;

    uint8_t getResponseControlByte() const { return _responseControlByte; }

    explicit Request(uint8_t responseControlByte) { _responseControlByte = responseControlByte; };

    ~Request() = default;;
   private:
    uint8_t _responseControlByte;
  };

  struct RequestByType {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool mutexReady = false;
    std::vector<char> response;
  };

  BaseLib::Output _out;
  MqttSettings _settings;
  uint32_t _prefixParts = 0;
  std::string _clientName;
  std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
  std::unique_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;
  std::unique_ptr<BaseLib::TcpSocket> _socket;
  std::thread _pingThread;
  std::thread _listenThread;
  std::atomic_bool _reconnecting;
  std::mutex _reconnectThreadMutex;
  std::thread _reconnectThread;
  std::mutex _connectMutex;
  std::atomic_bool _started;
  std::atomic_bool _connected;
  std::atomic<int16_t> _packetId;
  std::mutex _getResponseMutex;
  std::mutex _requestsMutex;
  std::map<int16_t, std::shared_ptr<Request>> _requests;
  std::mutex _requestsByTypeMutex;
  std::map<uint8_t, std::shared_ptr<RequestByType>> _requestsByType;
  std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;

  Mqtt(const Mqtt &);

  Mqtt &operator=(const Mqtt &);

  void getClientName();

  void connect();

  void reconnect();

  void disconnect();

  void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) override;

  std::vector<char> getLengthBytes(uint32_t length);

  uint32_t getLength(std::vector<char> packet, uint32_t &lengthBytes);

  void printConnectionError(char resultCode);

  /**
   * Publishes data to the MQTT broker.
   *
   * @param topic The topic without Homegear prefix ("/homegear/UNIQUEID/") and without starting "/" (e.g. c/d).
   * @param data The data to publish.
   */
  void publish(const std::string &topic, const std::vector<char> &data, bool retain);

  void ping();

  void getResponseByType(const std::vector<char> &packet, std::vector<char> &responseBuffer, uint8_t responseType, bool errors = true);

  void getResponse(const std::vector<char> &packet, std::vector<char> &responseBuffer, uint8_t responseType, int16_t packetId, bool errors = true);

  void listen();

  void processData(std::vector<char> &data);

  void processPublish(std::vector<char> &data);

  void subscribe(std::string topic);

  void send(const std::vector<char> &data);
};

}

#endif
