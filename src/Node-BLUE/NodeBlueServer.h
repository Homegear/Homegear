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

#ifndef NODEBLUESERVER_H_
#define NODEBLUESERVER_H_

#include "NodeBlueProcess.h"
#include <homegear-base/BaseLib.h>
#include "FlowInfoServer.h"
#include "NodeManager.h"
#include "NodeBlueCredentials.h"
#include "Node-PINK/Nodepink.h"
#include "Node-PINK/NodepinkWebsocket.h"

#include <queue>
#include <utility>

namespace Homegear::NodeBlue {

class NodeBlueServer : public BaseLib::IQueue {
 public:
  NodeBlueServer();

  ~NodeBlueServer() override;

  std::shared_ptr<Nodepink> getNodered() { return _nodepink; }
  std::shared_ptr<NodepinkWebsocket> getNoderedWebsocket() { return _nodepinkWebsocket; }

  bool lifetick();

  BaseLib::PVariable getLoad();

  bool start();

  void stop();

  void restartFlows();

  bool restartFlowsAsync();

  void stopFlows();

  void homegearShuttingDown();

  void homegearReloading();

  void processKilled(pid_t pid, int exitCode, int signal, bool coreDumped);

  uint32_t flowCount();

  bool isReady();

  void broadcastEvent(std::string &source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> &variables, BaseLib::PArray &values);

  void broadcastFlowVariableEvent(std::string &flowId, std::string &variable, BaseLib::PVariable &value);

  void broadcastGlobalVariableEvent(std::string &variable, BaseLib::PVariable &value);

  void broadcastServiceMessage(const BaseLib::PServiceMessage &serviceMessage);

  void broadcastNewDevices(const std::vector<uint64_t> &ids, const BaseLib::PVariable &deviceDescriptions);

  void broadcastDeleteDevices(const BaseLib::PVariable &deviceInfo);

  void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);

  void broadcastStatus(const std::string &nodeId, const BaseLib::PVariable &status);

  void broadcastError(const std::string &nodeId, int32_t level, const BaseLib::PVariable &error);

  void broadcastVariableProfileStateChanged(uint64_t profileId, bool state);

  void broadcastUiNotificationCreated(uint64_t uiNotificationId);

  void broadcastUiNotificationRemoved(uint64_t uiNotificationId);

  void broadcastUiNotificationAction(uint64_t uiNotificationId, const std::string &uiNotificationType, uint64_t buttonId);

  void broadcastRawPacketEvent(int32_t familyId, const std::string &interfaceId, const BaseLib::PVariable &packet);

  std::string handleGet(std::string &path, BaseLib::Http &http, std::string &responseEncoding, std::string &responseHeader);

  std::string handlePost(std::string &path, BaseLib::Http &http, std::string &responseEncoding, std::string &responseHeader);

  std::string handlePut(std::string &path, BaseLib::Http &http, std::string &responseEncoding, std::string &responseHeader);

  std::string handleDelete(std::string &path, BaseLib::Http &http, std::string &responseEncoding, std::string &responseHeader);

  BaseLib::PVariable getVariablesInRole(BaseLib::PRpcClientInfo clientInfo, uint64_t roleId, uint64_t peerId = 0);

  void nodeLog(const std::string &nodeId, uint32_t logLevel, const std::string &message);

  void nodeOutput(const std::string &nodeId, uint32_t index, const BaseLib::PVariable &message, bool synchronous);

  BaseLib::PVariable addNodesToFlow(const std::string &tab, const std::string &tag, const BaseLib::PVariable &nodes);

  BaseLib::PVariable removeNodesFromFlow(const std::string &tab, const std::string &tag);

  BaseLib::PVariable flowHasTag(const std::string &tab, const std::string &tag);

  BaseLib::PVariable executePhpNodeBaseMethod(BaseLib::PArray &parameters);

  BaseLib::PVariable getNodesWithFixedInputs();

  BaseLib::PVariable getNodeCredentials(const std::string &nodeId);

  BaseLib::PVariable setNodeCredentials(const std::string &nodeId, const BaseLib::PVariable &credentials);

  BaseLib::PVariable getNodeCredentialTypes(const std::string &nodeId);

  void setNodeCredentialTypes(const std::string &type, const BaseLib::PVariable &credentialTypes, bool fromIpcServer);

  BaseLib::PVariable getNodeVariable(const std::string &nodeId, const std::string &topic);

  void setNodeVariable(const std::string &nodeId, const std::string &topic, const BaseLib::PVariable &value);

  void enableNodeEvents();

  void disableNodeEvents();

  void frontendNodeEventLog(const std::string &message);
 private:
  class QueueEntry : public BaseLib::IQueueEntry {
   public:
    QueueEntry() {
      this->time = BaseLib::HelperFunctions::getTime();
    }

    QueueEntry(const PNodeBlueClientData &clientData, const std::vector<char> &packet) {
      this->time = BaseLib::HelperFunctions::getTime();
      this->clientData = clientData;
      this->packet = packet;
    }

    QueueEntry(const PNodeBlueClientData &clientData, const std::string &methodName, const BaseLib::PArray &parameters) {
      this->time = BaseLib::HelperFunctions::getTime();
      this->clientData = clientData;
      this->methodName = methodName;
      this->parameters = parameters;
    }

    int64_t time = 0;
    PNodeBlueClientData clientData;

    // {{{ Request
    std::string methodName;
    BaseLib::PArray parameters;
    // }}}

    // {{{ Response
    std::vector<char> packet;
    // }}}
  };

