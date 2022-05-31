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

#ifndef NODEBLUECLIENT_H_
#define NODEBLUECLIENT_H_

#include "NodeBlueResponseClient.h"
#include "FlowInfoClient.h"
#include "NodeManager.h"

#include <homegear-node/BinaryRpc.h>
#include <homegear-node/RpcDecoder.h>
#include <homegear-node/RpcEncoder.h>

#include <homegear-base/BaseLib.h>

#include <thread>
#include <mutex>
#include <string>
#include <utility>

namespace Homegear::NodeBlue {

class NodeBlueClient : public BaseLib::IQueue {
 public:
  NodeBlueClient();

  ~NodeBlueClient() override;

  void dispose();

  void start();

 private:
  struct RequestInfo {
    std::mutex waitMutex;
    std::condition_variable conditionVariable;
  };
  typedef std::shared_ptr<RequestInfo> PRequestInfo;

  struct InputValue {
    int64_t time = 0;
    Flows::PVariable value;
  };

  class QueueEntry : public BaseLib::IQueueEntry {
   public:
    QueueEntry() {
      this->time = BaseLib::HelperFunctions::getTime();
    }

    QueueEntry(const std::string &methodName, const Flows::PArray &parameters) {
      this->time = BaseLib::HelperFunctions::getTime();
      this->methodName = methodName;
      this->parameters = parameters;
    }

    explicit QueueEntry(std::vector<char> &packet) {
      this->time = BaseLib::HelperFunctions::getTime();
      this->packet = packet;
    }

    QueueEntry(const Flows::PNodeInfo &nodeInfo, uint32_t targetPort, const Flows::PVariable &message) {
      this->time = BaseLib::HelperFunctions::getTime();
      this->nodeInfo = nodeInfo;
      this->targetPort = targetPort;
      this->message = message;
    }

    int64_t time = 0;

    //{{{ Request
    std::string methodName;
    Flows::PArray parameters;
    //}}}

    //{{{ Response
    std::vector<char> packet;
    //}}}

    //{{{ Node output
    Flows::PNodeInfo nodeInfo;
    uint32_t targetPort = 0;
    Flows::PVariable message;
    //}}}
  };

  struct ErrorEventSubscription {
    std::string nodeId;
    bool catchConfigurationNodeErrors;
    bool hasScope;
    bool ignoreCaught;
  };

  BaseLib::Output _out;
  std::atomic_int _threadCount{0};
  std::atomic_int _processingThreadCount1{0};
  std::atomic_int _processingThreadCount2{0};
  std::atomic_int _processingThreadCount3{0};
  std::atomic<int64_t> _processingThreadCountMaxReached1{0};
  std::atomic<int64_t> _processingThreadCountMaxReached2{0};
  std::atomic<int64_t> _processingThreadCountMaxReached3{0};
  std::atomic_bool _startUpComplete{false};
  std::atomic_bool _shuttingDownOrRestarting{false};
  std::atomic_bool _shutdownComplete{false};
  std::atomic_bool _disposed{false};
  std::string _socketPath;
  std::shared_ptr<BaseLib::FileDescriptor> _fileDescriptor;
  std::atomic_bool _stopped{false};
  std::mutex _sendMutex;
  std::mutex _rpcResponsesMutex;
  std::map<int64_t, std::map<int32_t, PNodeBlueResponseClient>> _rpcResponses;
  std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
  std::map<std::string, std::function<Flows::PVariable(Flows::PArray &parameters)>> _localRpcMethods;
  std::thread _maintenanceThread;
  std::thread _watchdogThread;
  std::mutex _requestInfoMutex;
  std::map<int64_t, PRequestInfo> _requestInfo;
  std::mutex _packetIdMutex;
  int32_t _currentPacketId = 0;
  std::atomic_bool _nodesStopped{false};
  std::unique_ptr<NodeManager> _nodeManager;
  std::atomic_bool _frontendConnected{false};
  std::atomic<int64_t> _lastQueueSize{0};
  std::mutex _eventFlowIdMutex;
  std::string _eventFlowId;
  std::atomic_int _lastQueueSlowErrorCounter{0};
  std::atomic<int64_t> _lastQueueSlowError{0};

