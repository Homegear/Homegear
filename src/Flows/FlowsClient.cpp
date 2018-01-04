/* Copyright 2013-2017 Sathya Laufer
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

#include "FlowsClient.h"

#include <utility>
#include "../GD/GD.h"

namespace Flows
{

FlowsClient::FlowsClient() : IQueue(GD::bl.get(), 3, 100000)
{
    _stopped = false;
    _nodesStopped = false;
    _disposed = false;
    _shuttingDownOrRestarting = false;
    _frontendConnected = false;
    _startUpComplete = false;
    _shutdownComplete = false;
    _lastQueueSize = 0;

    _fileDescriptor = std::make_shared<BaseLib::FileDescriptor>();
    _out.init(GD::bl.get());
    _out.setPrefix("Flows Engine (" + std::to_string(getpid()) + "): ");

    _dummyClientInfo.reset(new BaseLib::RpcClientInfo());

    _nodeManager = std::unique_ptr<NodeManager>(new NodeManager(&_frontendConnected));

    _binaryRpc = std::unique_ptr<Flows::BinaryRpc>(new Flows::BinaryRpc());
    _rpcDecoder = std::unique_ptr<Flows::RpcDecoder>(new Flows::RpcDecoder());
    _rpcEncoder = std::unique_ptr<Flows::RpcEncoder>(new Flows::RpcEncoder(true));

    _localRpcMethods.emplace("reload", std::bind(&FlowsClient::reload, this, std::placeholders::_1));
    _localRpcMethods.emplace("shutdown", std::bind(&FlowsClient::shutdown, this, std::placeholders::_1));
    _localRpcMethods.emplace("startFlow", std::bind(&FlowsClient::startFlow, this, std::placeholders::_1));
    _localRpcMethods.emplace("startNodes", std::bind(&FlowsClient::startNodes, this, std::placeholders::_1));
    _localRpcMethods.emplace("configNodesStarted", std::bind(&FlowsClient::configNodesStarted, this, std::placeholders::_1));
    _localRpcMethods.emplace("startUpComplete", std::bind(&FlowsClient::startUpComplete, this, std::placeholders::_1));
    _localRpcMethods.emplace("stopNodes", std::bind(&FlowsClient::stopNodes, this, std::placeholders::_1));
    _localRpcMethods.emplace("stopFlow", std::bind(&FlowsClient::stopFlow, this, std::placeholders::_1));
    _localRpcMethods.emplace("flowCount", std::bind(&FlowsClient::flowCount, this, std::placeholders::_1));
    _localRpcMethods.emplace("nodeOutput", std::bind(&FlowsClient::nodeOutput, this, std::placeholders::_1));
    _localRpcMethods.emplace("invokeNodeMethod", std::bind(&FlowsClient::invokeExternalNodeMethod, this, std::placeholders::_1));
    _localRpcMethods.emplace("executePhpNodeBaseMethod", std::bind(&FlowsClient::executePhpNodeBaseMethod, this, std::placeholders::_1));
    _localRpcMethods.emplace("getFlowVariable", std::bind(&FlowsClient::getFlowVariable, this, std::placeholders::_1));
    _localRpcMethods.emplace("getNodeVariable", std::bind(&FlowsClient::getNodeVariable, this, std::placeholders::_1));
    _localRpcMethods.emplace("setFlowVariable", std::bind(&FlowsClient::setFlowVariable, this, std::placeholders::_1));
    _localRpcMethods.emplace("setNodeVariable", std::bind(&FlowsClient::setNodeVariable, this, std::placeholders::_1));
    _localRpcMethods.emplace("enableNodeEvents", std::bind(&FlowsClient::enableNodeEvents, this, std::placeholders::_1));
    _localRpcMethods.emplace("disableNodeEvents", std::bind(&FlowsClient::disableNodeEvents, this, std::placeholders::_1));
    _localRpcMethods.emplace("broadcastEvent", std::bind(&FlowsClient::broadcastEvent, this, std::placeholders::_1));
    _localRpcMethods.emplace("broadcastDeleteDevices", std::bind(&FlowsClient::broadcastDeleteDevices, this, std::placeholders::_1));
    _localRpcMethods.emplace("broadcastNewDevices", std::bind(&FlowsClient::broadcastNewDevices, this, std::placeholders::_1));
    _localRpcMethods.emplace("broadcastUpdateDevice", std::bind(&FlowsClient::broadcastUpdateDevice, this, std::placeholders::_1));
}

FlowsClient::~FlowsClient()
{
    dispose();
    if(_maintenanceThread.joinable()) _maintenanceThread.join();
    if(_watchdogThread.joinable()) _watchdogThread.join();
}

void FlowsClient::dispose()
{
    try
    {
        std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
        if(_disposed) return;
        _disposed = true;
        _out.printMessage("Shutting down...");

        int64_t startTime = 0;
        _out.printMessage("Calling waitForStop()...");
        {
            std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
            for(auto& flow : _flows)
            {
                for(auto& nodeIterator : flow.second->nodes)
                {
                    Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                    if(_bl->debugLevel >= 4) _out.printInfo("Info: Waiting for node " + nodeIterator.second->id + " to stop...");
                    startTime = BaseLib::HelperFunctions::getTime();
                    if(node) node->waitForStop();
                    if(BaseLib::HelperFunctions::getTime() - startTime > 1000) _out.printWarning("Warning: Waiting for stop on node " + nodeIterator.second->id + " of type " + nodeIterator.second->type + " in namespace " + nodeIterator.second->type + " in flow " + nodeIterator.second->info->structValue->at("flow")->stringValue + " took longer than 1 second.");
                }
            }
        }

        _out.printMessage("Nodes are stopped. Disposing...");

        GD::bl->shuttingDown = true;
        _stopped = true;

        std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
        for(auto& request : _requestInfo)
        {
            std::unique_lock<std::mutex> waitLock(request.second->waitMutex);
            waitLock.unlock();
            request.second->conditionVariable.notify_all();
        }

        stopQueue(0);
        stopQueue(1);
        stopQueue(2);

        {
            std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
            for(auto& flow : _flows)
            {
                for(auto& node : flow.second->nodes)
                {
                    {
                        std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
                        for(auto& peerId : _peerSubscriptions)
                        {
                            for(auto& channel : peerId.second)
                            {
                                for(auto& variable : channel.second)
                                {
                                    variable.second.erase(node.first);
                                }
                            }
                        }
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

void FlowsClient::resetClient(Flows::PVariable packetId)
{
    try
    {
        _shuttingDownOrRestarting = true;

        std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
        _out.printMessage("Calling waitForStop()...");
        {
            std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
            for(auto& flow : _flows)
            {
                for(auto& nodeIterator : flow.second->nodes)
                {
                    Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                    if(_bl->debugLevel >= 5) _out.printDebug("Debug: Waiting for node " + nodeIterator.second->id + " to stop...");
                    if(node) node->waitForStop();
                }
            }
        }

        _out.printMessage("Nodes are stopped. Stopping queues...");

        {
            std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
            for (auto &request : _requestInfo) {
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
            for(auto& flow : _flows)
            {
                for(auto& node : flow.second->nodes)
                {
                    {
                        std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
                        for(auto& peerId : _peerSubscriptions)
                        {
                            for(auto& channel : peerId.second)
                            {
                                for(auto& variable : channel.second)
                                {
                                    variable.second.erase(node.first);
                                }
                            }
                        }
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
            std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
            _internalMessages.clear();
        }

        {
            std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
            _inputValues.clear();
        }

        _out.printMessage("Reinitializing...");

        if(_watchdogThread.joinable()) _watchdogThread.join();

        _shuttingDownOrRestarting = false;
        _startUpComplete = false;
        _nodesStopped = false;
        _lastQueueSize = 0;

        startQueue(0, false, _threadCount, 0, SCHED_OTHER);
        startQueue(1, false, _threadCount, 0, SCHED_OTHER);
        startQueue(2, false, _threadCount, 0, SCHED_OTHER);

        if(GD::bl->settings.flowsWatchdogTimeout() >= 1000) _watchdogThread = std::thread(&FlowsClient::watchdog, this);

        _out.printMessage("Reset complete.");
        Flows::PVariable result = std::make_shared<Flows::Variable>();
        sendResponse(packetId, result);
        return;
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
    Flows::PVariable result = Flows::Variable::createError(-32500, "Unknown application error.");
    sendResponse(packetId, result);
}

void FlowsClient::start()
{
    try
    {
        if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.log").c_str(), "a", stdout))
        {
            _out.printError("Error: Could not redirect output to log file.");
        }
        if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.err").c_str(), "a", stderr))
        {
            _out.printError("Error: Could not redirect errors to log file.");
        }

        uint32_t flowsProcessingThreadCountNodes = GD::bl->settings.flowsProcessingThreadCountNodes();
        if(flowsProcessingThreadCountNodes < 5) flowsProcessingThreadCountNodes = 5;
        _threadCount = flowsProcessingThreadCountNodes;

        _processingThreadCount1 = 0;
        _processingThreadCount2 = 0;
        _processingThreadCount3 = 0;

        startQueue(0, false, _threadCount, 0, SCHED_OTHER);
        startQueue(1, false, _threadCount, 0, SCHED_OTHER);
        startQueue(2, false, _threadCount, 0, SCHED_OTHER);

        if(GD::bl->settings.flowsWatchdogTimeout() >= 1000) _watchdogThread = std::thread(&FlowsClient::watchdog, this);

        _socketPath = GD::bl->settings.socketPath() + "homegearFE.sock";
        if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Socket path is " + _socketPath);
        for(int32_t i = 0; i < 2; i++)
        {
            _fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
            if(!_fileDescriptor || _fileDescriptor->descriptor == -1)
            {
                _out.printError("Could not create socket.");
                return;
            }

            if(GD::bl->debugLevel >= 4 && i == 0) std::cout << "Info: Trying to connect..." << std::endl;
            sockaddr_un remoteAddress{};
            remoteAddress.sun_family = AF_LOCAL;
            //104 is the size on BSD systems - slightly smaller than in Linux
            if(_socketPath.length() > 104)
            {
                //Check for buffer overflow
                _out.printCritical("Critical: Socket path is too long.");
                return;
            }
            strncpy(remoteAddress.sun_path, _socketPath.c_str(), 104);
            remoteAddress.sun_path[103] = 0; //Just to make sure it is null terminated.
            if(connect(_fileDescriptor->descriptor, (struct sockaddr*)&remoteAddress, static_cast<socklen_t>(strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family))) == -1)
            {
                if(i == 0)
                {
                    _out.printDebug("Debug: Socket closed. Trying again...");
                    //When socket was not properly closed, we sometimes need to reconnect
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                    continue;
                }
                else
                {
                    _out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
                    return;
                }
            }
            else break;
        }
        if(GD::bl->debugLevel >= 4) _out.printMessage("Connected.");

        if(_maintenanceThread.joinable()) _maintenanceThread.join();
        _maintenanceThread = std::thread(&FlowsClient::registerClient, this);

        std::vector<char> buffer(1024);
        int32_t result = 0;
        int32_t bytesRead = 0;
        int32_t processedBytes = 0;
        while(!_stopped)
        {
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
            if(result == 0) continue;
            else if(result == -1)
            {
                if(errno == EINTR) continue;
                _out.printMessage("Connection to flows server closed (1). Exiting.");
                if(!_shutdownComplete)
                {
                    _out.printError("Error: Shutdown is not complete. Killing process...");
                    kill(getpid(), 9);
                }
                return;
            }

            bytesRead = static_cast<int32_t>(read(_fileDescriptor->descriptor, &buffer[0], 1024));
            if(bytesRead <= 0) //read returns 0, when connection is disrupted.
            {
                _out.printMessage("Connection to flows server closed (2). Exiting.");
                if(!_shutdownComplete)
                {
                    _out.printError("Error: Shutdown is not complete. Killing process...");
                    kill(getpid(), 9);
                }
                return;
            }

            if(bytesRead > (signed)buffer.size()) bytesRead = static_cast<int32_t>(buffer.size());

            try
            {
                processedBytes = 0;
                while(processedBytes < bytesRead)
                {
                    processedBytes += _binaryRpc->process(&buffer[processedBytes], bytesRead - processedBytes);
                    if(_binaryRpc->isFinished())
                    {
                        if(_binaryRpc->getType() == Flows::BinaryRpc::Type::request)
                        {
                            std::string methodName;
                            Flows::PArray parameters = _rpcDecoder->decodeRequest(_binaryRpc->getData(), methodName);
                            std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(methodName, parameters);
                            if(methodName == "shutdown") shutdown(parameters->at(2)->arrayValue);
                            else if(methodName == "reset")
                            {
                                if(_maintenanceThread.joinable()) _maintenanceThread.join();
                                _maintenanceThread = std::thread(&FlowsClient::resetClient, this, parameters->at(0));
                            }
                            else if(!enqueue(0, queueEntry, !_startUpComplete)) printQueueFullError(_out, "Error: Could not queue RPC request because buffer is full. Dropping it.");
                        }
                        else
                        {
                            std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(_binaryRpc->getData());
                            if(!enqueue(1, queueEntry, !_startUpComplete)) printQueueFullError(_out, "Error: Could not queue RPC response because buffer is full. Dropping it.");
                        }
                        _binaryRpc->reset();
                    }
                }
            }
            catch(Flows::BinaryRpcException& ex)
            {
                _out.printError("Error processing packet: " + ex.what());
                _binaryRpc->reset();
            }
        }
        buffer.clear();
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

void FlowsClient::registerClient()
{
    try
    {
        std::string methodName("registerFlowsClient");
        Flows::PArray parameters(new Flows::Array{std::make_shared<Flows::Variable>((int32_t)getpid())});
        Flows::PVariable result = invoke(methodName, parameters, true);
        if(result->errorStruct)
        {
            _out.printCritical("Critical: Could not register client.");
            dispose();
            return;
        }
        _out.printInfo("Info: Client registered to server.");
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

void FlowsClient::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
    try
    {
        std::shared_ptr<QueueEntry> queueEntry;
        queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
        if(!queueEntry) return;

        if(index == 0) //IPC request
        {
            _processingThreadCount1++;
            try
            {
                if(_processingThreadCount1 == _threadCount) _processingThreadCountMaxReached1 = BaseLib::HelperFunctions::getTime();
                if (queueEntry->parameters->size() < 3)
                {
                    _out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
                    return;
                }
                auto localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
                if (localMethodIterator == _localRpcMethods.end())
                {
                    _out.printError("Warning: RPC method not found: " + queueEntry->methodName);
                    Flows::PVariable error = Flows::Variable::createError(-32601, "Requested method not found.");
                    if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), error);
                    return;
                }

                if (GD::bl->debugLevel >= 5)
                {
                    _out.printDebug("Debug: Server is calling RPC method: " + queueEntry->methodName + " Parameters:");
                    for (const auto& parameter : *queueEntry->parameters)
                    {
                        parameter->print(true, false);
                    }
                }

                Flows::PVariable result = localMethodIterator->second(queueEntry->parameters->at(2)->arrayValue);
                if (GD::bl->debugLevel >= 5)
                {
                    _out.printDebug("Response: ");
                    result->print(true, false);
                }
                if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), result);
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
            _processingThreadCountMaxReached1 = 0;
            _processingThreadCount1--;
        }
        else if(index == 1) //IPC response
        {
            _processingThreadCount2++;
            try
            {
                if(_processingThreadCount2 == _threadCount) _processingThreadCountMaxReached2 = BaseLib::HelperFunctions::getTime();

                Flows::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
                if (response->arrayValue->size() < 3)
                {
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
                if (requestInfo)
                {
                    std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
                    {
                        std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
                        auto responseIterator = _rpcResponses[threadId].find(packetId);
                        if (responseIterator != _rpcResponses[threadId].end())
                        {
                            PFlowsResponseClient element = responseIterator->second;
                            if (element)
                            {
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
            _processingThreadCountMaxReached2 = 0;
            _processingThreadCount2--;
        }
        else //Node output
        {
            _processingThreadCount3++;
            try
            {
                if(_processingThreadCount3 == _threadCount) _processingThreadCountMaxReached3 = BaseLib::HelperFunctions::getTime();

                if (!queueEntry->nodeInfo || !queueEntry->message || _shuttingDownOrRestarting) return;
                Flows::PINode node = _nodeManager->getNode(queueEntry->nodeInfo->id);
                if (node)
                {
                    std::string eventFlowId;
                    {
                        std::lock_guard<std::mutex> eventFlowIdGuard(_eventFlowIdMutex);
                        eventFlowId = _eventFlowId;
                    }
                    if (GD::bl->settings.nodeBlueDebugOutput() && _startUpComplete && _frontendConnected && node->getFlowId() == eventFlowId && BaseLib::HelperFunctions::getTime() - queueEntry->nodeInfo->lastNodeEvent1 >= 100)
                    {
                        queueEntry->nodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
                        Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                        timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
                        nodeEvent(queueEntry->nodeInfo->id, "highlightNode/" + queueEntry->nodeInfo->id, timeout);
                    }
                    auto internalMessageIterator = queueEntry->message->structValue->find("_internal");
                    if (internalMessageIterator != queueEntry->message->structValue->end()) setInternalMessage(queueEntry->nodeInfo->id, internalMessageIterator->second);
                    {
                        std::lock_guard<std::mutex> nodeInputGuard(node->getInputMutex());
                        node->input(queueEntry->nodeInfo, queueEntry->targetPort, queueEntry->message);
                    }
                    setInputValue(queueEntry->nodeInfo->id, queueEntry->targetPort, queueEntry->message);
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
            _processingThreadCountMaxReached3 = 0;
            _processingThreadCount3--;
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

Flows::PVariable FlowsClient::send(std::vector<char>& data)
{
    try
    {
        int32_t totallySentBytes = 0;
        std::lock_guard<std::mutex> sendGuard(_sendMutex);
        while (totallySentBytes < (signed)data.size())
        {
            auto sentBytes = static_cast<int32_t>(::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL));
            if(sentBytes <= 0)
            {
                if(errno == EAGAIN) continue;
                if(_fileDescriptor->descriptor != -1) _out.printError("Could not send data to client " + std::to_string(_fileDescriptor->descriptor) + ". Sent bytes: " + std::to_string(totallySentBytes) + " of " + std::to_string(data.size()) + (sentBytes == -1 ? ". Error message: " + std::string(strerror(errno)) : ""));
                return Flows::Variable::createError(-32500, "Unknown application error.");
            }
            totallySentBytes += sentBytes;
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
    return std::make_shared<Flows::Variable>();
}

Flows::PVariable FlowsClient::invoke(std::string methodName, Flows::PArray parameters, bool wait)
{
    try
    {
        if(_nodesStopped)
        {
            if(methodName != "waitForStop" && methodName != "executePhpNodeMethod") return Flows::Variable::createError(-32501, "RPC calls are forbidden after \"stop()\" has been called.");
            else if(methodName == "executePhpNodeMethod" && (parameters->size() < 2 || parameters->at(1)->stringValue != "waitForStop")) return Flows::Variable::createError(-32501, "RPC calls are forbidden after \"stop()\" has been called.");
        }

        int64_t threadId = pthread_self();

        PRequestInfo requestInfo;
        if(wait)
        {
            std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
            auto emplaceResult = _requestInfo.emplace(std::piecewise_construct, std::make_tuple(threadId), std::make_tuple(std::make_shared<RequestInfo>()));
            if(!emplaceResult.second) _out.printError("Error: Thread is executing invoke() more than once.");
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

        PFlowsResponseClient response;
        if(wait)
        {
            {
                std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
                auto result = _rpcResponses[threadId].emplace(packetId, std::make_shared<FlowsResponseClient>());
                if(result.second) response = result.first->second;
            }
            if(!response)
            {
                _out.printError("Critical: Could not insert response struct into map.");
                return Flows::Variable::createError(-32500, "Unknown application error.");
            }
        }
        Flows::PVariable result = send(data);
        if(result->errorStruct || !wait)
        {
            if(!wait) return std::make_shared<Flows::Variable>();
            else
            {
                std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
                _rpcResponses[threadId].erase(packetId);
                if (_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
                return result;
            }
        }

        std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
        while (!requestInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&]
        {
            return response->finished || _stopped;
        }));

        if(!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId)
        {
            _out.printError("Error: No response received to RPC request. Method: " + methodName);
            result = Flows::Variable::createError(-1, "No response received.");
        }
        else result = response->response->arrayValue->at(2);

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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::invokeNodeMethod(std::string nodeId, std::string methodName, Flows::PArray parameters, bool wait)
{
    try
    {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if(node) return node->invokeLocal(methodName, parameters);

        Flows::PArray parametersArray = std::make_shared<Flows::Array>();
        parametersArray->reserve(4);
        parametersArray->push_back(std::make_shared<Flows::Variable>(nodeId));
        parametersArray->push_back(std::make_shared<Flows::Variable>(methodName));
        parametersArray->push_back(std::make_shared<Flows::Variable>(parameters));
        parametersArray->push_back(std::make_shared<Flows::Variable>(wait));

        return invoke("invokeNodeMethod", parametersArray, wait);
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

void FlowsClient::sendResponse(Flows::PVariable& packetId, Flows::PVariable& variable)
{
    try
    {
        Flows::PVariable array(new Flows::Variable(Flows::PArray(new Flows::Array{ packetId, variable })));
        std::vector<char> data;
        _rpcEncoder->encodeResponse(array, data);

        send(data);
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

void FlowsClient::log(std::string nodeId, int32_t logLevel, std::string message)
{
    _out.printMessage("Node " + nodeId + ": " + message, logLevel, logLevel <= 3);
}

void FlowsClient::subscribePeer(std::string nodeId, uint64_t peerId, int32_t channel, std::string variable)
{
    try
    {
        std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
        _peerSubscriptions[peerId][channel][variable].insert(nodeId);
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

void FlowsClient::unsubscribePeer(std::string nodeId, uint64_t peerId, int32_t channel, std::string variable)
{
    try
    {
        std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
        _peerSubscriptions[peerId][channel][variable].erase(nodeId);
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

void FlowsClient::queueOutput(std::string nodeId, uint32_t index, Flows::PVariable message, bool synchronous)
{
    try
    {
        if(!message || _shuttingDownOrRestarting) return;

        PNodeInfo nodeInfo;
        {
            std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
            auto nodesIterator = _nodes.find(nodeId);
            if(nodesIterator == _nodes.end()) return;
            nodeInfo = nodesIterator->second;
        }

        {
            std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
            auto internalMessagesIterator = _internalMessages.find(nodeId);
            if(internalMessagesIterator != _internalMessages.end())
            {
                message->structValue->emplace("_internal", internalMessagesIterator->second);
                if(!synchronous)
                {
                    auto synchronousOutputIterator = internalMessagesIterator->second->structValue->find("synchronousOutput");
                    if (synchronousOutputIterator != internalMessagesIterator->second->structValue->end())
                    {
                        synchronous = synchronousOutputIterator->second->booleanValue;
                    }
                }
            }
        }

        if(message->structValue->find("payload") == message->structValue->end()) message->structValue->emplace("payload", std::make_shared<Flows::Variable>());

        if(index >= nodeInfo->wiresOut.size())
        {
            _out.printError("Error: " + nodeId + " has no output with index " + std::to_string(index) + ".");
            return;
        }

        std::string eventFlowId;
        {
            std::lock_guard<std::mutex> eventFlowIdGuard(_eventFlowIdMutex);
            eventFlowId = _eventFlowId;
        }

        if(synchronous && GD::bl->settings.nodeBlueDebugOutput())
        {
            //Output node events before execution of input
            if(_frontendConnected && _startUpComplete && nodeInfo->info->structValue->at("flow")->stringValue == eventFlowId)
            {
                Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
                nodeEvent(nodeId, "highlightNode/" + nodeId, timeout);
                Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
                nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex);

                Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                std::string statusText = message->structValue->at("payload")->toString();
                if(statusText.size() > 20) statusText = statusText.substr(0, 17) + "...";
                statusText = std::to_string(index) + ": " + BaseLib::HelperFunctions::stripNonPrintable(statusText);
                status->structValue->emplace("text", std::make_shared<Flows::Variable>(statusText));
                nodeEvent(nodeId, "statusTop/" + nodeId, status);
            }
        }

        message->structValue->emplace("source", std::make_shared<Flows::Variable>(nodeId));
        for(auto& node : nodeInfo->wiresOut.at(index))
        {
            PNodeInfo outputNodeInfo;
            {
                std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
                auto nodeIterator = _nodes.find(node.id);
                if(nodeIterator == _nodes.end()) continue;
                outputNodeInfo = nodeIterator->second;
            }
            if(synchronous)
            {
                Flows::PINode nextNode = _nodeManager->getNode(outputNodeInfo->id);
                if (nextNode)
                {
                    if (GD::bl->settings.nodeBlueDebugOutput() && _startUpComplete && _frontendConnected && nextNode->getFlowId() == eventFlowId && BaseLib::HelperFunctions::getTime() - outputNodeInfo->lastNodeEvent1 >= 100)
                    {
                        outputNodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
                        Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                        timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
                        nodeEvent(outputNodeInfo->id, "highlightNode/" + outputNodeInfo->id, timeout);
                    }
                    auto internalMessageIterator = message->structValue->find("_internal");
                    if (internalMessageIterator != message->structValue->end())
                    {
                        //Emplace makes sure, that synchronousOutput stays false when set to false in node. This is the only way to make a synchronous output asynchronous again.
                        internalMessageIterator->second->structValue->emplace("synchronousOutput", std::make_shared<Flows::Variable>(true));
                        setInternalMessage(outputNodeInfo->id, internalMessageIterator->second);
                    }
                    else
                    {
                        Flows::PVariable internalMessage = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                        internalMessage->structValue->emplace("synchronousOutput", std::make_shared<Flows::Variable>(true));
                        setInternalMessage(outputNodeInfo->id, internalMessage);
                        message->structValue->emplace("_internal", internalMessage);
                    }

                    {
                        std::lock_guard<std::mutex> nodeInputGuard(nextNode->getInputMutex());
                        nextNode->input(outputNodeInfo, node.port, message);
                    }

                    setInputValue(outputNodeInfo->id, node.port, message);
                }
            }
            else
            {
                if(queueSize(2) > 1000 && BaseLib::HelperFunctions::getTime() - _lastQueueSize > 1000)
                {
                    _lastQueueSize = BaseLib::HelperFunctions::getTime();
                    if(_startUpComplete) _out.printWarning("Warning: Node output queue has " + std::to_string(queueSize(2)) + " entries.");
                    else _out.printInfo("Info: Node output queue has " + std::to_string(queueSize(2)) + " entries.");
                }
                std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(outputNodeInfo, node.port, message);
                if (!enqueue(2, queueEntry, !_startUpComplete))
                {
                    printQueueFullError(_out, "Error: Dropping output of node " + nodeId + ". Queue is full.");
                    return;
                }
            }
        }

        if(!synchronous && GD::bl->settings.nodeBlueDebugOutput())
        {
            if(_frontendConnected && _startUpComplete && nodeInfo->info->structValue->at("flow")->stringValue == eventFlowId)
            {
                Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
                nodeEvent(nodeId, "highlightNode/" + nodeId, timeout);
                Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
                nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex);

                Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
                std::string statusText = message->structValue->at("payload")->toString();
                if(statusText.size() > 20) statusText = statusText.substr(0, 17) + "...";
                statusText = std::to_string(index) + ": " + BaseLib::HelperFunctions::stripNonPrintable(statusText);
                status->structValue->emplace("text", std::make_shared<Flows::Variable>(statusText));
                nodeEvent(nodeId, "statusTop/" + nodeId, status);
            }
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

void FlowsClient::nodeEvent(std::string nodeId, std::string topic, Flows::PVariable value)
{
    try
    {
        if(!value || !_startUpComplete) return;

        if(!_frontendConnected && topic.compare(0, 13, "statusBottom/") != 0) return;

        Flows::PArray parameters = std::make_shared<Flows::Array>();
        parameters->reserve(3);
        parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
        parameters->push_back(std::make_shared<Flows::Variable>(topic));
        parameters->push_back(value);

        Flows::PVariable result = invoke("nodeEvent", parameters, false);
        if(result->errorStruct) GD::out.printError("Error calling nodeEvent: " + result->structValue->at("faultString")->stringValue);
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

Flows::PVariable FlowsClient::getNodeData(std::string nodeId, std::string key)
{
    try
    {
        Flows::PArray parameters = std::make_shared<Flows::Array>();
        parameters->reserve(2);
        parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
        parameters->push_back(std::make_shared<Flows::Variable>(key));

        Flows::PVariable result = invoke("getNodeData", parameters, true);
        if(result->errorStruct)
        {
            GD::out.printError("Error calling getNodeData: " + result->structValue->at("faultString")->stringValue);
            return std::make_shared<Flows::Variable>();
        }

        return result;
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
    return std::make_shared<Flows::Variable>();
}

void FlowsClient::setNodeData(std::string nodeId, std::string key, Flows::PVariable value)
{
    try
    {
        Flows::PArray parameters = std::make_shared<Flows::Array>();
        parameters->reserve(3);
        parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
        parameters->push_back(std::make_shared<Flows::Variable>(key));
        parameters->push_back(value);

        invoke("setNodeData", parameters, false);
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

void FlowsClient::setInternalMessage(std::string nodeId, Flows::PVariable message)
{
    try
    {
        std::lock_guard<std::mutex> internalMessagesGuard(_internalMessagesMutex);
        auto internalMessageIterator = _internalMessages.find(nodeId);
        if(internalMessageIterator != _internalMessages.end())
        {
            if(internalMessageIterator->second == message) return;
            for(auto element : *message->structValue)
            {
                internalMessageIterator->second->structValue->erase(element.first);
                internalMessageIterator->second->structValue->emplace(element.first, element.second);
            }
        }
        else _internalMessages[nodeId] = message;
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

void FlowsClient::setInputValue(std::string& nodeId, int32_t inputIndex, Flows::PVariable message)
{
    try
    {
        std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
        Flows::PVariable payload = message->structValue->at("payload");
        if(payload->type == Flows::VariableType::tArray)
        {
            payload = std::make_shared<Flows::Variable>(std::string("Array"));
        }
        else if(payload->type == Flows::VariableType::tStruct)
        {
            payload = std::make_shared<Flows::Variable>(std::string("Struct"));
        }
        else if(payload->type == Flows::VariableType::tBinary)
        {
            if(payload->binaryValue.size() > 10) payload = std::make_shared<Flows::Variable>(BaseLib::HelperFunctions::getHexString(payload->binaryValue.data(), 10) + "...");
            else payload = std::make_shared<Flows::Variable>(BaseLib::HelperFunctions::getHexString(payload->binaryValue.data(), payload->binaryValue.size()));
        }
        else if(payload->type == Flows::VariableType::tString && payload->stringValue.size() > 20)
        {
            payload = std::make_shared<Flows::Variable>(payload->stringValue.substr(0, 20) + "...");
        }

        auto& element = _inputValues[nodeId][inputIndex];
        InputValue inputValue;
        inputValue.time = BaseLib::HelperFunctions::getTime();
        inputValue.value = payload;
        element.push_front(std::move(inputValue));
        while(element.size() > 10) element.pop_back();
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

Flows::PVariable FlowsClient::getConfigParameter(std::string nodeId, std::string name)
{
    try
    {
        Flows::PINode node = _nodeManager->getNode(nodeId);
        if(node) return node->getConfigParameterIncoming(std::move(name));
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
    return std::make_shared<Flows::Variable>();
}

void FlowsClient::watchdog()
{
    while(!_stopped && !_shuttingDownOrRestarting)
    {
        try
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            if(_processingThreadCount1 == _threadCount)
            {
                int64_t time = _processingThreadCountMaxReached1;
                if(time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.flowsWatchdogTimeout())
                {
                    std::cout << "Watchdog 1 timed out. Killing process." << std::endl;
                    std::cerr << "Watchdog 1 timed out. Killing process." << std::endl;
                    kill(getpid(), 9);
                }
            }

            if(_processingThreadCount2 == _threadCount)
            {
                int64_t time = _processingThreadCountMaxReached2;
                if(time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.flowsWatchdogTimeout())
                {
                    std::cout << "Watchdog 2 timed out. Killing process." << std::endl;
                    std::cerr << "Watchdog 2 timed out. Killing process." << std::endl;
                    kill(getpid(), 9);
                }
            }

            if(_processingThreadCount3 == _threadCount)
            {
                int64_t time = _processingThreadCountMaxReached3;
                if(time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.flowsWatchdogTimeout())
                {
                    std::cout << "Watchdog 3 timed out. Killing process." << std::endl;
                    std::cerr << "Watchdog 3 timed out. Killing process." << std::endl;
                    kill(getpid(), 9);
                }
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
}

// {{{ RPC methods
Flows::PVariable FlowsClient::reload(Flows::PArray& parameters)
{
    try
    {
        if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.log").c_str(), "a", stdout))
        {
            GD::out.printError("Error: Could not redirect output to new log file.");
        }
        if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-flows.err").c_str(), "a", stderr))
        {
            GD::out.printError("Error: Could not redirect errors to new log file.");
        }

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::shutdown(Flows::PArray& parameters)
{
    try
    {
        if(_maintenanceThread.joinable()) _maintenanceThread.join();
        _maintenanceThread = std::thread(&FlowsClient::dispose, this);

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::startFlow(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

        std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
        if(_disposed) return Flows::Variable::createError(-1, "Client is disposing.");

        _startUpComplete = false;

        std::set<PNodeInfo> nodes;
        PFlowInfoClient flow = std::make_shared<FlowInfoClient>();
        flow->id = parameters->at(0)->integerValue;
        flow->flow = parameters->at(1)->arrayValue->at(0);
        std::string flowId;
        auto flowIdIterator = flow->flow->structValue->find("id");
        if(flowIdIterator != flow->flow->structValue->end()) flowId = flowIdIterator->second->stringValue;

        _out.printInfo("Info: Starting flow with ID " + std::to_string(flow->id) + " (" + flowId + ")...");

        for(auto& element : *parameters->at(1)->arrayValue)
        {
            PNodeInfo node = std::make_shared<NodeInfo>();

            auto idIterator = element->structValue->find("id");
            if(idIterator == element->structValue->end())
            {
                GD::out.printError("Error: Flow element has no id.");
                continue;
            }

            auto typeIterator = element->structValue->find("type");
            if(typeIterator == element->structValue->end())
            {
                GD::out.printError("Error: Flow element has no type.");
                continue;
            }
            if(typeIterator->second->stringValue.compare(0, 8, "subflow:") == 0) continue;

            node->id = idIterator->second->stringValue;
            node->type = typeIterator->second->stringValue;
            node->info = element;

            auto namespaceIterator = element->structValue->find("namespace");
            if(namespaceIterator != element->structValue->end()) node->nodeNamespace = namespaceIterator->second->stringValue;
            else node->nodeNamespace = node->type;

            auto wiresIterator = element->structValue->find("wires");
            if(wiresIterator != element->structValue->end())
            {
                node->wiresOut.reserve(wiresIterator->second->arrayValue->size());
                for(auto& outputIterator : *wiresIterator->second->arrayValue)
                {
                    std::vector<Wire> output;
                    output.reserve(outputIterator->structValue->size());
                    for(auto& wireIterator : *outputIterator->arrayValue)
                    {
                        auto innerIdIterator = wireIterator->structValue->find("id");
                        if(innerIdIterator == wireIterator->structValue->end()) continue;
                        uint32_t port = 0;
                        auto portIterator = wireIterator->structValue->find("port");
                        if(portIterator != wireIterator->structValue->end())
                        {
                            if(portIterator->second->type == Flows::VariableType::tString) port = static_cast<uint32_t>(BaseLib::Math::getNumber(portIterator->second->stringValue));
                            else port = static_cast<uint32_t>(portIterator->second->integerValue);
                        }
                        Wire wire;
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

        if(flow->nodes.empty())
        {
            _out.printWarning("Warning: Not starting flow with ID " + std::to_string(flow->id) + ", because it has no nodes.");
            return Flows::Variable::createError(-100, "Flow has no nodes.");
        }

        //{{{ Set wiresIn
            for(auto& node : flow->nodes)
            {
                int32_t outputIndex = 0;
                for(auto& output : node.second->wiresOut)
                {
                    for(auto& wireOut : output)
                    {
                        auto nodeIterator = flow->nodes.find(wireOut.id);
                        if(nodeIterator == flow->nodes.end()) continue;
                        if(nodeIterator->second->wiresIn.size() <= wireOut.port) nodeIterator->second->wiresIn.resize(wireOut.port + 1);
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
                for(auto& node : nodes)
                {
                    if(_bl->debugLevel >= 5) _out.printDebug("Starting node " + node->id + " of type " + node->type + ".");

                    Flows::PINode nodeObject;
                    int32_t result = _nodeManager->loadNode(node->nodeNamespace, node->type, node->id, nodeObject);
                    if(result < 0)
                    {
                        _out.printError("Error: Could not load node " + node->type + ". Error code: " + std::to_string(result));
                        continue;
                    }

                    nodeObject->setId(node->id);
                    nodeObject->setFlowId(flowId);

                    nodeObject->setLog(std::function<void(std::string, int32_t, std::string)>(std::bind(&FlowsClient::log, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
                    nodeObject->setInvoke(std::function<Flows::PVariable(std::string, Flows::PArray)>(std::bind(&FlowsClient::invoke, this, std::placeholders::_1, std::placeholders::_2, true)));
                    nodeObject->setInvokeNodeMethod(std::function<Flows::PVariable(std::string, std::string, Flows::PArray, bool)>(std::bind(&FlowsClient::invokeNodeMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
                    nodeObject->setSubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::subscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
                    nodeObject->setUnsubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::unsubscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
                    nodeObject->setOutput(std::function<void(std::string, uint32_t, Flows::PVariable, bool)>(std::bind(&FlowsClient::queueOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
                    nodeObject->setNodeEvent(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::nodeEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
                    nodeObject->setGetNodeData(std::function<Flows::PVariable(std::string, std::string)>(std::bind(&FlowsClient::getNodeData, this, std::placeholders::_1, std::placeholders::_2)));
                    nodeObject->setSetNodeData(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::setNodeData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
                    nodeObject->setSetInternalMessage(std::function<void(std::string, Flows::PVariable)>(std::bind(&FlowsClient::setInternalMessage, this, std::placeholders::_1, std::placeholders::_2)));
                    nodeObject->setGetConfigParameter(std::function<Flows::PVariable(std::string, std::string)>(std::bind(&FlowsClient::getConfigParameter, this, std::placeholders::_1, std::placeholders::_2)));

                    if(!nodeObject->init(node))
                    {
                        nodeObject.reset();
                        nodesToRemove.emplace(node->id);
                        _out.printError("Error: Could not load node " + node->type + " with ID " + node->id + ". \"init\" failed.");
                        continue;
                    }
                }
                for(auto& node : nodesToRemove)
                {
                    flow->nodes.erase(node);
                    {
                        std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
                        _nodes.erase(node);
                    }
                    _nodeManager->unloadNode(node);
                }
            }
        //}}}

        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        _flows.emplace(flow->id, flow);

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::startNodes(Flows::PArray& parameters)
{
    try
    {
        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        for(auto& flow : _flows)
        {
            std::set<std::string> nodesToRemove;
            for(auto& nodeIterator : flow.second->nodes)
            {
                Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                if(node)
                {
                    if(!node->start())
                    {
                        node.reset();
                        nodesToRemove.emplace(nodeIterator.second->id);
                        _out.printError("Error: Could not load node " + nodeIterator.second->type + " with ID " + nodeIterator.second->id + ". \"start\" failed.");
                    }
                }
            }
            for(auto& node : nodesToRemove)
            {
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::configNodesStarted(Flows::PArray& parameters)
{
    try
    {
        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        for(auto& flow : _flows)
        {
            for(auto& nodeIterator : flow.second->nodes)
            {
                Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                if(node) node->configNodesStarted();
            }
        }
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::startUpComplete(Flows::PArray& parameters)
{
    try
    {
        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        for(auto& flow : _flows)
        {
            for(auto& nodeIterator : flow.second->nodes)
            {
                Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                if(node) node->startUpComplete();
            }
        }
        _startUpComplete = true;
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::stopNodes(Flows::PArray& parameters)
{
    try
    {
        _shuttingDownOrRestarting = true;
        _out.printMessage("Calling stop()...");
        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        int64_t startTime = 0;
        for(auto& flow : _flows)
        {
            for(auto& nodeIterator : flow.second->nodes)
            {
                Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
                if(_bl->debugLevel >= 5) _out.printDebug("Debug: Calling stop() on node " + nodeIterator.second->id + "...");
                startTime = BaseLib::HelperFunctions::getTime();
                if(node) node->stop();
                if(BaseLib::HelperFunctions::getTime() - startTime > 100) _out.printWarning("Warning: Stop of node " + nodeIterator.second->id + " of type " + nodeIterator.second->type + " in namespace " + nodeIterator.second->type + " in flow " + nodeIterator.second->info->structValue->at("flow")->stringValue + " took longer than 100ms.");
                if(_bl->debugLevel >= 5) _out.printDebug("Debug: Node " + nodeIterator.second->id + " stopped.");
            }
        }
        _out.printMessage("Call to stop() completed.");
        _nodesStopped = true;
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::stopFlow(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        auto flowsIterator = _flows.find(parameters->at(0)->integerValue);
        if(flowsIterator == _flows.end()) return Flows::Variable::createError(-100, "Unknown flow.");
        for(auto& node : flowsIterator->second->nodes)
        {
            {
                std::lock_guard<std::mutex> peerSubscriptionsGuard(_peerSubscriptionsMutex);
                for(auto& peerId : _peerSubscriptions)
                {
                    for(auto& channel : peerId.second)
                    {
                        for(auto& variable : channel.second)
                        {
                            variable.second.erase(node.first);
                        }
                    }
                }
            }
            _nodeManager->unloadNode(node.second->id);
        }
        _flows.erase(flowsIterator);
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::flowCount(Flows::PArray& parameters)
{
    try
    {
        std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
        return std::make_shared<Flows::Variable>(_flows.size());
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::nodeOutput(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 3 && parameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");

        queueOutput(parameters->at(0)->stringValue, static_cast<uint32_t>(parameters->at(1)->integerValue), parameters->at(2), parameters->size() == 4 ? parameters->at(3)->booleanValue : false);
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::invokeExternalNodeMethod(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");

        Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
        if(!node) return Flows::Variable::createError(-1, "Unknown node.");
        return node->invokeLocal(parameters->at(1)->stringValue, parameters->at(2)->arrayValue);
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::executePhpNodeBaseMethod(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

        std::string& methodName = parameters->at(1)->stringValue;
        Flows::PArray& innerParameters = parameters->at(2)->arrayValue;

        if(methodName == "log")
        {
            if(innerParameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
            if(innerParameters->at(2)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 3 is not of type string.");

            log(innerParameters->at(0)->stringValue, innerParameters->at(1)->integerValue, innerParameters->at(2)->stringValue);
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "invokeNodeMethod")
        {
            if(innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");
            if(innerParameters->at(2)->type != Flows::VariableType::tArray) return Flows::Variable::createError(-1, "Parameter 3 is not of type array.");
            if(innerParameters->at(3)->type != Flows::VariableType::tBoolean) return Flows::Variable::createError(-1, "Parameter 4 is not of type boolean.");

            return invokeNodeMethod(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue, innerParameters->at(2)->arrayValue, innerParameters->at(3)->booleanValue);
        }
        else if(methodName == "subscribePeer")
        {
            if(innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
            if(innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
            if(innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

            subscribePeer(innerParameters->at(0)->stringValue, static_cast<uint64_t>(innerParameters->at(1)->integerValue64), innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "unsubscribePeer")
        {
            if(innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
            if(innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
            if(innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

            unsubscribePeer(innerParameters->at(0)->stringValue, static_cast<uint64_t>(innerParameters->at(1)->integerValue64), innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "output")
        {
            if(innerParameters->size() != 3 && innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");

            queueOutput(innerParameters->at(0)->stringValue, static_cast<uint32_t>(innerParameters->at(1)->integerValue), innerParameters->at(2), innerParameters->size() == 4 ? innerParameters->at(3)->booleanValue : false);
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "nodeEvent")
        {
            if(innerParameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
            if(innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");

            nodeEvent(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue, innerParameters->at(2));
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "getNodeData")
        {
            if(innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter is not of type string.");

            return getNodeData(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue);
        }
        else if(methodName == "setNodeData")
        {
            if(innerParameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");

            setNodeData(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue, innerParameters->at(1));
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "setInternalMessage")
        {
            if(innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

            setInternalMessage(parameters->at(0)->stringValue, innerParameters->at(0));
            return std::make_shared<Flows::Variable>();
        }
        else if(methodName == "getConfigParameter")
        {
            if(innerParameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");
            if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter is not of type string.");

            return getConfigParameter(parameters->at(0)->stringValue, innerParameters->at(0)->stringValue);
        }
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::getNodeVariable(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");
        if(parameters->at(1)->stringValue.compare(0, sizeof("inputValue") - 1, "inputValue") == 0 && parameters->at(1)->stringValue.size() > 10)
        {
            std::string indexString = parameters->at(1)->stringValue.substr(10);
            int32_t index = Flows::Math::getNumber(indexString);
            std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
            auto nodeIterator = _inputValues.find(parameters->at(0)->stringValue);
            if(nodeIterator != _inputValues.end())
            {
                auto inputIterator = nodeIterator->second.find(index);
                if(inputIterator != nodeIterator->second.end()) return inputIterator->second.front().value;
            }
        }
        else if(parameters->at(1)->stringValue.compare(0, sizeof("inputHistory") - 1, "inputHistory") == 0 && parameters->at(1)->stringValue.size() > 12)
        {
            std::string indexString = parameters->at(1)->stringValue.substr(12);
            int32_t index = Flows::Math::getNumber(indexString);
            std::lock_guard<std::mutex> inputValuesGuard(_inputValuesMutex);
            auto nodeIterator = _inputValues.find(parameters->at(0)->stringValue);
            if(nodeIterator != _inputValues.end())
            {
                auto inputIterator = nodeIterator->second.find(index);
                if(inputIterator != nodeIterator->second.end())
                {
                    Flows::PVariable array = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
                    array->arrayValue->reserve(inputIterator->second.size());
                    for(auto& inputValue : inputIterator->second)
                    {
                        Flows::PVariable innerArray = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
                        innerArray->arrayValue->reserve(2);
                        innerArray->arrayValue->push_back(std::make_shared<Flows::Variable>(inputValue.time));
                        innerArray->arrayValue->push_back(inputValue.value);
                        array->arrayValue->push_back(innerArray);
                    }
                    return array;
                }
            }
        }
        else
        {
            Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
            if(node) return node->getNodeVariable(parameters->at(1)->stringValue);
        }
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::getFlowVariable(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 2) return Flows::Variable::createError(-1, "Wrong parameter count.");

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::setNodeVariable(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
        Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
        if(node) node->setNodeVariable(parameters->at(1)->stringValue, parameters->at(2));
        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::setFlowVariable(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
        if(parameters->at(1)->stringValue == "enableEvents" && parameters->at(2)->booleanValue)
        {
            std::lock_guard<std::mutex> eventFlowGuard(_eventFlowIdMutex);
            _eventFlowId = parameters->at(0)->stringValue;
            _out.printInfo("Info: Events are now sent to flow " + _eventFlowId);
        }

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::enableNodeEvents(Flows::PArray& parameters)
{
    try
    {
        _frontendConnected = true;

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::disableNodeEvents(Flows::PArray& parameters)
{
    try
    {
        _frontendConnected = false;

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::broadcastEvent(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");

        std::lock_guard<std::mutex> eventsGuard(_peerSubscriptionsMutex);
        auto peerId = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        int32_t channel = parameters->at(1)->integerValue;

        auto peerIterator = _peerSubscriptions.find(peerId);
        if(peerIterator == _peerSubscriptions.end()) return std::make_shared<Flows::Variable>();

        auto channelIterator = peerIterator->second.find(channel);
        if(channelIterator == peerIterator->second.end()) return std::make_shared<Flows::Variable>();

        for(uint32_t j = 0; j < parameters->at(2)->arrayValue->size(); j++)
        {
            std::string variableName = parameters->at(2)->arrayValue->at(j)->stringValue;

            auto variableIterator = channelIterator->second.find(variableName);
            if(variableIterator == channelIterator->second.end()) continue;

            Flows::PVariable value = parameters->at(3)->arrayValue->at(j);

            for(auto nodeId : variableIterator->second)
            {
                Flows::PINode node = _nodeManager->getNode(nodeId);
                if(node) node->variableEvent(peerId, channel, variableName, value);
            }
        }

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::broadcastNewDevices(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::broadcastDeleteDevices(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 1) return Flows::Variable::createError(-1, "Wrong parameter count.");

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}

Flows::PVariable FlowsClient::broadcastUpdateDevice(Flows::PArray& parameters)
{
    try
    {
        if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

        return std::make_shared<Flows::Variable>();
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
    return Flows::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}
