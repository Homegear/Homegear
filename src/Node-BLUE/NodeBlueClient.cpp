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

#include "NodeBlueClient.h"
#include <homegear-node/JsonEncoder.h>
#include <homegear-node/JsonDecoder.h>
#include "../GD/GD.h"
#include <memory>
#include <utility>

namespace Homegear::NodeBlue {

NodeBlueClient::NodeBlueClient() : IQueue(GD::bl.get(), 3, 100000) {
  _fileDescriptor = std::make_shared<BaseLib::FileDescriptor>();
  _out.init(GD::bl.get());
  _out.setPrefix("Node-BLUE (" + std::to_string(getpid()) + "): ");

  _dummyClientInfo.reset(new BaseLib::RpcClientInfo());

  _nodeManager = std::make_unique<NodeManager>(&_frontendConnected);

  _binaryRpc = std::make_unique<Flows::BinaryRpc>();
  _rpcDecoder = std::make_unique<Flows::RpcDecoder>();
  _rpcEncoder = std::make_unique<Flows::RpcEncoder>(true);

  _localRpcMethods.emplace("reload", std::bind(&NodeBlueClient::reload, this, std::placeholders::_1));
  _localRpcMethods.emplace("shutdown", std::bind(&NodeBlueClient::shutdown, this, std::placeholders::_1));
  _localRpcMethods.emplace("lifetick", std::bind(&NodeBlueClient::lifetick, this, std::placeholders::_1));
  _localRpcMethods.emplace("getLoad", std::bind(&NodeBlueClient::getLoad, this, std::placeholders::_1));
  _localRpcMethods.emplace("startFlow", std::bind(&NodeBlueClient::startFlow, this, std::placeholders::_1));
  _localRpcMethods.emplace("startNodes", std::bind(&NodeBlueClient::startNodes, this, std::placeholders::_1));
  _localRpcMethods.emplace("configNodesStarted", std::bind(&NodeBlueClient::configNodesStarted, this, std::placeholders::_1));
  _localRpcMethods.emplace("startUpComplete", std::bind(&NodeBlueClient::startUpComplete, this, std::placeholders::_1));
  _localRpcMethods.emplace("stopNodes", std::bind(&NodeBlueClient::stopNodes, this, std::placeholders::_1));
  _localRpcMethods.emplace("waitForNodesStopped", std::bind(&NodeBlueClient::waitForNodesStopped, this, std::placeholders::_1));
  _localRpcMethods.emplace("stopFlow", std::bind(&NodeBlueClient::stopFlow, this, std::placeholders::_1));
  _localRpcMethods.emplace("flowCount", std::bind(&NodeBlueClient::flowCount, this, std::placeholders::_1));
  _localRpcMethods.emplace("nodeLog", std::bind(&NodeBlueClient::nodeLog, this, std::placeholders::_1));
  _localRpcMethods.emplace("nodeOutput", std::bind(&NodeBlueClient::nodeOutput, this, std::placeholders::_1));
  _localRpcMethods.emplace("invokeNodeMethod", std::bind(&NodeBlueClient::invokeExternalNodeMethod, this, std::placeholders::_1));
  _localRpcMethods.emplace("executePhpNodeBaseMethod", std::bind(&NodeBlueClient::executePhpNodeBaseMethod, this, std::placeholders::_1));
  _localRpcMethods.emplace("getNodeProcessingTimes", std::bind(&NodeBlueClient::getNodeProcessingTimes, this, std::placeholders::_1));
  _localRpcMethods.emplace("getNodesWithFixedInputs", std::bind(&NodeBlueClient::getNodesWithFixedInputs, this, std::placeholders::_1));
  _localRpcMethods.emplace("getFlowVariable", std::bind(&NodeBlueClient::getFlowVariable, this, std::placeholders::_1));
  _localRpcMethods.emplace("getNodeVariable", std::bind(&NodeBlueClient::getNodeVariable, this, std::placeholders::_1));
  _localRpcMethods.emplace("setFlowVariable", std::bind(&NodeBlueClient::setFlowVariable, this, std::placeholders::_1));
  _localRpcMethods.emplace("setNodeVariable", std::bind(&NodeBlueClient::setNodeVariable, this, std::placeholders::_1));
  _localRpcMethods.emplace("enableNodeEvents", std::bind(&NodeBlueClient::enableNodeEvents, this, std::placeholders::_1));
  _localRpcMethods.emplace("disableNodeEvents", std::bind(&NodeBlueClient::disableNodeEvents, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastEvent", std::bind(&NodeBlueClient::broadcastEvent, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastFlowVariableEvent", std::bind(&NodeBlueClient::broadcastFlowVariableEvent, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastGlobalVariableEvent", std::bind(&NodeBlueClient::broadcastGlobalVariableEvent, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastServiceMessage", std::bind(&NodeBlueClient::broadcastServiceMessage, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastDeleteDevices", std::bind(&NodeBlueClient::broadcastDeleteDevices, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastNewDevices", std::bind(&NodeBlueClient::broadcastNewDevices, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUpdateDevice", std::bind(&NodeBlueClient::broadcastUpdateDevice, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastStatus", std::bind(&NodeBlueClient::broadcastStatus, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastError", std::bind(&NodeBlueClient::broadcastError, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastVariableProfileStateChanged", std::bind(&NodeBlueClient::broadcastVariableProfileStateChanged, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationCreated", std::bind(&NodeBlueClient::broadcastUiNotificationCreated, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationRemoved", std::bind(&NodeBlueClient::broadcastUiNotificationRemoved, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationAction", std::bind(&NodeBlueClient::broadcastUiNotificationAction, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastRawPacketEvent", std::bind(&NodeBlueClient::broadcastRawPacketEvent, this, std::placeholders::_1));
}

NodeBlueClient::~NodeBlueClient() {
  dispose();
  if (_maintenanceThread.joinable()) _maintenanceThread.join();
  if (_watchdogThread.joinable()) _watchdogThread.join();
}

void NodeBlueClient::dispose() {
  try {
    std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
    if (_disposed) return;
    _disposed = true;
    _out.printMessage("Shutting down...");

    _out.printMessage("Nodes are stopped. Disposing...");

    GD::bl->shuttingDown = true;
    _stopped = true;

    std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
    for (auto &request: _requestInfo) {
      std::unique_lock<std::mutex> waitLock(request.second->waitMutex);
      waitLock.unlock();
      request.second->conditionVariable.notify_all();
    }

    stopQueue(0);
    stopQueue(1);
    stopQueue(2);

    {
      std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
      for (auto &flow: _flows) {
        for (auto &node: flow.second->nodes) {
          {
            std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
            for (auto &peerId: _peerSubscriptions) {
              for (auto &channel: peerId.second) {
                for (auto &variable: channel.second) {
                  variable.second.erase(node.first);
                }
              }
            }
          }

          {
            std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
            for (auto &flowId: _flowSubscriptions) {
              flowId.second.erase(node.first);
            }
          }

          {
            std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
            _globalSubscriptions.erase(node.first);
          }

          {
            std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
            _homegearEventSubscriptions.erase(node.first);
          }

          _nodeManager->unloadNode(node.second->id);
        }
      }
      _flows.clear();
    }

    {
      std::lock_guard<std::mutex> rpcResponsesGuard(_rpcResponsesMutex);
      _rpcResponses.clear();
    }

    _shutdownComplete = true;
    _out.printMessage("Shut down complete.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::resetClient(Flows::PVariable packetId) {
  try {
    _shuttingDownOrRestarting = true;

    std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);

    _out.printMessage("Nodes are stopped. Stopping queues...");

    {
      std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
      for (auto &request: _requestInfo) {
        std::unique_lock<std::mutex> waitLock(request.second->waitMutex);
        waitLock.unlock();
        request.second->conditionVariable.notify_all();
      }
    }

    stopQueue(0);
    stopQueue(1);
    stopQueue(2);

    _out.printMessage("Doing final cleanups...");

    {
      std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
      for (auto &flow: _flows) {
        for (auto &node: flow.second->nodes) {
          {
            std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
            for (auto &peerId: _peerSubscriptions) {
              for (auto &channel: peerId.second) {
                for (auto &variable: channel.second) {
                  variable.second.erase(node.first);
                }
              }
            }
          }

          {
            std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
            for (auto &flowId: _flowSubscriptions) {
              flowId.second.erase(node.first);
            }
          }

          {
            std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
            _globalSubscriptions.erase(node.first);
          }

          {
            std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
            _homegearEventSubscriptions.erase(node.first);
          }

          _nodeManager->unloadNode(node.second->id);
        }
      }
      _flows.clear();
    }

    {
      std::lock_guard<std::mutex> rpcResponsesGuard(_rpcResponsesMutex);
      _rpcResponses.clear();
    }

    {
      std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
      _requestInfo.clear();
    }

    {
      std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
      _nodes.clear();
    }

    {
      std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
      _peerSubscriptions.clear();
    }

    {
      std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
      _flowSubscriptions.clear();
    }

    {
      std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
      _globalSubscriptions.clear();
    }

    {
      std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
      _homegearEventSubscriptions.clear();
    }

    {
      std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
      _internalMessages.clear();
    }

    // We don't reset _inputValues. This keeps old nodes in the array but you can still request history data for all
    // nodes that still exist after reload which outweighs this problem.
    /*{
        std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
        _inputValues.clear();
    }*/

    _nodeManager->clearManagerModuleInfo();

    _out.printMessage("Reinitializing...");

    if (_watchdogThread.joinable()) _watchdogThread.join();

    _shuttingDownOrRestarting = false;
    _startUpComplete = false;
    _nodesStopped = false;
    _lastQueueSize = 0;

    startQueue(0, false, _threadCount, 0, SCHED_OTHER);
    startQueue(1, false, _threadCount, 0, SCHED_OTHER);
    startQueue(2, false, _threadCount, 0, SCHED_OTHER);

    if (GD::bl->settings.nodeBlueWatchdogTimeout() >= 1000) _watchdogThread = std::thread(&NodeBlueClient::watchdog, this);

    _out.printMessage("Reset complete.");
    Flows::PVariable result = std::make_shared<Flows::Variable>();
    sendResponse(packetId, result);
    return;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  Flows::PVariable result = Flows::Variable::createError(-32500, "Unknown application error.");
  sendResponse(packetId, result);
}

void NodeBlueClient::start() {
  try {
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.log").c_str(), "a", stdout)) {
      _out.printError("Error: Could not redirect output to log file.");
    }
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.err").c_str(), "a", stderr)) {
      _out.printError("Error: Could not redirect errors to log file.");
    }

    uint32_t flowsProcessingThreadCountNodes = GD::bl->settings.nodeBlueProcessingThreadCountNodes();
    if (flowsProcessingThreadCountNodes < 5) flowsProcessingThreadCountNodes = 5;
    _threadCount = flowsProcessingThreadCountNodes;

    _processingThreadCount1 = 0;
    _processingThreadCount2 = 0;
    _processingThreadCount3 = 0;

    startQueue(0, false, _threadCount, 0, SCHED_OTHER);
    startQueue(1, false, _threadCount, 0, SCHED_OTHER);
    startQueue(2, false, _threadCount, 0, SCHED_OTHER);

    if (GD::bl->settings.nodeBlueWatchdogTimeout() >= 1000) _watchdogThread = std::thread(&NodeBlueClient::watchdog, this);

    _socketPath = GD::bl->settings.socketPath() + "homegearFE.sock";
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Socket path is " + _socketPath);
    for (int32_t i = 0; i < 2; i++) {
      _fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
      if (!_fileDescriptor || _fileDescriptor->descriptor == -1) {
        _out.printError("Could not create socket.");
        return;
      }

      if (GD::bl->debugLevel >= 4 && i == 0) std::cout << "Info: Trying to connect..." << std::endl;
      sockaddr_un remoteAddress{};
      remoteAddress.sun_family = AF_LOCAL;
      //104 is the size on BSD systems - slightly smaller than in Linux
      if (_socketPath.length() > 104) {
        //Check for buffer overflow
        _out.printCritical("Critical: Socket path is too long.");
        return;
      }
      strncpy(remoteAddress.sun_path, _socketPath.c_str(), 104);
      remoteAddress.sun_path[103] = 0; //Just to make sure it is null terminated.
      if (connect(_fileDescriptor->descriptor, (struct sockaddr *)&remoteAddress, static_cast<socklen_t>(strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family))) == -1) {
        if (i == 0) {
          _out.printDebug("Debug: Socket closed. Trying again...");
          //When socket was not properly closed, we sometimes need to reconnect
          std::this_thread::sleep_for(std::chrono::milliseconds(2000));
          continue;
        } else {
          _out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
          return;
        }
      } else break;
    }
    if (GD::bl->debugLevel >= 4) _out.printMessage("Connected.");

    if (_maintenanceThread.joinable()) _maintenanceThread.join();
    _maintenanceThread = std::thread(&NodeBlueClient::registerClient, this);

    std::vector<char> buffer(1024);
    int32_t result = 0;
    int32_t bytesRead = 0;
    int32_t processedBytes = 0;
    while (!_stopped) {
      timeval timeout{};
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      fd_set readFileDescriptor{};
      FD_ZERO(&readFileDescriptor);
      {
        auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
        fileDescriptorGuard.lock();
        FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
      }

      result = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, nullptr, nullptr, &timeout);
      if (result == 0) continue;
      else if (result == -1) {
        if (errno == EINTR) continue;
        _out.printMessage("Connection to flows server closed (1). Exiting.");
        if (!_shutdownComplete) {
          _out.printError("Error: Shutdown is not complete. Killing process...");
          kill(getpid(), 9);
        }
        return;
      }

      bytesRead = static_cast<int32_t>(read(_fileDescriptor->descriptor, &buffer[0], 1024));
      if (bytesRead <= 0) //read returns 0, when connection is disrupted.
      {
        _out.printMessage("Connection to flows server closed (2). Exiting.");
        if (!_shutdownComplete) {
          _out.printError("Error: Shutdown is not complete. Killing process...");
          kill(getpid(), 9);
        }
        return;
      }

      if (bytesRead > (signed)buffer.size()) bytesRead = static_cast<int32_t>(buffer.size());

      try {
        processedBytes = 0;
        while (processedBytes < bytesRead) {
          processedBytes += _binaryRpc->process(buffer.data() + processedBytes, bytesRead - processedBytes);
          if (_binaryRpc->isFinished()) {
            if (_binaryRpc->getType() == Flows::BinaryRpc::Type::request) {
              std::string methodName;
              Flows::PArray parameters = _rpcDecoder->decodeRequest(_binaryRpc->getData(), methodName);
              std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(methodName, parameters);
              if (methodName == "shutdown") shutdown(parameters->at(2)->arrayValue);
              else if (methodName == "reset") {
                if (_maintenanceThread.joinable()) _maintenanceThread.join();
                _maintenanceThread = std::thread(&NodeBlueClient::resetClient, this, parameters->at(0));
              } else if (!enqueue(0, queueEntry, !_startUpComplete)) printQueueFullError(_out, "Error: Could not queue RPC request because buffer is full. Dropping it.");
            } else {
              std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(_binaryRpc->getData());
              if (!enqueue(1, queueEntry, !_startUpComplete)) printQueueFullError(_out, "Error: Could not queue RPC response because buffer is full. Dropping it.");
            }
            _binaryRpc->reset();
          }
        }
      }
      catch (Flows::BinaryRpcException &ex) {
        _out.printError("Error processing packet: " + std::string(ex.what()));
        _binaryRpc->reset();
      }
    }
    buffer.clear();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::registerClient() {
  try {
    std::string methodName("registerFlowsClient");
    Flows::PArray parameters(new Flows::Array{std::make_shared<Flows::Variable>((int32_t)getpid())});
    Flows::PVariable result = invoke(methodName, parameters, true);
    if (result->errorStruct) {
      _out.printCritical("Critical: Could not register client.");
      dispose();
      return;
    }
    _out.printInfo("Info: Client registered to server.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    std::shared_ptr<QueueEntry> queueEntry;
    queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
    if (!queueEntry) return;

    if (BaseLib::HelperFunctions::getTime() - queueEntry->time > 2000) {
      _lastQueueSlowErrorCounter++;
      if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
        _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
        _lastQueueSlowErrorCounter = 0;
        _out.printWarning(
            "Warning: Queue entry of queue " + std::to_string(index) + " was queued for " + std::to_string(BaseLib::HelperFunctions::getTime() - queueEntry->time)
                + "ms. Either something is hanging or you need to increase your number of processing threads. Messages since last log entry: " + std::to_string(_lastQueueSlowErrorCounter));
      }
    }

    if (index == 0) //IPC request
    {
      auto start_time = BaseLib::HelperFunctions::getTime();
      _processingThreadCount1++;
      try {
        if (_processingThreadCount1 == _threadCount) _processingThreadCountMaxReached1 = BaseLib::HelperFunctions::getTime();
        if (queueEntry->parameters->size() < 3) {
          _out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
          return;
        }
        auto localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
        if (localMethodIterator == _localRpcMethods.end()) {
          _out.printError("Warning: RPC method not found: " + queueEntry->methodName);
          Flows::PVariable error = Flows::Variable::createError(-32601, "Requested method not found.");
          if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), error);
          return;
        }

        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Debug: Server is calling RPC method: " + queueEntry->methodName + " Parameters:");
          for (const auto &parameter: *queueEntry->parameters) {
            parameter->print(true, false);
          }
        }

        Flows::PVariable result = localMethodIterator->second(queueEntry->parameters->at(2)->arrayValue);
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Response: ");
          result->print(true, false);
        }
        if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), result);
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      _processingThreadCountMaxReached1 = 0;
      _processingThreadCount1--;
      if (BaseLib::HelperFunctions::getTime() - start_time > 2000) {
        _lastQueueSlowErrorCounter++;
        if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
          _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
          _lastQueueSlowErrorCounter = 0;
          _out.printInfo("Info: Processing of IPC method call to " + queueEntry->methodName + " took " + std::to_string(BaseLib::HelperFunctions::getTime() - queueEntry->time) + "ms.");
        }
      }
    } else if (index == 1) //IPC response
    {
      auto start_time = BaseLib::HelperFunctions::getTime();
      _processingThreadCount2++;
      try {
        if (_processingThreadCount2 == _threadCount) _processingThreadCountMaxReached2 = BaseLib::HelperFunctions::getTime();

        Flows::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
        if (response->arrayValue->size() < 3) {
          _out.printError("Error: Response has wrong array size.");
          return;
        }
        int64_t threadId = response->arrayValue->at(0)->integerValue64;
        int32_t packetId = response->arrayValue->at(1)->integerValue;

        PRequestInfo requestInfo;
        {
          std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
          auto requestIterator = _requestInfo.find(threadId);
          if (requestIterator != _requestInfo.end()) requestInfo = requestIterator->second;
        }
        if (requestInfo) {
          std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
          {
            std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
            auto responseIterator = _rpcResponses[threadId].find(packetId);
            if (responseIterator != _rpcResponses[threadId].end()) {
              PNodeBlueResponseClient element = responseIterator->second;
              if (element) {
                element->response = response;
                element->packetId = packetId;
                element->finished = true;
              }
            }
          }
          waitLock.unlock();
          requestInfo->conditionVariable.notify_all();
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      _processingThreadCountMaxReached2 = 0;
      _processingThreadCount2--;
      if (BaseLib::HelperFunctions::getTime() - start_time > 2000) {
        _lastQueueSlowErrorCounter++;
        if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
          _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
          _lastQueueSlowErrorCounter = 0;
          _out.printInfo("Info: Processing of IPC method response took " + std::to_string(BaseLib::HelperFunctions::getTime() - queueEntry->time) + "ms.");
        }
      }
    } else //Node output
    {
      auto start_time = BaseLib::HelperFunctions::getTime();
      _processingThreadCount3++;
      try {
        if (_processingThreadCount3 == _threadCount) _processingThreadCountMaxReached3 = BaseLib::HelperFunctions::getTime();

        if (!queueEntry->nodeInfo || !queueEntry->message || _shuttingDownOrRestarting) return;
        Flows::PINode node = _nodeManager->getNode(queueEntry->nodeInfo->id);
        if (node) {
          std::string eventFlowId;
          {
            std::lock_guard<std::mutex> eventFlowIdGuard(_eventFlowIdMutex);
            eventFlowId = _eventFlowId;
          }
          if (GD::bl->settings.nodeBlueDebugOutput() && _startUpComplete && _frontendConnected && node->getFlowId() == eventFlowId && BaseLib::HelperFunctions::getTime() - queueEntry->nodeInfo->lastNodeEvent1 >= 100) {
            queueEntry->nodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
            Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
            timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
            nodeEvent(queueEntry->nodeInfo->id, "highlightNode/" + queueEntry->nodeInfo->id, timeout, false);
          }
          auto internalMessageIterator = queueEntry->message->structValue->find("_internal");
          if (internalMessageIterator != queueEntry->message->structValue->end()) setInternalMessage(queueEntry->nodeInfo->id, internalMessageIterator->second);

          {
            std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
            auto nodeIterator = _fixedInputValues.find(queueEntry->nodeInfo->id);
            if (nodeIterator != _fixedInputValues.end()) {
              auto inputIterator = nodeIterator->second.find(queueEntry->targetPort);
              if (inputIterator != nodeIterator->second.end()) {
                auto messageCopy = std::make_shared<Flows::Variable>();
                *messageCopy = *(queueEntry->message);
                (*messageCopy->structValue)["payload"] = inputIterator->second;
                queueEntry->message = messageCopy;
              }
            }
          }

          if (_nodeManager->isNodeRedNode(queueEntry->nodeInfo->type)) {
            auto parameters = std::make_shared<Flows::Array>();
            parameters->reserve(5);
            parameters->emplace_back(std::make_shared<Flows::Variable>(node->getId()));
            parameters->emplace_back(queueEntry->nodeInfo->serialize());
            parameters->emplace_back(std::make_shared<Flows::Variable>(queueEntry->targetPort));
            parameters->emplace_back(queueEntry->message);
            parameters->emplace_back(std::make_shared<Flows::Variable>(false));
            auto result = invoke("nodeRedNodeInput", parameters, false);
            if (result->errorStruct) _out.printError("Error calling \"nodeRedNodeInput\": " + result->structValue->at("faultString")->stringValue);
          } else {
            std::lock_guard<std::mutex> nodeInputGuard(node->getInputMutex());
            node->input(queueEntry->nodeInfo, queueEntry->targetPort, queueEntry->message);
          }

          setInputValue(queueEntry->nodeInfo->type, queueEntry->nodeInfo->id, queueEntry->targetPort, queueEntry->message);
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      _processingThreadCountMaxReached3 = 0;
      _processingThreadCount3--;
      auto processing_time = BaseLib::HelperFunctions::getTime() - start_time;
      if (queueEntry->targetPort + 1 >= queueEntry->nodeInfo->processingTimes.size()) queueEntry->nodeInfo->processingTimes.resize(queueEntry->targetPort + 1);
      queueEntry->nodeInfo->processingTimes.at(queueEntry->targetPort) = processing_time;
      if (processing_time > 2000) {
        _lastQueueSlowErrorCounter++;
        if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
          _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
          _lastQueueSlowErrorCounter = 0;
          _out.printInfo("Info: Processing of node input took " + std::to_string(processing_time) + "ms (node: " + queueEntry->nodeInfo->id + ", input: " + std::to_string(queueEntry->targetPort) + ").");
        }
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PVariable NodeBlueClient::send(std::vector<char> &data) {
  try {
    int32_t totallySentBytes = 0;
    std::lock_guard<std::mutex> sendGuard(_sendMutex);
    while (totallySentBytes < (signed)data.size()) {
      auto sentBytes = static_cast<int32_t>(::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL));
      if (sentBytes <= 0) {
        if (errno == EAGAIN) continue;
        if (_fileDescriptor->descriptor != -1)
          _out.printError("Could not send data to client " + std::to_string(_fileDescriptor->descriptor) + ". Sent bytes: " + std::to_string(totallySentBytes) + " of " + std::to_string(data.size())
                              + (sentBytes == -1 ? ". Error message: " + std::string(strerror(errno)) : ""));
        return Flows::Variable::createError(-32500, "Unknown application error.");
      }
      totallySentBytes += sentBytes;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<Flows::Variable>();
}

Flows::PVariable NodeBlueClient::invoke(const std::string &methodName, Flows::PArray parameters, bool wait) {
  try {
    if (_nodesStopped) {
      if (methodName != "waitForStop" && methodName != "executePhpNodeMethod") return Flows::Variable::createError(-32501, "RPC calls are forbidden after \"stop()\" has been called.");
      else if (methodName == "executePhpNodeMethod" && (parameters->size() < 2 || parameters->at(1)->stringValue != "waitForStop")) return Flows::Variable::createError(-32501, "RPC calls are forbidden after \"stop()\" has been called.");
    }

    int64_t threadId = pthread_self();

    PRequestInfo requestInfo;
    if (wait) {
      std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
      auto emplaceResult = _requestInfo.emplace(std::piecewise_construct, std::make_tuple(threadId), std::make_tuple(std::make_shared<RequestInfo>()));
      if (!emplaceResult.second) _out.printError("Error: Thread is executing invoke() more than once.");
      requestInfo = emplaceResult.first->second;
    }

    int32_t packetId;
    {
      std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
      packetId = _currentPacketId++;
    }
    Flows::PArray array = std::make_shared<Flows::Array>();
    array->reserve(4);
    array->push_back(std::make_shared<Flows::Variable>(threadId));
    array->push_back(std::make_shared<Flows::Variable>(packetId));
    array->push_back(std::make_shared<Flows::Variable>(wait));
    array->push_back(std::make_shared<Flows::Variable>(parameters));
    std::vector<char> data;
    _rpcEncoder->encodeRequest(methodName, array, data);

    PNodeBlueResponseClient response;
    if (wait) {
      {
        std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
        auto result = _rpcResponses[threadId].emplace(packetId, std::make_shared<NodeBlueResponseClient>());
        if (result.second) response = result.first->second;
      }
      if (!response) {
        _out.printError("Critical: Could not insert response struct into map.");
        return Flows::Variable::createError(-32500, "Unknown application error.");
      }
    }
    Flows::PVariable result = send(data);
    if (result->errorStruct || !wait) {
      if (!wait) return std::make_shared<Flows::Variable>();
      else {
        std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
        _rpcResponses[threadId].erase(packetId);
        if (_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
        return result;
      }
    }

    int64_t startTime = BaseLib::HelperFunctions::getTime();
    std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
    while (!requestInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
      if (_shuttingDownOrRestarting && BaseLib::HelperFunctions::getTime() - startTime > 10000) return true;
      else return response->finished || _stopped;
    }));

    if (!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId) {
      _out.printError("Error: No response received to RPC request. Method: " + methodName);
      result = Flows::Variable::createError(-1, "No response received.");
    } else result = response->response->arrayValue->at(2);

    {
      std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
      _rpcResponses[threadId].erase(packetId);
      if (_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
    }

    {
      std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
      _requestInfo.erase(threadId);
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::invokeNodeMethod(const std::string &nodeId, const std::string &methodName, Flows::PArray parameters, bool wait) {
  try {
    Flows::PINode node = _nodeManager->getNode(nodeId);
    if (node && !_nodeManager->isNodeRedNode(node->getType())) return node->invokeLocal(methodName, parameters);

    Flows::PArray parametersArray = std::make_shared<Flows::Array>();
    parametersArray->reserve(4);
    parametersArray->push_back(std::make_shared<Flows::Variable>(nodeId));
    parametersArray->push_back(std::make_shared<Flows::Variable>(methodName));
    parametersArray->push_back(std::make_shared<Flows::Variable>(parameters));
    parametersArray->push_back(std::make_shared<Flows::Variable>(wait));

    return invoke("invokeNodeMethod", parametersArray, wait);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

void NodeBlueClient::sendResponse(Flows::PVariable &packetId, Flows::PVariable &variable) {
  try {
    Flows::PVariable array(new Flows::Variable(Flows::PArray(new Flows::Array{packetId, variable})));
    std::vector<char> data;
    _rpcEncoder->encodeResponse(array, data);

    send(data);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::log(const std::string &nodeId, int32_t logLevel, const std::string &message) {
  _out.printMessage("Node " + nodeId + ": " + message, logLevel, logLevel <= 3);
  frontendEventLog(nodeId, message);
  if (logLevel <= 3) errorEvent(nodeId, logLevel * 10, message);
}

void NodeBlueClient::errorEvent(const std::string &nodeId, int32_t level, const std::string &message) {
  try {
    Flows::PINode node = _nodeManager->getNode(nodeId);
    if (!node) return;

    auto messageStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    messageStruct->structValue->emplace("payload", std::make_shared<Flows::Variable>(Flows::VariableType::tStruct));
    auto errorStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    errorStruct->structValue->emplace("message", std::make_shared<Flows::Variable>(message));
    auto sourceStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    sourceStruct->structValue->emplace("id", std::make_shared<Flows::Variable>(nodeId));
    sourceStruct->structValue->emplace("z", std::make_shared<Flows::Variable>(node->getFlowId()));
    sourceStruct->structValue->emplace("type", std::make_shared<Flows::Variable>(node->getType()));
    sourceStruct->structValue->emplace("name", std::make_shared<Flows::Variable>(node->getName()));
    errorStruct->structValue->emplace("source", sourceStruct);
    messageStruct->structValue->emplace("error", errorStruct);

    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(3);
    parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
    parameters->push_back(std::make_shared<Flows::Variable>(level));
    parameters->push_back(messageStruct);

    Flows::PVariable result = invoke("errorEvent", parameters, false);
    if (result->errorStruct) GD::out.printError("Error calling errorEvent: " + result->structValue->at("faultString")->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::frontendEventLog(const std::string &nodeId, const std::string &message) {
  try {
    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(2);
    parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
    parameters->push_back(std::make_shared<Flows::Variable>(message));

    Flows::PVariable result = invoke("frontendEventLog", parameters, false);
    if (result->errorStruct) GD::out.printError("Error calling frontendEventLog: " + result->structValue->at("faultString")->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribePeer(const std::string &nodeId, uint64_t peerId, int32_t channel, const std::string &variable) {
  try {
    std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
    _peerSubscriptions[peerId][channel][variable].insert(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribePeer(const std::string &nodeId, uint64_t peerId, int32_t channel, const std::string &variable) {
  try {
    std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
    _peerSubscriptions[peerId][channel][variable].erase(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribeFlow(const std::string &nodeId, const std::string &flowId) {
  try {
    if (_bl->debugLevel >= 5) _out.printInfo("Debug: Subscribing flow with ID " + flowId + " for node " + nodeId);
    std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
    _flowSubscriptions[flowId].insert(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribeFlow(const std::string &nodeId, const std::string &flowId) {
  try {
    if (_bl->debugLevel >= 5) _out.printInfo("Debug: Unsubscribing flow with ID " + flowId + " for node " + nodeId);
    std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
    _flowSubscriptions[flowId].erase(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribeGlobal(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
    _globalSubscriptions.insert(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribeGlobal(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
    _globalSubscriptions.erase(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribeHomegearEvents(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
    _homegearEventSubscriptions.insert(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribeHomegearEvents(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
    _homegearEventSubscriptions.erase(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribeStatusEvents(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> eventSubscriptionsGuard(_statusEventSubscriptionsMutex);
    _statusEventSubscriptions.insert(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribeStatusEvents(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> eventSubscriptionsGuard(_statusEventSubscriptionsMutex);
    _statusEventSubscriptions.erase(nodeId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::subscribeErrorEvents(const std::string &nodeId, bool catchConfigurationNodeErrors, bool hasScope, bool ignoreCaught) {
  try {
    ErrorEventSubscription errorEventSubscription;
    errorEventSubscription.nodeId = nodeId;
    errorEventSubscription.catchConfigurationNodeErrors = catchConfigurationNodeErrors;
    errorEventSubscription.hasScope = hasScope;
    errorEventSubscription.ignoreCaught = ignoreCaught;

    std::lock_guard<std::mutex> eventSubscriptionsGuard(_errorEventSubscriptionsMutex);
    _errorEventSubscriptions.emplace_back(errorEventSubscription);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::unsubscribeErrorEvents(const std::string &nodeId) {
  try {
    std::lock_guard<std::mutex> eventSubscriptionsGuard(_errorEventSubscriptionsMutex);
    for (auto i = _errorEventSubscriptions.begin(); i != _errorEventSubscriptions.end(); ++i) {
      if (i->nodeId == nodeId) {
        _errorEventSubscriptions.erase(i);
        break;
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::queueOutput(const std::string &nodeId, uint32_t index, Flows::PVariable message, bool synchronous) {
  try {
    if (!message || _shuttingDownOrRestarting) return;

    Flows::PNodeInfo nodeInfo;
    {
      std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
      auto nodesIterator = _nodes.find(nodeId);
      if (nodesIterator == _nodes.end()) return;
      nodeInfo = nodesIterator->second;
    }

    {
      std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
      auto internalMessagesIterator = _internalMessages.find(nodeId);
      if (internalMessagesIterator != _internalMessages.end()) {
        message->structValue->emplace("_internal", internalMessagesIterator->second);
        if (!synchronous) {
          auto synchronousOutputIterator = internalMessagesIterator->second->structValue->find("synchronousOutput");
          if (synchronousOutputIterator != internalMessagesIterator->second->structValue->end()) {
            synchronous = synchronousOutputIterator->second->booleanValue;
          }
        }
      }
    }

    if (message->structValue->find("payload") == message->structValue->end()) message->structValue->emplace("payload", std::make_shared<Flows::Variable>());

    if (index >= nodeInfo->wiresOut.size()) {
      _out.printError("Error: " + nodeId + " has no output with index " + std::to_string(index) + ".");
      return;
    }

    std::string eventFlowId;
    {
      std::lock_guard<std::mutex> eventFlowIdGuard(_eventFlowIdMutex);
      eventFlowId = _eventFlowId;
    }

    if (synchronous && GD::bl->settings.nodeBlueDebugOutput()) {
      //Output node events before execution of input
      if (_frontendConnected && _startUpComplete && nodeInfo->info->structValue->at("flow")->stringValue == eventFlowId) {
        Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
        nodeEvent(nodeId, "highlightNode/" + nodeId, timeout, false);
        Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
        nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex, false);

        Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        std::string statusText = message->structValue->at("payload")->toString();
        if (statusText.size() > 20) statusText = statusText.substr(0, 17) + "...";
        statusText = std::to_string(index) + ": " + BaseLib::HelperFunctions::stripNonPrintable(statusText);
        status->structValue->emplace("text", std::make_shared<Flows::Variable>(statusText));
        nodeEvent(nodeId, "statusTop/" + nodeId, status, false);
      }
    }

    message->structValue->emplace("source", std::make_shared<Flows::Variable>(nodeId));
    for (auto &node: nodeInfo->wiresOut.at(index)) {
      Flows::PNodeInfo outputNodeInfo;
      {
        std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
        auto nodeIterator = _nodes.find(node.id);
        if (nodeIterator == _nodes.end()) continue;
        outputNodeInfo = nodeIterator->second;
      }
      if (synchronous) {
        Flows::PINode nextNode = _nodeManager->getNode(outputNodeInfo->id);
        if (nextNode) {
          if (GD::bl->settings.nodeBlueDebugOutput() && _startUpComplete && _frontendConnected && nextNode->getFlowId() == eventFlowId && BaseLib::HelperFunctions::getTime() - outputNodeInfo->lastNodeEvent1 >= 100) {
            outputNodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
            Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
            timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
            nodeEvent(outputNodeInfo->id, "highlightNode/" + outputNodeInfo->id, timeout, false);
          }
          auto internalMessageIterator = message->structValue->find("_internal");
          if (internalMessageIterator != message->structValue->end()) {
            //Emplace makes sure, that synchronousOutput stays false when set to false in node. This is the only way to make a synchronous output asynchronous again.
            internalMessageIterator->second->structValue->emplace("synchronousOutput", std::make_shared<Flows::Variable>(true));
            setInternalMessage(outputNodeInfo->id, internalMessageIterator->second);
          } else {
            Flows::PVariable internalMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
            internalMessage->structValue->emplace("synchronousOutput", std::make_shared<Flows::Variable>(true));
            setInternalMessage(outputNodeInfo->id, internalMessage);
            message->structValue->emplace("_internal", internalMessage);
          }

          {
            std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
            auto nodeIterator = _fixedInputValues.find(outputNodeInfo->id);
            if (nodeIterator != _fixedInputValues.end()) {
              auto inputIterator = nodeIterator->second.find(node.port);
              if (inputIterator != nodeIterator->second.end()) {
                auto messageCopy = std::make_shared<Flows::Variable>();
                *messageCopy = *message;
                (*messageCopy->structValue)["payload"] = inputIterator->second;
                message = messageCopy;
              }
            }
          }

          if (_nodeManager->isNodeRedNode(nextNode->getType())) {
            auto parameters = std::make_shared<Flows::Array>();
            parameters->reserve(5);
            parameters->emplace_back(std::make_shared<Flows::Variable>(nextNode->getId()));
            parameters->emplace_back(outputNodeInfo->serialize());
            parameters->emplace_back(std::make_shared<Flows::Variable>(node.port));
            parameters->emplace_back(message);
            parameters->emplace_back(std::make_shared<Flows::Variable>(true));
            auto result = invoke("nodeRedNodeInput", parameters, true);
            if (result->errorStruct) _out.printError("Error calling \"nodeRedNodeInput\": " + result->structValue->at("faultString")->stringValue);
          } else {
            std::lock_guard<std::mutex> nodeInputGuard(nextNode->getInputMutex());
            nextNode->input(outputNodeInfo, node.port, message);
          }

          setInputValue(outputNodeInfo->type, outputNodeInfo->id, node.port, message);
        }
      } else {
        if (queueSize(2) > 1000 && BaseLib::HelperFunctions::getTime() - _lastQueueSize > 1000) {
          _lastQueueSize = BaseLib::HelperFunctions::getTime();
          if (_startUpComplete) _out.printWarning("Warning: Node output queue has " + std::to_string(queueSize(2)) + " entries.");
          else _out.printInfo("Info: Node output queue has " + std::to_string(queueSize(2)) + " entries.");
        }
        std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(outputNodeInfo, node.port, message);
        if (!enqueue(2, queueEntry, !_startUpComplete)) {
          printQueueFullError(_out, "Error: Dropping output of node " + nodeId + ". Queue is full.");
          return;
        }
      }
    }

    if (!synchronous && GD::bl->settings.nodeBlueDebugOutput()) {
      if (_frontendConnected && _startUpComplete && nodeInfo->info->structValue->at("flow")->stringValue == eventFlowId && BaseLib::HelperFunctions::getTime() - nodeInfo->lastNodeEvent1 >= 100) {
        nodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
        Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
        nodeEvent(nodeId, "highlightNode/" + nodeId, timeout, false);
        Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
        nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex, false);

        Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        std::string statusText = message->structValue->at("payload")->toString();
        if (statusText.size() > 20) statusText = statusText.substr(0, 17) + "...";
        statusText = std::to_string(index) + ": " + BaseLib::HelperFunctions::stripNonPrintable(statusText);
        status->structValue->emplace("text", std::make_shared<Flows::Variable>(statusText));
        nodeEvent(nodeId, "statusTop/" + nodeId, status, false);
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::nodeEvent(const std::string &nodeId, const std::string &topic, const Flows::PVariable &value, bool retain) {
  try {
    if (!value) return; //Allow nodeEvent even if startup is not complete yet (don't check _startUpComplete here)

    if (!_frontendConnected && !retain) return;

    auto node = _nodeManager->getNode(nodeId);
    auto sourceStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    sourceStruct->structValue->emplace("id", std::make_shared<Flows::Variable>(node->getId()));
    sourceStruct->structValue->emplace("z", std::make_shared<Flows::Variable>(node->getFlowId()));
    sourceStruct->structValue->emplace("name", std::make_shared<Flows::Variable>(node->getName()));
    value->structValue->emplace("source", sourceStruct);

    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(4);
    parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
    parameters->push_back(std::make_shared<Flows::Variable>(topic));
    parameters->push_back(value);
    parameters->push_back(std::make_shared<Flows::Variable>(retain));

    Flows::PVariable result = invoke("nodeEvent", parameters, false);
    if (result->errorStruct) GD::out.printError("Error calling nodeEvent: " + result->structValue->at("faultString")->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PVariable NodeBlueClient::getNodeData(const std::string &nodeId, const std::string &key) {
  try {
    if (key == "credentials") {
      Flows::PArray parameters = std::make_shared<Flows::Array>();
      parameters->push_back(std::make_shared<Flows::Variable>(nodeId));

      Flows::PVariable result = invoke("getCredentials", parameters, true);
      if (result->errorStruct) {
        GD::out.printError("Error calling getCredentials: " + result->structValue->at("faultString")->stringValue);
        return std::make_shared<Flows::Variable>();
      }
      return result;
    } else {
      Flows::PArray parameters = std::make_shared<Flows::Array>();
      parameters->reserve(2);
      parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
      parameters->push_back(std::make_shared<Flows::Variable>(key));

      Flows::PVariable result = invoke("getNodeData", parameters, true);
      if (result->errorStruct) {
        GD::out.printError("Error calling getNodeData: " + result->structValue->at("faultString")->stringValue);
        return std::make_shared<Flows::Variable>();
      }
      return result;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<Flows::Variable>();
}

void NodeBlueClient::setNodeData(const std::string &nodeId, const std::string &key, Flows::PVariable value) {
  try {
    if (key == "credentials") {
      Flows::PArray parameters = std::make_shared<Flows::Array>();
      parameters->reserve(2);
      parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
      parameters->push_back(value);

      invoke("setCredentials", parameters, true);
    } else if (key == "credentialTypes") {
      Flows::PArray parameters = std::make_shared<Flows::Array>();
      parameters->reserve(2);
      parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
      parameters->push_back(value);

      invoke("setCredentialTypes", parameters, true);
    } else {
      Flows::PArray parameters = std::make_shared<Flows::Array>();
      parameters->reserve(3);
      parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
      parameters->push_back(std::make_shared<Flows::Variable>(key));
      parameters->push_back(value);

      invoke("setNodeData", parameters, true);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PVariable NodeBlueClient::getFlowData(const std::string &flowId, const std::string &key) {
  try {
    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(2);
    parameters->push_back(std::make_shared<Flows::Variable>(flowId));
    parameters->push_back(std::make_shared<Flows::Variable>(key));

    Flows::PVariable result = invoke("getFlowData", parameters, true);
    if (result->errorStruct) {
      GD::out.printError("Error calling getFlowData: " + result->structValue->at("faultString")->stringValue);
      return std::make_shared<Flows::Variable>();
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<Flows::Variable>();
}

void NodeBlueClient::setFlowData(const std::string &flowId, const std::string &key, Flows::PVariable value) {
  try {
    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(3);
    parameters->push_back(std::make_shared<Flows::Variable>(flowId));
    parameters->push_back(std::make_shared<Flows::Variable>(key));
    parameters->push_back(value);

    invoke("setFlowData", parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PVariable NodeBlueClient::getGlobalData(const std::string &key) {
  try {
    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->push_back(std::make_shared<Flows::Variable>(key));

    Flows::PVariable result = invoke("getGlobalData", parameters, true);
    if (result->errorStruct) {
      GD::out.printError("Error calling getGlobalData: " + result->structValue->at("faultString")->stringValue);
      return std::make_shared<Flows::Variable>();
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<Flows::Variable>();
}

void NodeBlueClient::setGlobalData(const std::string &key, Flows::PVariable value) {
  try {
    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(2);
    parameters->push_back(std::make_shared<Flows::Variable>(key));
    parameters->push_back(value);

    invoke("setGlobalData", parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::setInternalMessage(const std::string &nodeId, Flows::PVariable message) {
  try {
    std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
    auto internalMessageIterator = _internalMessages.find(nodeId);
    if (internalMessageIterator != _internalMessages.end()) {
      if (internalMessageIterator->second == message) return;
      Flows::PVariable copy = std::make_shared<Flows::Variable>();
      *copy = *internalMessageIterator->second;
      for (auto element: *message->structValue) {
        copy->structValue->erase(element.first);
        copy->structValue->emplace(element.first, element.second);
      }
      _internalMessages[nodeId] = copy;
    } else _internalMessages[nodeId] = message;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueClient::setInputValue(const std::string &nodeType, const std::string &nodeId, int32_t inputIndex, const Flows::PVariable &message) {
  try {
    std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
    auto &element = _inputValues[nodeId][inputIndex];
    InputValue inputValue;
    inputValue.time = BaseLib::HelperFunctions::getTime();
    inputValue.value = std::make_shared<Flows::Variable>();
    *inputValue.value = nodeType == "unit-test-helper" ? *message : *(message->structValue->at("payload"));

    if (inputValue.value->type == Flows::VariableType::tArray || inputValue.value->type == Flows::VariableType::tStruct) {
      inputValue.value = std::make_shared<Flows::Variable>(Flows::JsonEncoder::getString(inputValue.value));
    }
    if (inputValue.value->type == Flows::VariableType::tBinary) {
      if (inputValue.value->binaryValue.size() > 10) inputValue.value = std::make_shared<Flows::Variable>(BaseLib::HelperFunctions::getHexString(inputValue.value->binaryValue.data(), 10) + "...");
      else inputValue.value = std::make_shared<Flows::Variable>(BaseLib::HelperFunctions::getHexString(inputValue.value->binaryValue.data(), inputValue.value->binaryValue.size()));
    } else if (inputValue.value->type == Flows::VariableType::tString && inputValue.value->stringValue.size() > 20) {
      inputValue.value = std::make_shared<Flows::Variable>(inputValue.value->stringValue.substr(0, 20) + "...");
    }

    element.push_front(std::move(inputValue));
    while (element.size() > GD::bl->settings.nodeBlueFrontendHistorySize()) element.pop_back();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PVariable NodeBlueClient::getConfigParameter(const std::string &nodeId, const std::string &name) {
  try {
    Flows::PINode node = _nodeManager->getNode(nodeId);
    if (node) return node->getConfigParameterIncoming(std::move(name));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<Flows::Variable>();
}

void NodeBlueClient::watchdog() {
  while (!_stopped && !_shuttingDownOrRestarting) {
    try {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      if (_processingThreadCount1 == _threadCount) {
        int64_t time = _processingThreadCountMaxReached1;
        if (time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.nodeBlueWatchdogTimeout()) {
          std::cout << "Watchdog 1 timed out. Killing process." << std::endl;
          std::cerr << "Watchdog 1 timed out. Killing process." << std::endl;
          kill(getpid(), 9);
        }
      }

      if (_processingThreadCount2 == _threadCount) {
        int64_t time = _processingThreadCountMaxReached2;
        if (time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.nodeBlueWatchdogTimeout()) {
          std::cout << "Watchdog 2 timed out. Killing process." << std::endl;
          std::cerr << "Watchdog 2 timed out. Killing process." << std::endl;
          kill(getpid(), 9);
        }
      }

      if (_processingThreadCount3 == _threadCount) {
        int64_t time = _processingThreadCountMaxReached3;
        if (time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.nodeBlueWatchdogTimeout()) {
          std::cout << "Watchdog 3 timed out. Killing process." << std::endl;
          std::cerr << "Watchdog 3 timed out. Killing process." << std::endl;
          kill(getpid(), 9);
        }
      }
    }
    catch (const std::exception &ex) {
      _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
  }
}

// {{{ RPC methods
Flows::PVariable NodeBlueClient::reload(Flows::PArray &parameters) {
  try {
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.log").c_str(), "a", stdout)) {
      GD::out.printError("Error: Could not redirect output to new log file.");
    }
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.err").c_str(), "a", stderr)) {
      GD::out.printError("Error: Could not redirect errors to new log file.");
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::shutdown(Flows::PArray &parameters) {
  try {
    if (_maintenanceThread.joinable()) _maintenanceThread.join();
    _maintenanceThread = std::thread(&NodeBlueClient::dispose, this);

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::lifetick(Flows::PArray &parameters) {
  try {
    for (int32_t i = 0; i < _queueCount; i++) {
      if (queueSize(i) > 1000) {
        _out.printError("Error in lifetick: More than 1000 items are queued in queue number " + std::to_string(i));
        return std::make_shared<Flows::Variable>(false);
      }
    }

    return std::make_shared<Flows::Variable>(true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::getLoad(Flows::PArray &parameters) {
  try {
    auto loadStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);

    for (int32_t i = 0; i < _queueCount; i++) {
      auto queueStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);

      queueStruct->structValue->emplace("Max load", std::make_shared<Flows::Variable>(maxThreadLoad(i)));
      queueStruct->structValue->emplace("Max load 1m", std::make_shared<Flows::Variable>(maxThreadLoad1m(i)));
      queueStruct->structValue->emplace("Max load 10m", std::make_shared<Flows::Variable>(maxThreadLoad10m(i)));
      queueStruct->structValue->emplace("Max load 1h", std::make_shared<Flows::Variable>(maxThreadLoad1h(i)));
      queueStruct->structValue->emplace("Max latency", std::make_shared<Flows::Variable>(maxWait(i)));
      queueStruct->structValue->emplace("Max latency 1m", std::make_shared<Flows::Variable>(maxWait1m(i)));
      queueStruct->structValue->emplace("Max latency 10m", std::make_shared<Flows::Variable>(maxWait10m(i)));
      queueStruct->structValue->emplace("Max latency 1h", std::make_shared<Flows::Variable>(maxWait1h(i)));

      loadStruct->structValue->emplace("Queue " + std::to_string(i + 1), queueStruct);
    }

    return loadStruct;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::startFlow(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
    if (_disposed) return Flows::Variable::createError(-1, "Client is disposing.");

    _startUpComplete = false;

    std::set<Flows::PNodeInfo> nodes;
    PFlowInfoClient flow = std::make_shared<FlowInfoClient>();
    flow->id = parameters->at(0)->integerValue;
    flow->flow = parameters->at(1)->arrayValue->at(0);
    std::string flowId;
    for (auto &element: *parameters->at(1)->arrayValue) {
      auto flowIdIterator = element->structValue->find("flow");
      Flows::Struct::const_iterator idIterator = element->structValue->find("id"); //Check if node is a subflow node. Ignore those.
      if (flowIdIterator != element->structValue->end() && idIterator != element->structValue->end() && idIterator->second->stringValue.find(':') == std::string::npos) {
        flowId = flowIdIterator->second->stringValue;
        break;
      }
    }

    _out.printInfo("Info: Starting flow with ID " + std::to_string(flow->id) + " (" + flowId + ")...");

    for (auto &element: *parameters->at(1)->arrayValue) {
      Flows::PNodeInfo node = std::make_shared<Flows::NodeInfo>();

      auto idIterator = element->structValue->find("id");
      if (idIterator == element->structValue->end()) {
        GD::out.printError("Error: Flow element has no id.");
        continue;
      }

      auto flowIdIterator = element->structValue->find("flow"); //For subflows this is not flowId
      if (flowIdIterator == element->structValue->end()) {
        GD::out.printError("Error: Flow element has no flow ID.");
        continue;
      }

      auto typeIterator = element->structValue->find("type");
      if (typeIterator == element->structValue->end()) {
        GD::out.printError("Error: Flow element has no type.");
        continue;
      }
      if (typeIterator->second->stringValue.compare(0, 8, "subflow:") == 0) continue;

      node->id = idIterator->second->stringValue;
      node->flowId = flowIdIterator->second->stringValue;
      node->type = typeIterator->second->stringValue;
      node->info = element;

      auto wiresIterator = element->structValue->find("wires");
      if (wiresIterator != element->structValue->end()) {
        node->wiresOut.reserve(wiresIterator->second->arrayValue->size());
        for (auto &outputIterator: *wiresIterator->second->arrayValue) {
          std::vector<Flows::Wire> output;
          output.reserve(outputIterator->structValue->size());
          for (auto &wireIterator: *outputIterator->arrayValue) {
            auto innerIdIterator = wireIterator->structValue->find("id");
            if (innerIdIterator == wireIterator->structValue->end()) continue;
            uint32_t port = 0;
            auto portIterator = wireIterator->structValue->find("port");
            if (portIterator != wireIterator->structValue->end()) {
              if (portIterator->second->type == Flows::VariableType::tString) port = static_cast<uint32_t>(BaseLib::Math::getNumber(portIterator->second->stringValue));
              else port = static_cast<uint32_t>(portIterator->second->integerValue);
            }
            Flows::Wire wire;
            wire.id = innerIdIterator->second->stringValue;
            wire.port = port;
            output.push_back(wire);
          }
          node->wiresOut.push_back(output);
        }
      }

      flow->nodes.emplace(node->id, node);
      nodes.emplace(node);
      std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
      _nodes.emplace(node->id, node);
    }

    if (flow->nodes.empty()) {
      _out.printWarning("Warning: Not starting flow with ID " + std::to_string(flow->id) + ", because it has no nodes.");
      return Flows::Variable::createError(-100, "Flow has no nodes.");
    }

    //{{{ Set wiresIn
    for (auto &node: flow->nodes) {
      int32_t outputIndex = 0;
      for (auto &output: node.second->wiresOut) {
        for (auto &wireOut: output) {
          auto nodeIterator = flow->nodes.find(wireOut.id);
          if (nodeIterator == flow->nodes.end()) continue;
          if (nodeIterator->second->wiresIn.size() <= wireOut.port) {
            nodeIterator->second->wiresIn.resize(wireOut.port + 1);
          }
          Flows::Wire wire;
          wire.id = node.second->id;
          wire.port = static_cast<uint32_t>(outputIndex);
          nodeIterator->second->wiresIn.at(wireOut.port).push_back(wire);
        }
        outputIndex++;
      }
    }
    //}}}

    _nodesStopped = false;

    //{{{ Init nodes
    {
      std::set<std::string> nodesToRemove;
      for (auto &node: nodes) {
        if (_bl->debugLevel >= 5) _out.printDebug("Starting node " + node->id + " of type " + node->type + ".");

        Flows::PINode nodeObject;
        int32_t result = _nodeManager->loadNode(node->type, node->id, nodeObject);
        if (result < 0) {
          _out.printError("Error: Could not load node " + node->type + ". Error code: " + std::to_string(result));
          continue;
        }

        if (!nodeObject) continue;

        nodeObject->setId(node->id);
        nodeObject->setFlowId(node->flowId);
        auto nameIterator = node->info->structValue->find("name");
        if (nameIterator != node->info->structValue->end()) nodeObject->setName(nameIterator->second->stringValue);

        nodeObject->setLog(std::function<void(const std::string &, int32_t, const std::string &)>(std::bind(&NodeBlueClient::log, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        nodeObject->setInvoke(std::function<Flows::PVariable(const std::string &, Flows::PArray)>(std::bind(&NodeBlueClient::invoke, this, std::placeholders::_1, std::placeholders::_2, true)));
        nodeObject->setInvokeNodeMethod(std::function<Flows::PVariable(const std::string &, const std::string &, Flows::PArray, bool)>(std::bind(&NodeBlueClient::invokeNodeMethod,
                                                                                                                                                 this,
                                                                                                                                                 std::placeholders::_1,
                                                                                                                                                 std::placeholders::_2,
                                                                                                                                                 std::placeholders::_3,
                                                                                                                                                 std::placeholders::_4)));
        nodeObject->setSubscribePeer(std::function<void(const std::string &, uint64_t, int32_t, const std::string &)>(std::bind(&NodeBlueClient::subscribePeer,
                                                                                                                                this,
                                                                                                                                std::placeholders::_1,
                                                                                                                                std::placeholders::_2,
                                                                                                                                std::placeholders::_3,
                                                                                                                                std::placeholders::_4)));
        nodeObject->setUnsubscribePeer(std::function<void(const std::string &, uint64_t, int32_t, const std::string &)>(std::bind(&NodeBlueClient::unsubscribePeer,
                                                                                                                                  this,
                                                                                                                                  std::placeholders::_1,
                                                                                                                                  std::placeholders::_2,
                                                                                                                                  std::placeholders::_3,
                                                                                                                                  std::placeholders::_4)));
        nodeObject->setSubscribeFlow(std::function<void(const std::string &, const std::string &)>(std::bind(&NodeBlueClient::subscribeFlow, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setUnsubscribeFlow(std::function<void(const std::string &, const std::string &)>(std::bind(&NodeBlueClient::unsubscribeFlow, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setSubscribeGlobal(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::subscribeGlobal, this, std::placeholders::_1)));
        nodeObject->setUnsubscribeGlobal(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::unsubscribeGlobal, this, std::placeholders::_1)));
        nodeObject->setSubscribeHomegearEvents(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::subscribeHomegearEvents, this, std::placeholders::_1)));
        nodeObject->setUnsubscribeHomegearEvents(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::unsubscribeHomegearEvents, this, std::placeholders::_1)));
        nodeObject->setSubscribeStatusEvents(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::subscribeStatusEvents, this, std::placeholders::_1)));
        nodeObject->setUnsubscribeStatusEvents(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::unsubscribeStatusEvents, this, std::placeholders::_1)));
        nodeObject->setSubscribeErrorEvents(std::function<void(const std::string &, bool, bool, bool)>(std::bind(&NodeBlueClient::subscribeErrorEvents,
                                                                                                                 this,
                                                                                                                 std::placeholders::_1,
                                                                                                                 std::placeholders::_2,
                                                                                                                 std::placeholders::_3,
                                                                                                                 std::placeholders::_4)));
        nodeObject->setUnsubscribeErrorEvents(std::function<void(const std::string &)>(std::bind(&NodeBlueClient::unsubscribeErrorEvents, this, std::placeholders::_1)));
        nodeObject->setOutput(std::function<void(const std::string &, uint32_t, Flows::PVariable, bool)>(std::bind(&NodeBlueClient::queueOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
        nodeObject->setNodeEvent(std::function<void(const std::string &, const std::string &, const Flows::PVariable &, bool)>(std::bind(&NodeBlueClient::nodeEvent,
                                                                                                                                         this,
                                                                                                                                         std::placeholders::_1,
                                                                                                                                         std::placeholders::_2,
                                                                                                                                         std::placeholders::_3,
                                                                                                                                         std::placeholders::_4)));
        nodeObject->setGetNodeData(std::function<Flows::PVariable(const std::string &, const std::string &)>(std::bind(&NodeBlueClient::getNodeData, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setSetNodeData(std::function<void(const std::string &, const std::string &, Flows::PVariable)>(std::bind(&NodeBlueClient::setNodeData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        nodeObject->setGetFlowData(std::function<Flows::PVariable(const std::string &, const std::string &)>(std::bind(&NodeBlueClient::getFlowData, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setSetFlowData(std::function<void(const std::string &, const std::string &, Flows::PVariable)>(std::bind(&NodeBlueClient::setFlowData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        nodeObject->setGetGlobalData(std::function<Flows::PVariable(const std::string &)>(std::bind(&NodeBlueClient::getGlobalData, this, std::placeholders::_1)));
        nodeObject->setSetGlobalData(std::function<void(const std::string &, Flows::PVariable)>(std::bind(&NodeBlueClient::setGlobalData, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setSetInternalMessage(std::function<void(const std::string &, Flows::PVariable)>(std::bind(&NodeBlueClient::setInternalMessage, this, std::placeholders::_1, std::placeholders::_2)));
        nodeObject->setGetConfigParameter(std::function<Flows::PVariable(const std::string &, const std::string &)>(std::bind(&NodeBlueClient::getConfigParameter, this, std::placeholders::_1, std::placeholders::_2)));

        try {
          if (!nodeObject->init(node)) {
            nodeObject.reset();
            nodesToRemove.emplace(node->id);
            _out.printError("Error: Could not load node " + node->type + " with ID " + node->id + ". \"init\" failed.");
            continue;
          }
        } catch (const std::exception &ex) {
          _out.printError("Error within init() of node " + node->id + ": " + ex.what());
        } catch (...) {
          _out.printError("Error without further information within init() of node " + node->id);
        }
      }
      for (auto &node: nodesToRemove) {
        flow->nodes.erase(node);
        {
          std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
          _nodes.erase(node);
        }
        _nodeManager->unloadNode(node);
      }
    }
    //}}}

    //{{{ Remove non-existant fixed inputs and input values
    {
      std::unique_lock<std::mutex> nodesGuard(_nodesMutex, std::defer_lock);
      std::unique_lock<std::mutex> fixedInputGuard(_fixedInputValuesMutex, std::defer_lock);
      std::lock(nodesGuard, fixedInputGuard);
      std::unordered_set<std::string> entriesToRemove;
      for (auto &entry: _fixedInputValues) {
        if (_nodes.find(entry.first) == _nodes.end()) entriesToRemove.emplace(entry.first);
      }
      nodesGuard.unlock();
      for (auto &entry: entriesToRemove) {
        _fixedInputValues.erase(entry);
      }
    }

    {
      std::unique_lock<std::mutex> nodesGuard(_nodesMutex, std::defer_lock);
      std::unique_lock<std::mutex> inputValuesGuard(_inputValuesMutex, std::defer_lock);
      std::lock(nodesGuard, inputValuesGuard);
      std::unordered_set<std::string> entriesToRemove;
      for (auto &entry: _inputValues) {
        if (_nodes.find(entry.first) == _nodes.end()) entriesToRemove.emplace(entry.first);
      }
      nodesGuard.unlock();
      for (auto &entry: entriesToRemove) {
        _inputValues.erase(entry);
      }
    }
    //}}}

    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    _flows.emplace(flow->id, flow);

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::startNodes(Flows::PArray &parameters) {
  try {
    //Sort error event subscriptions
    {
      std::lock_guard<std::mutex> eventSubscriptionsGuard(_errorEventSubscriptionsMutex);
      _errorEventSubscriptions.sort([](const ErrorEventSubscription &a, const ErrorEventSubscription &b) {
        if (a.hasScope && !b.hasScope) {
          return true;
        } else if (!a.hasScope && b.hasScope) {
          return false;
        } else if (a.hasScope && b.hasScope) {
          return false;
        } else if (a.ignoreCaught && !b.ignoreCaught) {
          return false;
        } else if (!a.ignoreCaught && b.ignoreCaught) {
          return true;
        }
        return false;
      });
    }

    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    for (auto &flow: _flows) {
      std::set<std::string> nodesToRemove;
      for (auto &nodeIterator: flow.second->nodes) {
        Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
        if (node) {
          try {
            if (!node->start()) {
              node.reset();
              nodesToRemove.emplace(nodeIterator.second->id);
              _out.printError("Error: Could not load node " + nodeIterator.second->type + " with ID " + nodeIterator.second->id + ". \"start\" failed.");
            }
          } catch (const std::exception &ex) {
            _out.printError("Error within start() of node " + nodeIterator.second->id + ": " + ex.what());
          } catch (...) {
            _out.printError("Error without further information within start() of node " + nodeIterator.second->id);
          }
        }
      }
      for (auto &node: nodesToRemove) {
        flow.second->nodes.erase(node);
        {
          std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
          _nodes.erase(node);
        }
        _nodeManager->unloadNode(node);
      }
    }
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::configNodesStarted(Flows::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    for (auto &flow: _flows) {
      for (auto &nodeIterator: flow.second->nodes) {
        Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
        try {
          if (node) node->configNodesStarted();
        }
        catch (const std::exception &ex) {
          _out.printError("Error within configNodesStarted() of node " + nodeIterator.second->id + ": " + ex.what());
        } catch (...) {
          _out.printError("Error without further information within configNodesStarted() of node " + nodeIterator.second->id);
        }
      }
    }
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::startUpComplete(Flows::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    for (auto &flow: _flows) {
      for (auto &nodeIterator: flow.second->nodes) {
        Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
        try {
          if (node) node->startUpComplete();
        }
        catch (const std::exception &ex) {
          _out.printError("Error within startUpComplete() of node " + nodeIterator.second->id + ": " + ex.what());
        } catch (...) {
          _out.printError("Error without further information within startUpComplete() of node " + nodeIterator.second->id);
        }
      }
    }
    _startUpComplete = true;
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::stopNodes(Flows::PArray &parameters) {
  try {
    _shuttingDownOrRestarting = true;
    _out.printMessage("Calling stop()...");
    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    int64_t startTime = 0;
    for (auto &flow: _flows) {
      for (auto &nodeIterator: flow.second->nodes) {
        Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
        if (_bl->debugLevel >= 5) _out.printDebug("Debug: Calling stop() on node " + nodeIterator.second->id + "...");
        startTime = BaseLib::HelperFunctions::getTime();
        try {
          if (node) node->stop();
        } catch (const std::exception &ex) {
          _out.printError("Error within stop() of node " + nodeIterator.second->id + ": " + ex.what());
        } catch (...) {
          _out.printError("Error without further information within stop() of node " + nodeIterator.second->id);
        }
        if (BaseLib::HelperFunctions::getTime() - startTime > 100)
          _out.printWarning(
              "Warning: Stop of node " + nodeIterator.second->id + " of type " + nodeIterator.second->type + " in namespace " + nodeIterator.second->type + " in flow " + nodeIterator.second->info->structValue->at("flow")->stringValue
                  + " took longer than 100ms.");
        if (_bl->debugLevel >= 5) _out.printDebug("Debug: Node " + nodeIterator.second->id + " stopped.");
      }
    }
    _out.printMessage("Call to stop() completed.");
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::waitForNodesStopped(Flows::PArray &parameters) {
  try {
    _shuttingDownOrRestarting = true;
    _out.printMessage("Calling waitForStop()...");
    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    int64_t startTime = 0;
    for (auto &flow: _flows) {
      for (auto &nodeIterator: flow.second->nodes) {
        Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
        if (_bl->debugLevel >= 4) _out.printInfo("Info: Waiting for node " + nodeIterator.second->id + " to stop...");
        startTime = BaseLib::HelperFunctions::getTime();
        try {
          if (node) node->waitForStop();
        } catch (const std::exception &ex) {
          _out.printError("Error within waitForStop() of node " + nodeIterator.second->id + ": " + ex.what());
        } catch (...) {
          _out.printError("Error without further information within waitForStop() of node " + nodeIterator.second->id);
        }
        if (BaseLib::HelperFunctions::getTime() - startTime > 1500)
          _out.printWarning(
              "Warning: Waiting for stop on node " + nodeIterator.second->id + " of type " + nodeIterator.second->type + " in namespace " + nodeIterator.second->type + " in flow " + nodeIterator.second->info->structValue->at("flow")->stringValue
                  + " took longer than 1.5 seconds.");
      }
    }
    _nodesStopped = true;
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::stopFlow(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    auto flowsIterator = _flows.find(parameters->at(0)->integerValue);
    if (flowsIterator == _flows.end()) return Flows::Variable::createError(-100, "Unknown flow.");
    for (auto &node: flowsIterator->second->nodes) {
      {
        std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
        for (auto &peerId: _peerSubscriptions) {
          for (auto &channel: peerId.second) {
            for (auto &variable: channel.second) {
              variable.second.erase(node.first);
            }
          }
        }
      }
      {
        std::lock_guard<std::mutex> flowSubscriptionsGuard(_flowSubscriptionsMutex);
        for (auto &flowId: _flowSubscriptions) {
          flowId.second.erase(node.first);
        }
      }
      {
        std::lock_guard<std::mutex> globalSubscriptionsGuard(_globalSubscriptionsMutex);
        _globalSubscriptions.erase(node.first);
      }
      {
        std::lock_guard<std::mutex> eventSubscriptionsGuard(_homegearEventSubscriptionsMutex);
        _homegearEventSubscriptions.erase(node.first);
      }
      _nodeManager->unloadNode(node.second->id);
    }
    _flows.erase(flowsIterator);
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::flowCount(Flows::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
    return std::make_shared<Flows::Variable>(_flows.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::nodeLog(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    log(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue);
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::nodeOutput(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3 && parameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");

    queueOutput(parameters->at(0)->stringValue, static_cast<uint32_t>(parameters->at(1)->integerValue), parameters->at(2), parameters->size() == 4 ? parameters->at(3)->booleanValue : false);
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::invokeExternalNodeMethod(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");

    Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
    if (!node) return Flows::Variable::createError(-1, "Node is unknown in Node-BLUE client.");
    return node->invokeLocal(parameters->at(1)->stringValue, parameters->at(2)->arrayValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::executePhpNodeBaseMethod(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::string &methodName = parameters->at(1)->stringValue;
    Flows::PArray &innerParameters = parameters->at(2)->arrayValue;

    if (methodName == "log") {
      if (innerParameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
      if (innerParameters->at(2)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 3 is not of type string.");

      log(innerParameters->at(0)->stringValue, innerParameters->at(1)->integerValue, innerParameters->at(2)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "frontendEventLog") {
      if (innerParameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 3 is not of type string.");

      frontendEventLog(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "invokeNodeMethod") {
      if (innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");
      if (innerParameters->at(2)->type != Flows::VariableType::tArray) return Flows::Variable::createError(-1, "Parameter 3 is not of type array.");
      if (innerParameters->at(3)->type != Flows::VariableType::tBoolean) return Flows::Variable::createError(-1, "Parameter 4 is not of type boolean.");

      return invokeNodeMethod(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue, innerParameters->at(2)->arrayValue, innerParameters->at(3)->booleanValue);
    } else if (methodName == "subscribePeer") {
      if (innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
      if (innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
      if (innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

      subscribePeer(innerParameters->at(0)->stringValue, static_cast<uint64_t>(innerParameters->at(1)->integerValue64), innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "unsubscribePeer") {
      if (innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
      if (innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
      if (innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

      unsubscribePeer(innerParameters->at(0)->stringValue, static_cast<uint64_t>(innerParameters->at(1)->integerValue64), innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "subscribeFlow") {
      if (innerParameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");

      subscribeFlow(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "unsubscribeFlow") {
      if (innerParameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");

      unsubscribeFlow(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "subscribeGlobal") {
      if (innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");

      subscribeGlobal(innerParameters->at(0)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "unsubscribeGlobal") {
      if (innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");

      unsubscribeGlobal(innerParameters->at(0)->stringValue);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "output") {
      if (innerParameters->size() != 3 && innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");

      queueOutput(innerParameters->at(0)->stringValue, static_cast<uint32_t>(innerParameters->at(1)->integerValue), innerParameters->at(2), innerParameters->size() == 4 ? innerParameters->at(3)->booleanValue : false);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "nodeEvent") {
      if (innerParameters->size() != 3 && innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");
      if (innerParameters->size() == 4 && innerParameters->at(3)->type != Flows::VariableType::tBoolean) return Flows::Variable::createError(-1, "Parameter 4 is not of type boolean.");

      nodeEvent(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue, innerParameters->at(2), innerParameters->size() == 4 ? innerParameters->at(3)->booleanValue : true);
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "getNodeData") {
      if (innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter is not of type string.");

      return getNodeData(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue);
    } else if (methodName == "setNodeData") {
      if (innerParameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");

      setNodeData(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue, innerParameters->at(1));
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "setInternalMessage") {
      if (innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

      setInternalMessage(parameters->at(0)->stringValue, innerParameters->at(0));
      return std::make_shared<Flows::Variable>();
    } else if (methodName == "getConfigParameter") {
      if (innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
      if (innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter is not of type string.");

      return getConfigParameter(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue);
    }
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::getNodesWithFixedInputs(Flows::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
    auto nodeStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);

    for (auto &node: _fixedInputValues) {
      nodeStruct->structValue->emplace(node.first, std::make_shared<Flows::Variable>(node.second.size()));
    }

    return nodeStruct;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::getNodeProcessingTimes(Flows::PArray &parameters) {
  try {
    std::multimap<uint32_t, const Flows::PNodeInfo> resultMap;

    {
      std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
      for (const auto &node: _nodes) {
        if (!node.second || node.second->processingTimes.empty()) continue;
        auto maxTime = *std::max_element(node.second->processingTimes.begin(), node.second->processingTimes.end());
        resultMap.emplace(maxTime, node.second);
      }
    }

    auto resultArray = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
    resultArray->arrayValue->reserve(resultMap.size());

    for (auto &element : resultMap) {
      auto nodeStruct = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);

      nodeStruct->structValue->emplace("id", std::make_shared<Flows::Variable>(element.second->id));
      nodeStruct->structValue->emplace("type", std::make_shared<Flows::Variable>(element.second->type));

      auto processingTimeArray = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
      processingTimeArray->arrayValue->reserve(element.second->processingTimes.size());
      for (auto &time : element.second->processingTimes) {
        processingTimeArray->arrayValue->emplace_back(std::make_shared<Flows::Variable>(time));
      }
      nodeStruct->structValue->emplace("processingTimes", processingTimeArray);

      resultArray->arrayValue->emplace_back(nodeStruct);
    }

    return resultArray;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::getNodeVariable(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(1)->stringValue.compare(0, sizeof("inputValue") - 1, "inputValue") == 0 && parameters->at(1)->stringValue.size() > 10) {
      std::string indexString = parameters->at(1)->stringValue.substr(10);
      int32_t index = Flows::Math::getNumber(indexString);
      std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
      auto nodeIterator = _inputValues.find(parameters->at(0)->stringValue);
      if (nodeIterator != _inputValues.end()) {
        auto inputIterator = nodeIterator->second.find(index);
        if (inputIterator != nodeIterator->second.end()) return inputIterator->second.front().value;
      }
    } else if (parameters->at(1)->stringValue.compare(0, sizeof("inputHistory") - 1, "inputHistory") == 0 && parameters->at(1)->stringValue.size() > 12) {
      std::string indexString = parameters->at(1)->stringValue.substr(12);
      int32_t index = Flows::Math::getNumber(indexString);
      std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
      auto nodeIterator = _inputValues.find(parameters->at(0)->stringValue);
      if (nodeIterator != _inputValues.end()) {
        auto inputIterator = nodeIterator->second.find(index);
        if (inputIterator != nodeIterator->second.end()) {
          Flows::PVariable array = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
          array->arrayValue->reserve(inputIterator->second.size());
          for (auto &inputValue: inputIterator->second) {
            Flows::PVariable innerArray = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
            innerArray->arrayValue->reserve(2);
            innerArray->arrayValue->push_back(std::make_shared<Flows::Variable>(inputValue.time));
            innerArray->arrayValue->push_back(inputValue.value);
            array->arrayValue->push_back(innerArray);
          }
          return array;
        }
      }
    } else if (parameters->at(1)->stringValue.compare(0, sizeof("fixedInput") - 1, "fixedInput") == 0 && parameters->at(1)->stringValue.size() > 10) {
      std::string indexString = parameters->at(1)->stringValue.substr(10);
      int32_t index = Flows::Math::getNumber(indexString);
      std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
      auto nodeIterator = _fixedInputValues.find(parameters->at(0)->stringValue);
      if (nodeIterator != _fixedInputValues.end()) {
        auto inputIterator = nodeIterator->second.find(index);
        if (inputIterator != nodeIterator->second.end()) {
          std::string type;
          if (inputIterator->second->type == Flows::VariableType::tBoolean) type = "bool";
          else if (inputIterator->second->type == Flows::VariableType::tInteger || inputIterator->second->type == Flows::VariableType::tInteger64) type = "int";
          else if (inputIterator->second->type == Flows::VariableType::tFloat) type = "float";
          else if (inputIterator->second->type == Flows::VariableType::tString) type = "string";
          else if (inputIterator->second->type == Flows::VariableType::tArray) type = "array";
          else if (inputIterator->second->type == Flows::VariableType::tStruct) type = "struct";
          else if (inputIterator->second->type == Flows::VariableType::tBinary) type = "bin";
          else return std::make_shared<Flows::Variable>();

          std::string value = inputIterator->second->toString();
          if (inputIterator->second->type == Flows::VariableType::tArray || inputIterator->second->type == Flows::VariableType::tStruct) {
            Flows::JsonEncoder jsonEncoder;
            value = jsonEncoder.getString(inputIterator->second);
          }

          Flows::PVariable array = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
          array->arrayValue->reserve(2);
          array->arrayValue->push_back(std::make_shared<Flows::Variable>(type));
          array->arrayValue->push_back(std::make_shared<Flows::Variable>(value));

          return array;
        }
      }
    } else {
      Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
      if (node) return node->getNodeVariable(parameters->at(1)->stringValue);
    }
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::getFlowVariable(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::setNodeVariable(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    if (parameters->at(1)->stringValue.compare(0, sizeof("fixedInput") - 1, "fixedInput") == 0 && parameters->at(1)->stringValue.size() > 10) {
      std::string indexString = parameters->at(1)->stringValue.substr(10);
      int32_t index = Flows::Math::getNumber(indexString);

      if (parameters->at(2)->arrayValue->size() != 2 && parameters->at(2)->type != Flows::VariableType::tStruct) {
        std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
        auto fixedInputIterator = _fixedInputValues.find(parameters->at(0)->stringValue);
        if (fixedInputIterator != _fixedInputValues.end()) {
          fixedInputIterator->second.erase(index);
          if (fixedInputIterator->second.empty()) _fixedInputValues.erase(fixedInputIterator);
        }

        return std::make_shared<Flows::Variable>();
      }

      Flows::PVariable message;
      if (parameters->at(2)->arrayValue->size() == 2) {
        std::string payloadType = parameters->at(2)->arrayValue->at(0)->stringValue;
        std::string payload = parameters->at(2)->arrayValue->at(1)->stringValue;

        auto value = std::make_shared<Flows::Variable>();
        if (payloadType == "bool") {
          value->setType(Flows::VariableType::tBoolean);
          value->booleanValue = payload == "true";
        } else if (payloadType == "int") {
          value->setType(Flows::VariableType::tInteger64);
          value->integerValue64 = Flows::Math::getNumber64(payload);
          value->integerValue = (int32_t)value->integerValue64;
          value->floatValue = value->integerValue64;
        } else if (payloadType == "float" || payloadType == "num") {
          value->setType(Flows::VariableType::tFloat);
          value->floatValue = Flows::Math::getDouble(payload);
          value->integerValue = value->floatValue;
          value->integerValue64 = value->floatValue;
        } else if (payloadType == "string" || payloadType == "str") {
          value->setType(Flows::VariableType::tString);
          value->stringValue = payload;
        } else if (payloadType == "array" || payloadType == "struct") {
          value = Flows::JsonDecoder::decode(payload);
        } else if (payloadType == "bin") {
          value->setType(Flows::VariableType::tBinary);
          value->binaryValue = BaseLib::HelperFunctions::getUBinary(payload);
        } else return Flows::Variable::createError(-3, "Unknown payload type.");
        message = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
        message->structValue->emplace("payload", value);

        {
          std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
          _fixedInputValues[parameters->at(0)->stringValue][index] = value;
        }
      } else {
        message = parameters->at(2);

        {
          std::lock_guard<std::mutex> fixedInputGuard(_fixedInputValuesMutex);
          _fixedInputValues[parameters->at(0)->stringValue][index] = message;
        }
      }

      Flows::PNodeInfo nodeInfo;
      {
        std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
        auto nodesIterator = _nodes.find(parameters->at(0)->stringValue);
        if (nodesIterator != _nodes.end()) nodeInfo = nodesIterator->second;
      }

      Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
      if (nodeInfo && node) {
        if (_nodeManager->isNodeRedNode(nodeInfo->type)) {
          auto nodeRedParameters = std::make_shared<Flows::Array>();
          nodeRedParameters->reserve(5);
          nodeRedParameters->emplace_back(std::make_shared<Flows::Variable>(node->getId()));
          nodeRedParameters->emplace_back(nodeInfo->serialize());
          nodeRedParameters->emplace_back(std::make_shared<Flows::Variable>(index));
          nodeRedParameters->emplace_back(message);
          nodeRedParameters->emplace_back(std::make_shared<Flows::Variable>(false));
          auto result = invoke("nodeRedNodeInput", nodeRedParameters, false);
          if (result->errorStruct) _out.printError("Error calling \"nodeRedNodeInput\": " + result->structValue->at("faultString")->stringValue);
        } else {
          std::lock_guard<std::mutex> nodeInputGuard(node->getInputMutex());
          node->input(nodeInfo, index, message);
        }
      }

      return std::make_shared<Flows::Variable>();
    }

    Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
    if (node) node->setNodeVariable(parameters->at(1)->stringValue, parameters->at(2));
    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::setFlowVariable(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(1)->stringValue == "enableEvents" && parameters->at(2)->booleanValue) {
      std::lock_guard<std::mutex> eventFlowGuard(_eventFlowIdMutex);
      _eventFlowId = parameters->at(0)->stringValue;
      _out.printInfo("Info: Events are now sent to flow " + _eventFlowId);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::enableNodeEvents(Flows::PArray &parameters) {
  try {
    _frontendConnected = true;

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::disableNodeEvents(Flows::PArray &parameters) {
  try {
    _frontendConnected = false;

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastEvent(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 6) return Flows::Variable::createError(-1, "Wrong parameter count.");

    auto peerId = static_cast<uint64_t>(parameters->at(1)->integerValue64);
    int32_t channel = parameters->at(2)->integerValue;

    {
      std::string eventType;
      if (peerId == 0) eventType = "systemVariableEvent";
      else if (peerId == 0x50000001) eventType = "nodeBlueVariableEvent";
      else if (channel == -1) eventType = "metadataVariableEvent";
      else eventType = "deviceVariableEvent";

      std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
      for (auto &nodeId: _homegearEventSubscriptions) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->homegearEvent(eventType, parameters);
      }
    }

    {
      std::lock_guard<std::mutex> eventsGuard(_peerSubscriptionsMutex);
      std::string &source = parameters->at(0)->stringValue;

      auto peerIterator = _peerSubscriptions.find(peerId);
      if (peerIterator == _peerSubscriptions.end()) return std::make_shared<Flows::Variable>();

      auto channelIterator = peerIterator->second.find(channel);
      if (channelIterator == peerIterator->second.end()) return std::make_shared<Flows::Variable>();

      for (uint32_t j = 0; j < parameters->at(3)->arrayValue->size(); j++) {
        std::string variableName = parameters->at(3)->arrayValue->at(j)->stringValue;

        auto variableIterator = channelIterator->second.find(variableName);
        if (variableIterator == channelIterator->second.end()) continue;

        Flows::PVariable value = parameters->at(4)->arrayValue->at(j);

        for (auto &nodeId: variableIterator->second) {
          Flows::PINode node = _nodeManager->getNode(nodeId);
          if (node) node->variableEvent(source, peerId, channel, variableName, value, parameters->at(5));
        }
      }
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastFlowVariableEvent(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    {
      std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
      for (const auto &nodeId: _homegearEventSubscriptions) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->homegearEvent("flowVariableEvent", parameters);
      }
    }

    {
      std::lock_guard<std::mutex> eventsGuard(_flowSubscriptionsMutex);
      std::string &flowId = parameters->at(0)->stringValue;

      auto flowIterator = _flowSubscriptions.find(flowId);
      if (flowIterator == _flowSubscriptions.end()) return std::make_shared<Flows::Variable>();

      for (const auto &nodeId: flowIterator->second) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->flowVariableEvent(flowId, parameters->at(1)->stringValue, parameters->at(2));
      }
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastGlobalVariableEvent(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    {
      std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
      for (const auto &nodeId: _homegearEventSubscriptions) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->homegearEvent("globalVariableEvent", parameters);
      }
    }

    {
      std::lock_guard<std::mutex> eventsGuard(_globalSubscriptionsMutex);
      for (const auto &nodeId: _globalSubscriptions) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->globalVariableEvent(parameters->at(0)->stringValue, parameters->at(1));
      }
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastServiceMessage(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    {
      std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
      for (const auto &nodeId: _homegearEventSubscriptions) {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if (node) node->homegearEvent("serviceMessage", parameters);
      }
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastNewDevices(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("newDevices", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastDeleteDevices(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("deleteDevices", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastUpdateDevice(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("updateDevice", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastStatus(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    auto sourceIterator = parameters->at(1)->structValue->find("source");
    if (sourceIterator == parameters->at(1)->structValue->end()) return std::make_shared<Flows::Variable>();
    auto flowIdIterator = sourceIterator->second->structValue->find("z");
    if (flowIdIterator == sourceIterator->second->structValue->end()) return std::make_shared<Flows::Variable>();
    auto &flowId = flowIdIterator->second->stringValue;

    std::lock_guard<std::mutex> eventsGuard(_statusEventSubscriptionsMutex);
    for (const auto &nodeId: _statusEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node->getFlowId() != flowId) continue;
      if (node) node->statusEvent(parameters->at(0)->stringValue, parameters->at(1));
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastError(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    auto errorIterator = parameters->at(2)->structValue->find("error");
    if (errorIterator == parameters->at(2)->structValue->end()) return std::make_shared<Flows::Variable>();
    auto sourceIterator = errorIterator->second->structValue->find("source");
    if (sourceIterator == errorIterator->second->structValue->end()) return std::make_shared<Flows::Variable>();
    auto flowIdIterator = sourceIterator->second->structValue->find("z");
    if (flowIdIterator == sourceIterator->second->structValue->end()) return std::make_shared<Flows::Variable>();
    auto &flowId = flowIdIterator->second->stringValue;

    bool handled = false;

    std::lock_guard<std::mutex> eventsGuard(_errorEventSubscriptionsMutex);
    for (const auto &subscription: _errorEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(subscription.nodeId);
      if (node->getFlowId() != flowId && !subscription.catchConfigurationNodeErrors) continue;
      if (handled && subscription.ignoreCaught) continue;
      if (node->errorEvent(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2))) handled = true;
    }

    return std::make_shared<Flows::Variable>(handled);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastVariableProfileStateChanged(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("variableProfileStateChanged", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastUiNotificationCreated(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("uiNotificationCreated", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastUiNotificationRemoved(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("uiNotificationRemoved", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastUiNotificationAction(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("uiNotificationAction", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable NodeBlueClient::broadcastRawPacketEvent(Flows::PArray &parameters) {
  try {
    if (parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(_homegearEventSubscriptionsMutex);
    for (const auto &nodeId: _homegearEventSubscriptions) {
      Flows::PINode node = _nodeManager->getNode(nodeId);
      if (node) node->homegearEvent("rawPacketEvent", parameters);
    }

    return std::make_shared<Flows::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}