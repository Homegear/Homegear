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
#include "../GD/GD.h"

namespace Flows
{

FlowsClient::FlowsClient() : IQueue(GD::bl.get(), 3, 1000)
{
	_stopped = false;
	_nodesStopped = false;
	_disposed = false;
	_shuttingDown = false;
	_frontendConnected = false;

	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
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
}

void FlowsClient::dispose()
{
	try
	{
		std::lock_guard<std::mutex> startFlowGuard(_startFlowMutex);
		if(_disposed) return;
		_disposed = true;
		_out.printMessage("Shutting down...");

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

		_out.printMessage("Nodes are stopped. Disposing...");

		GD::bl->shuttingDown = true;
		_stopped = true;

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

		_flows.clear();
		_rpcResponses.clear();

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

		startQueue(0, false, flowsProcessingThreadCountNodes, 0, SCHED_OTHER);
		startQueue(1, false, flowsProcessingThreadCountNodes, 0, SCHED_OTHER);
		startQueue(2, false, flowsProcessingThreadCountNodes, 0, SCHED_OTHER);

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
			sockaddr_un remoteAddress;
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
			if(connect(_fileDescriptor->descriptor, (struct sockaddr*)&remoteAddress, strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family)) == -1)
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
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			{
				auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
				fileDescriptorGuard.lock();
				FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
			}

			result = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(result == 0) continue;
			else if(result == -1)
			{
				if(errno == EINTR) continue;
				_out.printMessage("Connection to flows server closed (1). Exiting.");
				return;
			}

			bytesRead = read(_fileDescriptor->descriptor, &buffer[0], 1024);
			if(bytesRead <= 0) //read returns 0, when connection is disrupted.
			{
				_out.printMessage("Connection to flows server closed (2). Exiting.");
				return;
			}

			if(bytesRead > (signed)buffer.size()) bytesRead = buffer.size();

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
							else if(!enqueue(0, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC request because buffer is full. Dropping it.");
						}
						else
						{
							std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(_binaryRpc->getData());
							if(!enqueue(1, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC response because buffer is full. Dropping it.");
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
		Flows::PArray parameters(new Flows::Array{Flows::PVariable(new Flows::Variable((int32_t)getpid()))});
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
			if(queueEntry->parameters->size() < 3)
			{
				_out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
				return;
			}
			std::map<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
			if(localMethodIterator == _localRpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + queueEntry->methodName);
				Flows::PVariable error = Flows::Variable::createError(-32601, "Requested method not found.");
				if(queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), error);
				return;
			}

			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Debug: Server is calling RPC method: " + queueEntry->methodName + " Parameters:");
				for(auto parameter : *queueEntry->parameters)
				{
					parameter->print(true, false);
				}
			}

			Flows::PVariable result = localMethodIterator->second(queueEntry->parameters->at(2)->arrayValue);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print(true, false);
			}
			if(queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), result);
		}
		else if(index == 1) //IPC response
		{
			Flows::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
			if(response->arrayValue->size() < 3)
			{
				_out.printError("Error: Response has wrong array size.");
				return;
			}
			int64_t threadId = response->arrayValue->at(0)->integerValue64;
			int32_t packetId = response->arrayValue->at(1)->integerValue;

			{
				std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
				auto responseIterator = _rpcResponses[threadId].find(packetId);
				if(responseIterator != _rpcResponses[threadId].end())
				{
					PFlowsResponseClient element = responseIterator->second;
					if(element)
					{
						element->response = response;
						element->packetId = packetId;
						element->finished = true;
					}
				}
			}
			std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
			std::map<int64_t, RequestInfo>::iterator requestIterator = _requestInfo.find(threadId);
			if (requestIterator != _requestInfo.end()) requestIterator->second.conditionVariable.notify_all();
		}
		else //Node output
		{
			if(!queueEntry->nodeInfo || !queueEntry->message || _shuttingDown) return;
			Flows::PINode node = _nodeManager->getNode(queueEntry->nodeInfo->id);
			if(node)
			{
				if(_frontendConnected && GD::bl->settings.nodeBlueDebugOutput() && BaseLib::HelperFunctions::getTime() - queueEntry->nodeInfo->lastNodeEvent1 >= 200)
				{
					queueEntry->nodeInfo->lastNodeEvent1 = BaseLib::HelperFunctions::getTime();
					Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
					timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
					nodeEvent(queueEntry->nodeInfo->id, "highlightNode/" + queueEntry->nodeInfo->id, timeout);
				}
				std::lock_guard<std::mutex> nodeInputGuard(node->getInputMutex());
				node->input(queueEntry->nodeInfo, queueEntry->targetPort, queueEntry->message);
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

Flows::PVariable FlowsClient::send(std::vector<char>& data)
{
	try
	{
		int32_t totallySentBytes = 0;
		std::lock_guard<std::mutex> sendGuard(_sendMutex);
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sentBytes = ::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
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
    return Flows::PVariable(new Flows::Variable());
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
		std::unique_lock<std::mutex> requestInfoGuard(_requestInfoMutex);
		RequestInfo& requestInfo = _requestInfo[threadId];
		requestInfoGuard.unlock();

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
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[threadId].erase(packetId);
			if (_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
			return result;
		}
		if(!wait) return std::make_shared<Flows::Variable>();

		std::unique_lock<std::mutex> waitLock(requestInfo.waitMutex);
		while (!requestInfo.conditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&]
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

Flows::PVariable FlowsClient::invokeNodeMethod(std::string nodeId, std::string methodName, Flows::PArray parameters)
{
	try
	{
		Flows::PINode node = _nodeManager->getNode(nodeId);
		if(node) return node->invokeLocal(methodName, parameters);

		Flows::PArray parametersArray = std::make_shared<Flows::Array>();
		parametersArray->reserve(3);
		parametersArray->push_back(std::make_shared<Flows::Variable>(nodeId));
		parametersArray->push_back(std::make_shared<Flows::Variable>(methodName));
		parametersArray->push_back(std::make_shared<Flows::Variable>(parameters));

		return invoke("invokeNodeMethod", parametersArray, true);
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

void FlowsClient::queueOutput(std::string nodeId, uint32_t index, Flows::PVariable message)
{
	try
	{
		if(!message || _shuttingDown) return;

		PNodeInfo nodeInfo;
		{
			std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
			auto nodesIterator = _nodes.find(nodeId);
			if(nodesIterator == _nodes.end()) return;
			nodeInfo = nodesIterator->second;
		}

		if(message->structValue->find("payload") == message->structValue->end()) message->structValue->emplace("payload", std::make_shared<Flows::Variable>());

		if(index >= nodeInfo->wiresOut.size())
		{
			_out.printError("Error: " + nodeId + " has no output with index " + std::to_string(index) + ".");
			return;
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
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(outputNodeInfo, node.port, message);
			if(!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Dropping output of node " + nodeId + ". Queue is full.");
		}

		if(_frontendConnected && GD::bl->settings.nodeBlueDebugOutput() && BaseLib::HelperFunctions::getTime() - nodeInfo->lastNodeEvent2 >= 200)
		{
			nodeInfo->lastNodeEvent2 = BaseLib::HelperFunctions::getTime();
			Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
			timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
			nodeEvent(nodeId, "highlightNode/" + nodeId, timeout);
			Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
			outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
			nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex);
			Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
			std::string statusText = std::to_string(index) + ": " + message->structValue->at("payload")->toString();
			if(statusText.size() > 20) statusText = statusText.substr(0, 17) + "...";
			status->structValue->emplace("text", std::make_shared<Flows::Variable>(statusText));
			nodeEvent(nodeId, "statusTop/" + nodeId, status);
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
		if(!value) return;

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

Flows::PVariable FlowsClient::getConfigParameter(std::string nodeId, std::string name)
{
	try
	{
		Flows::PINode node = _nodeManager->getNode(nodeId);
		if(node) return node->getConfigParameterIncoming(name);
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
						auto idIterator = wireIterator->structValue->find("id");
						if(idIterator == wireIterator->structValue->end()) continue;
						uint32_t port = 0;
						auto portIterator = wireIterator->structValue->find("port");
						if(portIterator != wireIterator->structValue->end())
						{
							if(portIterator->second->type == Flows::VariableType::tString) port = BaseLib::Math::getNumber(portIterator->second->stringValue);
							else port = portIterator->second->integerValue;
						}
						Wire wire;
						wire.id = idIterator->second->stringValue;
						wire.port = port;
						output.push_back(wire);
					}
					node->wiresOut.push_back(output);
				}
			}

			flow->nodes.emplace(node->id, node);

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
						wire.port = outputIndex;
						nodeIterator->second->wiresIn.at(wireOut.port).push_back(wire);
					}
					outputIndex++;
				}
			}
		//}}}

		_nodesStopped = false;

		//{{{ Init nodes
			{

				std::vector<PNodeInfo> nodes;
				nodes.reserve(_nodes.size());
				{
					std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
					for(auto& node : _nodes)
					{
						nodes.push_back(node.second);
					}
				}
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
					nodeObject->setInvokeNodeMethod(std::function<Flows::PVariable(std::string, std::string, Flows::PArray)>(std::bind(&FlowsClient::invokeNodeMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setSubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::subscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
					nodeObject->setUnsubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::unsubscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
					nodeObject->setOutput(std::function<void(std::string, uint32_t, Flows::PVariable)>(std::bind(&FlowsClient::queueOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setNodeEvent(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::nodeEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setGetNodeData(std::function<Flows::PVariable(std::string, std::string)>(std::bind(&FlowsClient::getNodeData, this, std::placeholders::_1, std::placeholders::_2)));
					nodeObject->setSetNodeData(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::setNodeData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
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
		_shuttingDown = true;
		_out.printMessage("Calling stop()...");
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		for(auto& flow : _flows)
		{
			for(auto& nodeIterator : flow.second->nodes)
			{
				Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
				if(_bl->debugLevel >= 5) _out.printDebug("Debug: Calling stop() on node " + nodeIterator.second->id + "...");
				if(node) node->stop();
			}
		}
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
		if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

		queueOutput(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2));
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
		if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");

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
			if(innerParameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
			if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
			if(innerParameters->at(1)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 2 is not of type string.");
			if(innerParameters->at(2)->type != Flows::VariableType::tArray) return Flows::Variable::createError(-1, "Parameter 3 is not of type array.");

			return invokeNodeMethod(innerParameters->at(0)->stringValue, innerParameters->at(1)->stringValue, innerParameters->at(2)->arrayValue);
		}
		else if(methodName == "subscribePeer")
		{
			if(innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
			if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
			if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
			if(innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
			if(innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

			subscribePeer(innerParameters->at(0)->stringValue, innerParameters->at(1)->integerValue64, innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
			return std::make_shared<Flows::Variable>();
		}
		else if(methodName == "unsubscribePeer")
		{
			if(innerParameters->size() != 4) return Flows::Variable::createError(-1, "Wrong parameter count.");
			if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
			if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");
			if(innerParameters->at(2)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 3 is not of type integer.");
			if(innerParameters->at(3)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 4 is not of type string.");

			unsubscribePeer(innerParameters->at(0)->stringValue, innerParameters->at(1)->integerValue64, innerParameters->at(2)->integerValue, innerParameters->at(3)->stringValue);
			return std::make_shared<Flows::Variable>();
		}
		else if(methodName == "output")
		{
			if(innerParameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
			if(innerParameters->at(0)->type != Flows::VariableType::tString) return Flows::Variable::createError(-1, "Parameter 1 is not of type string.");
			if(innerParameters->at(1)->type != Flows::VariableType::tInteger64) return Flows::Variable::createError(-1, "Parameter 2 is not of type integer.");

			queueOutput(innerParameters->at(0)->stringValue, innerParameters->at(1)->integerValue, innerParameters->at(2));
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
		uint64_t peerId = parameters->at(0)->integerValue64;
		int32_t channel = parameters->at(1)->integerValue;

		auto peerIterator = _peerSubscriptions.find(peerId);
		if(peerIterator == _peerSubscriptions.end()) return Flows::PVariable(new Flows::Variable());

		auto channelIterator = peerIterator->second.find(channel);
		if(channelIterator == peerIterator->second.end()) return Flows::PVariable(new Flows::Variable());

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