  struct NodeServerInfo {
    int32_t clientId = -1;
    std::string type;
    NodeManager::NodeCodeType nodeCodeType = NodeManager::NodeCodeType::undefined;
    std::unordered_map<uint32_t, uint32_t> outputRoles;
  };

  BaseLib::Output _out;
  std::string _socketPath;
  std::string _webroot;
  std::vector<std::string> _anonymousPaths;
  std::atomic_bool _shuttingDown{false};
  std::atomic_bool _stopServer{false};
  std::atomic_bool _nodeEventsEnabled{false};
  std::thread _mainThread;
  std::thread _maintenanceThread;
  int32_t _backlog = 100;
  std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
  std::atomic<int32_t> _processCallbackHandlerId{-1};
  std::mutex _processRequestMutex;
  std::mutex _newProcessMutex;
  std::mutex _processMutex;
  std::map<pid_t, PNodeBlueProcess> _processes;
  std::mutex _currentFlowIdMutex;
  int32_t _currentFlowId = 0;
  std::mutex _stateMutex;
  std::shared_ptr<Nodepink> _nodepink;
  std::shared_ptr<NodepinkWebsocket> _nodepinkWebsocket;
  std::map<int32_t, PNodeBlueClientData> _clients;
  int32_t _currentClientId = 0;
  int64_t _lastGarbageCollection = 0;
  std::shared_ptr<BaseLib::RpcClientInfo> _nodeBlueClientInfo;
  std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> _rpcMethods;
  std::map<std::string, std::function<BaseLib::PVariable(PNodeBlueClientData &clientData, BaseLib::PArray &parameters)>> _localRpcMethods;
  std::mutex _packetIdMutex;
  int32_t _currentPacketId = 0;
  std::atomic_bool _flowsRestarting{false};
  std::mutex _restartFlowsMutex;
  std::mutex _flowsPostMutex;
  std::mutex _flowsFileMutex;
  std::mutex _nodesInstallMutex;
  std::unique_ptr<NodeManager> _nodeManager;
  std::map<std::string, uint32_t> _maxThreadCounts;
  std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
  std::unique_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;
  std::mutex _nodeInfoMapMutex;
  std::map<std::string, NodeServerInfo> _nodeInfoMap;
  std::mutex _flowClientIdMapMutex;
  std::map<std::string, int32_t> _flowClientIdMap;
  std::unique_ptr<NodeBlueCredentials> _nodeBlueCredentials;
  std::unique_ptr<BaseLib::HttpClient> _nodePinkHttpClient;

  std::atomic<int64_t> _lastNodeEvent{0};
  std::atomic<uint32_t> _nodeEventCounter{0};

  std::atomic<int64_t> _lastQueueSlowError{0};
  std::atomic_int _lastQueueSlowErrorCounter{0};

  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
  std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

  std::mutex _lifetick1Mutex;
  std::pair<int64_t, bool> _lifetick1;
  std::mutex _lifetick2Mutex;
  std::pair<int64_t, bool> _lifetick2;

  // {{{ Debugging / Valgrinding
  pid_t _manualClientCurrentProcessId = 1;
  std::mutex _unconnectedProcessesMutex;
  std::queue<pid_t> _unconnectedProcesses;
  // }}}

  void collectGarbage();

  bool getFileDescriptor(bool deleteOldSocket = false);

  void mainThread();

  void readClient(PNodeBlueClientData &clientData);

  BaseLib::PVariable send(PNodeBlueClientData &clientData, std::vector<char> &data);

  BaseLib::PVariable sendRequest(PNodeBlueClientData &clientData, const std::string &methodName, const BaseLib::PArray &parameters, bool wait);

  void sendResponse(PNodeBlueClientData &clientData, BaseLib::PVariable &scriptId, BaseLib::PVariable &packetId, BaseLib::PVariable &variable);

  void sendShutdown();

  bool sendReset();

  void closeClientConnections();

  void closeClientConnection(const PNodeBlueClientData &client);

  PNodeBlueProcess getFreeProcess(uint32_t maxThreadCount);

  void getMaxThreadCounts();

  bool checkIntegrity(const std::string &flowsFile);

  void backupFlows();

  void startFlows();

  void stopNodes();

  std::set<std::string> insertSubflows(BaseLib::PVariable &subflowNode,
                                       std::unordered_map<std::string, BaseLib::PVariable> &subflowInfos,
                                       std::unordered_map<std::string, BaseLib::PVariable> &flowNodes,
                                       std::unordered_map<std::string, BaseLib::PVariable> &subflowNodes,
                                       std::set<std::string> &allNodeIds);

  void startFlow(PFlowInfoServer &flowInfo, const std::unordered_map<std::string, BaseLib::PVariable> &flowNodes);

  void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) override;

  std::string processNodeUpload(BaseLib::Http &http);

  std::string deploy(BaseLib::Http &http);

  std::string installNode(BaseLib::Http &http);

  void parseEnvironmentVariables(const BaseLib::PVariable &env, BaseLib::PVariable &output);

  static std::string getNodeBlueFormatFromVariableType(const BaseLib::PVariable &variable);

  // {{{ RPC methods
  BaseLib::PVariable registerFlowsClient(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable errorEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable executePhpNode(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable executePhpNodeMethod(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable frontendEventLog(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable getCredentials(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable invokeNodeMethod(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable invokeIpcProcessMethod(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable nodeEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable nodeBlueVariableEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable nodeRedNodeInput(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable registerUiNodeRoles(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable setCredentials(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);

  BaseLib::PVariable setCredentialTypes(PNodeBlueClientData &clientData, BaseLib::PArray &parameters);
  // }}}
};

}

#endif
