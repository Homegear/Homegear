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

#ifndef IPCSERVER_H_
#define IPCSERVER_H_

#include "IpcClientData.h"

#include <homegear-base/BaseLib.h>

#include <utility>

namespace Homegear {

class IpcServer : public BaseLib::IQueue {
 public:
  IpcServer();

  virtual ~IpcServer();

  bool lifetick();

  bool start();

  void stop();

  void homegearShuttingDown();

  std::string generateWebSshToken();

  void broadcastEvent(std::string &source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> &variables, BaseLib::PArray &values);

  void broadcastServiceMessage(const BaseLib::PServiceMessage &serviceMessage);

  void broadcastNewDevices(std::vector<uint64_t> &ids, BaseLib::PVariable deviceDescriptions);

  void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);

  void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);

  void broadcastVariableProfileStateChanged(uint64_t profileId, bool state);

  void broadcastUiNotificationCreated(uint64_t uiNotificationId);

  void broadcastUiNotificationRemoved(uint64_t uiNotificationId);

  void broadcastUiNotificationAction(uint64_t uiNotificationId, const std::string& uiNotificationType, uint64_t buttonId);

  bool methodExists(const BaseLib::PRpcClientInfo& clientInfo, std::string &methodName);

  BaseLib::PVariable callProcessRpcMethod(pid_t processId, const BaseLib::PRpcClientInfo &clientInfo, const std::string &methodName, const BaseLib::PArray &parameters);

  BaseLib::PVariable callRpcMethod(const BaseLib::PRpcClientInfo &clientInfo, const std::string &methodName, const BaseLib::PArray &parameters);

  std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> getRpcMethods();

 private:
  class QueueEntry : public BaseLib::IQueueEntry {
   public:
    enum class QueueEntryType {
      defaultType,
      broadcast
    };

    QueueEntry() = default;

    QueueEntry(PIpcClientData &clientData, std::vector<char> &packet) {
      this->clientData = clientData;
      this->packet = packet;
    }

    QueueEntry(PIpcClientData &clientData, const std::string &methodName, BaseLib::PArray &parameters) {
      type = QueueEntryType::broadcast;
      this->clientData = clientData;
      this->methodName = methodName;
      this->parameters = parameters;
    }

    ~QueueEntry() override = default;

    QueueEntryType type = QueueEntryType::defaultType;
    PIpcClientData clientData;

    // {{{ defaultType
    std::vector<char> packet;
    // }}}

    // {{{ broadcast
    std::string methodName;
    BaseLib::PArray parameters;
    // }}}
  };

  BaseLib::Output _out;
  std::string _socketPath;
  std::atomic_bool _shuttingDown{false};
  std::atomic_bool _stopServer{false};
  std::thread _mainThread;
  int32_t _backlog = 100;
  std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
  std::mutex _stateMutex;
  std::map<int32_t, PIpcClientData> _clients;
  std::mutex _clientsByRpcMethodsMutex;
  std::unordered_map<std::string, std::pair<BaseLib::Rpc::PRpcMethod, std::unordered_map<int32_t, PIpcClientData>>> _clientsByRpcMethods;
  int32_t _currentClientId = 0;
  int64_t _lastGargabeCollection = 0;
  std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
  std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> _rpcMethods;
  std::unordered_map<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int64_t threadId, BaseLib::PArray &parameters)>> _localRpcMethods;
  std::mutex _packetIdMutex;
  int32_t _currentPacketId = 0;

  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
  std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

  std::pair<std::atomic<int64_t>, std::atomic<bool>> lifetick_1_;
  std::pair<std::atomic<int64_t>, std::atomic<bool>> lifetick_2_;

  void collectGarbage();

  std::vector<PIpcClientData> GetClientsForRpcMethod(const std::string &rpc_method);

  bool getFileDescriptor(bool deleteOldSocket = false);

  void mainThread();

  void readClient(PIpcClientData &clientData);

  BaseLib::PVariable send(const PIpcClientData &clientData, const std::vector<char> &data);

  BaseLib::PVariable sendRequest(const PIpcClientData &clientData, const std::string &methodName, const BaseLib::PArray &parameters);

  void sendResponse(PIpcClientData &clientData, BaseLib::PVariable &scriptId, BaseLib::PVariable &packetId, BaseLib::PVariable &variable);

  void closeClientConnection(const PIpcClientData& client);

  void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) override;

  // {{{ RPC methods
  BaseLib::PVariable getHomegearPid(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable setPid(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable getClientId(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable getNodeCredentials(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable setNodeCredentials(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable setNodeCredentialTypes(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable registerRpcMethod(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable cliGeneralCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable cliFamilyCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable cliPeerCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable noderedEvent(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);

  BaseLib::PVariable ptyOutput(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters);
  // }}}
};

}

#endif
