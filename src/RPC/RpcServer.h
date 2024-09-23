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

#ifndef RPCSERVER_H_
#define RPCSERVER_H_

#include "RpcMethods/RPCMethods.h"
#include "Auth.h"
#include "RestServer.h"
#include "../WebServer/WebServer.h"
#include <homegear-base/BaseLib.h>

#include <thread>
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <memory>

#include <gnutls/gnutls.h>

namespace Homegear {

namespace Rpc {

class SocketBindException : public C1Net::Exception {
 public:
  explicit SocketBindException(const std::string &message) : C1Net::Exception(message) {}
};

class RpcServer {
 public:
  class Client : public BaseLib::RpcClientInfo {
   public:
    bool webSocketClient = false;
    bool webSocketAuthorized = false;
    bool nodeClient = false;
    std::thread readThread;
    std::shared_ptr<Auth> auth;

    Client();

    virtual ~Client();
  };

  struct PacketType {
    enum Enum {
      xmlRequest,
      xmlResponse,
      binaryRequest,
      binaryResponse,
      jsonRequest,
      jsonResponse,
      webSocketRequest,
      webSocketResponse
    };
  };

  RpcServer();

  virtual ~RpcServer();

  void dispose();

  const std::vector<BaseLib::PRpcClientInfo> getClientInfo();

  BaseLib::Rpc::PServerInfo getInfo() { return _info; }

  bool lifetick();

  bool isRunning() { return !_stopped; }

  static BaseLib::PFileDescriptor bindAndReturnSocket(BaseLib::FileDescriptorManager &fileDescriptorManager, const std::string &address, const std::string &port, uint32_t connectionBacklogSize, std::string &listenAddress, int32_t &listenPort);

  void start(BaseLib::Rpc::PServerInfo &settings);

  void stop();

  uint32_t connectionCount();

  std::shared_ptr<std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>> getMethods() { return _rpcMethods; };

  bool methodExists(const BaseLib::PRpcClientInfo &clientInfo, std::string &methodName);

  BaseLib::PVariable callMethod(const BaseLib::PRpcClientInfo &clientInfo,
                                const std::string &methodName,
                                BaseLib::PVariable &parameters);

  BaseLib::PEventHandler addWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink *eventHandler);

  void removeWebserverEventHandler(BaseLib::PEventHandler eventHandler);
 protected:
 private:
  BaseLib::Output _out;
  BaseLib::Rpc::PServerInfo _info;
  gnutls_certificate_credentials_t _x509Cred = nullptr;
  gnutls_priority_t _tlsPriorityCache = nullptr;
  int32_t _threadPolicy = SCHED_OTHER;
  int32_t _threadPriority = 0;
  std::atomic_bool _stopServer;
  std::atomic_bool _stopped;
  std::thread _mainThread;
  static constexpr int32_t _backlog = 100;
  std::mutex _garbageCollectionMutex;
  int64_t _lastGargabeCollection = 0;
  std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
  std::mutex _stateMutex;
  std::map<int32_t, std::shared_ptr<Client>> _clients;
  std::shared_ptr<std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>> _rpcMethods;
  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoderAnsi;
  std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;
  std::unique_ptr<BaseLib::Rpc::XmlrpcDecoder> _xmlRpcDecoder;
  std::unique_ptr<BaseLib::Rpc::XmlrpcEncoder> _xmlRpcEncoder;
  std::unique_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;
  std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
  std::unique_ptr<WebServer::WebServer> _webServer;
  std::unique_ptr<RestServer> _restServer;
  std::pair<std::atomic<int64_t>, std::atomic<bool>> lifetick_1_;
  std::pair<std::atomic<int64_t>, std::atomic<bool>> lifetick_2_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _cloudUserMap;

  void collectGarbage();

  void getSocketDescriptor();

  C1Net::PSocket getClientSocketDescriptor(std::string &address, int32_t &port);

  void getSSLSocketDescriptor(std::shared_ptr<Client>);

  void mainThread();

  void readClient(std::shared_ptr<Client> client);

  void sendRPCResponseToClient(std::shared_ptr<Client> client,
                               const BaseLib::PVariable &variable,
                               int32_t messageId,
                               PacketType::Enum packetType,
                               bool keepAlive);

  void sendRPCResponseToClient(std::shared_ptr<Client> client, std::vector<char> &data, bool keepAlive);

  void packetReceived(std::shared_ptr<Client> client,
                      const std::vector<char> &packet,
                      PacketType::Enum packetType,
                      bool keepAlive);

  void handleConnectionUpgrade(std::shared_ptr<Client> client, BaseLib::Http &http);

  void analyzeRPC(const std::shared_ptr<Client> &client,
                  const std::vector<char> &packet,
                  PacketType::Enum packetType,
                  bool keepAlive);

  void analyzeRPCResponse(const std::shared_ptr<Client> &client,
                          const std::vector<char> &packet,
                          PacketType::Enum packetType,
                          bool keepAlive);

  void callMethod(const std::shared_ptr<Client> &client,
                  const std::string &methodName,
                  std::shared_ptr<std::vector<BaseLib::PVariable>> parameters,
                  int32_t messageId,
                  PacketType::Enum responseType,
                  bool keepAlive);

  static std::string getHttpResponseHeader(const std::string &contentType, uint32_t contentLength, bool closeConnection);

  void closeClientConnection(std::shared_ptr<Client> client);

  bool clientValid(std::shared_ptr<Client> &client);
};

}

}

#endif