  std::unique_ptr<Flows::BinaryRpc> _binaryRpc;
  std::unique_ptr<Flows::RpcDecoder> _rpcDecoder;
  std::unique_ptr<Flows::RpcEncoder> _rpcEncoder;

  std::mutex _startFlowMutex;

  std::mutex _flowsMutex;
  std::unordered_map<int32_t, PFlowInfoClient> _flows;

  std::mutex _nodesMutex;
  std::unordered_map<std::string, Flows::PNodeInfo> _nodes;

  std::mutex _peerSubscriptionsMutex;
  std::unordered_map<uint64_t, std::unordered_map<int32_t, std::unordered_map<std::string, std::set<std::string>>>> _peerSubscriptions;
  std::mutex _homegearEventSubscriptionsMutex;
  std::unordered_set<std::string> _homegearEventSubscriptions;
  std::mutex _statusEventSubscriptionsMutex;
  std::unordered_set<std::string> _statusEventSubscriptions;
  std::mutex _errorEventSubscriptionsMutex;
  std::list<ErrorEventSubscription> _errorEventSubscriptions;

  std::mutex _flowSubscriptionsMutex;
  std::unordered_map<std::string, std::set<std::string>> _flowSubscriptions;

  std::mutex _globalSubscriptionsMutex;
  std::set<std::string> _globalSubscriptions;

  std::mutex _internalMessagesMutex;
  std::unordered_map<std::string, Flows::PVariable> _internalMessages;

  std::mutex _inputValuesMutex;
  std::unordered_map<std::string, std::unordered_map<int32_t, std::list<InputValue>>> _inputValues;

  std::mutex _fixedInputValuesMutex;
  std::unordered_map<std::string, std::unordered_map<int32_t, Flows::PVariable>> _fixedInputValues;

  void watchdog();

  void resetClient(Flows::PVariable packetId);

  void registerClient();

  Flows::PVariable invoke(const std::string &methodName, Flows::PArray parameters, bool wait);

  Flows::PVariable invokeNodeMethod(const std::string &nodeId, const std::string &methodName, Flows::PArray parameters, bool wait);

  void sendResponse(Flows::PVariable &packetId, Flows::PVariable &variable);

  void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) override;

  Flows::PVariable send(std::vector<char> &data);

  void log(const std::string &nodeId, int32_t logLevel, const std::string &message);

  void errorEvent(const std::string &nodeId, int32_t level, const std::string &message);

  void frontendEventLog(const std::string &nodeId, const std::string &message);

  void subscribePeer(const std::string &nodeId, uint64_t peerId, int32_t channel, const std::string &variable);

  void unsubscribePeer(const std::string &nodeId, uint64_t peerId, int32_t channel, const std::string &variable);

  void subscribeFlow(const std::string &nodeId, const std::string &flowId);

  void unsubscribeFlow(const std::string &nodeId, const std::string &flowId);

  void subscribeGlobal(const std::string &nodeId);

  void unsubscribeGlobal(const std::string &nodeId);

  void subscribeHomegearEvents(const std::string &nodeId);

  void unsubscribeHomegearEvents(const std::string &nodeId);

  void subscribeStatusEvents(const std::string &nodeId);

  void unsubscribeStatusEvents(const std::string &nodeId);

  void subscribeErrorEvents(const std::string &nodeId, bool catchConfigurationNodeErrors, bool hasScope, bool ignoreCaught);

  void unsubscribeErrorEvents(const std::string &nodeId);

  void queueOutput(const std::string &nodeId, uint32_t index, Flows::PVariable message, bool synchronous);

  void nodeEvent(const std::string &nodeId, const std::string &topic, const Flows::PVariable& value, bool retain);

  Flows::PVariable getNodeData(const std::string &nodeId, const std::string &key);

  void setNodeData(const std::string &nodeId, const std::string &key, Flows::PVariable value);

  Flows::PVariable getFlowData(const std::string &flowId, const std::string &key);

  void setFlowData(const std::string &flowId, const std::string &key, Flows::PVariable value);

  Flows::PVariable getGlobalData(const std::string &key);

  void setGlobalData(const std::string &key, Flows::PVariable value);

  void setInternalMessage(const std::string &nodeId, Flows::PVariable message);

  void setInputValue(const std::string &nodeType, const std::string &nodeId, int32_t inputIndex, const Flows::PVariable& message);

  Flows::PVariable getConfigParameter(const std::string &nodeId, const std::string &name);

  // {{{ RPC methods
  /**
   * Causes the log files to be reopened.
   * @param parameters Irrelevant for this method.
   */
  Flows::PVariable reload(Flows::PArray &parameters);

  /**
   * Causes the script engine client to exit.
   * @param parameters Irrelevant for this method.
   */
  Flows::PVariable shutdown(Flows::PArray &parameters);

  /**
   * Checks if everything is working.
   * @param parameters Irrelevant for this method.
   */
  Flows::PVariable lifetick(Flows::PArray &parameters);

  /**
   * Returns load metrics.
   * @param parameters Irrelevant for this method.
   */
  Flows::PVariable getLoad(Flows::PArray &parameters);

  /**
   * Starts a new flow.
   * @param parameters
   */
  Flows::PVariable startFlow(Flows::PArray &parameters);

  /**
   * Executes the method "start" on all nodes. It is run after all nodes are initialized.
   * @param parameters
   */
  Flows::PVariable startNodes(Flows::PArray &parameters);

  /**
   * Executed when all config nodes are available.
   * @param parameters
   */
  Flows::PVariable configNodesStarted(Flows::PArray &parameters);

  /**
   * Executed when start up is complete. Nodes can output data from within this method.
   * @param parameters
   */
  Flows::PVariable startUpComplete(Flows::PArray &parameters);

  /**
   * Executes the method "stop" on all nodes. RPC methods can still be called within "stop", but not afterwards.
   * @param parameters
   */
  Flows::PVariable stopNodes(Flows::PArray &parameters);

  /**
   * Executes the method "waitForStop" on all nodes. RPC methods can't be called anymore.
   * @param parameters
   */
  Flows::PVariable waitForNodesStopped(Flows::PArray &parameters);

  /**
   * Stops a flow.
   * @param parameters
   */
  Flows::PVariable stopFlow(Flows::PArray &parameters);

  /**
   * Returns the number of flows currently running.
   * @param parameters Irrelevant for this method.
   * @return Returns the number of running flows.
   */
  Flows::PVariable flowCount(Flows::PArray &parameters);

  Flows::PVariable nodeLog(Flows::PArray &parameters);

  Flows::PVariable nodeOutput(Flows::PArray &parameters);

  Flows::PVariable invokeExternalNodeMethod(Flows::PArray &parameters);

  Flows::PVariable executePhpNodeBaseMethod(Flows::PArray &parameters);

  Flows::PVariable getNodesWithFixedInputs(Flows::PArray &parameters);

  Flows::PVariable getNodeProcessingTimes(Flows::PArray &parameters);

  Flows::PVariable getNodeVariable(Flows::PArray &parameters);

  Flows::PVariable getFlowVariable(Flows::PArray &parameters);

  Flows::PVariable setNodeVariable(Flows::PArray &parameters);

  Flows::PVariable setFlowVariable(Flows::PArray &parameters);

  Flows::PVariable enableNodeEvents(Flows::PArray &parameters);

  Flows::PVariable disableNodeEvents(Flows::PArray &parameters);

  Flows::PVariable broadcastEvent(Flows::PArray &parameters);

  Flows::PVariable broadcastFlowVariableEvent(Flows::PArray &parameters);

  Flows::PVariable broadcastGlobalVariableEvent(Flows::PArray &parameters);

  Flows::PVariable broadcastServiceMessage(Flows::PArray &parameters);

  Flows::PVariable broadcastNewDevices(Flows::PArray &parameters);

  Flows::PVariable broadcastDeleteDevices(Flows::PArray &parameters);

  Flows::PVariable broadcastUpdateDevice(Flows::PArray &parameters);

  Flows::PVariable broadcastStatus(Flows::PArray &parameters);

  Flows::PVariable broadcastError(Flows::PArray &parameters);

  Flows::PVariable broadcastVariableProfileStateChanged(Flows::PArray &parameters);

  Flows::PVariable broadcastUiNotificationCreated(Flows::PArray &parameters);

  Flows::PVariable broadcastUiNotificationRemoved(Flows::PArray &parameters);

  Flows::PVariable broadcastUiNotificationAction(Flows::PArray &parameters);

  Flows::PVariable broadcastRawPacketEvent(Flows::PArray &parameters);
  // }}}
};

}
#endif
