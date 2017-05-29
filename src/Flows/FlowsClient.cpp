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

FlowsClient::FlowsClient() : IQueue(GD::bl.get(), 2, 1000)
{
	_stopped = false;
	_frontendConnected = false;

	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());
	_out.setPrefix("Flows Engine (" + std::to_string(getpid()) + "): ");

	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_nodeManager = std::unique_ptr<NodeManager>(new NodeManager(&_frontendConnected));

	_binaryRpc = std::unique_ptr<Flows::BinaryRpc>(new Flows::BinaryRpc());
	_rpcDecoder = std::unique_ptr<Flows::RpcDecoder>(new Flows::RpcDecoder());
	_rpcEncoder = std::unique_ptr<Flows::RpcEncoder>(new Flows::RpcEncoder(true));

	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("reload", std::bind(&FlowsClient::reload, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("shutdown", std::bind(&FlowsClient::shutdown, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("startFlow", std::bind(&FlowsClient::startFlow, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("startNodes", std::bind(&FlowsClient::startNodes, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("configNodesStarted", std::bind(&FlowsClient::configNodesStarted, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("stopFlow", std::bind(&FlowsClient::stopFlow, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("flowCount", std::bind(&FlowsClient::flowCount, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("nodeOutput", std::bind(&FlowsClient::nodeOutput, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("invokeNodeMethod", std::bind(&FlowsClient::invokeExternalNodeMethod, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("setNodeVariable", std::bind(&FlowsClient::setNodeVariable, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("enableNodeEvents", std::bind(&FlowsClient::enableNodeEvents, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("disableNodeEvents", std::bind(&FlowsClient::disableNodeEvents, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("broadcastEvent", std::bind(&FlowsClient::broadcastEvent, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("broadcastDeleteDevices", std::bind(&FlowsClient::broadcastDeleteDevices, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("broadcastNewDevices", std::bind(&FlowsClient::broadcastNewDevices, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>("broadcastUpdateDevice", std::bind(&FlowsClient::broadcastUpdateDevice, this, std::placeholders::_1)));
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
		GD::bl->shuttingDown = true;

		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Wait for shutdown response to be sent.

		stopQueue(0);
		stopQueue(1);

		_stopped = true;

		_flows.clear();
		_rpcResponses.clear();
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

		startQueue(0, 5, 0, SCHED_OTHER);
		startQueue(1, GD::bl->settings.flowsProcessingThreadCountNodes(), 0, SCHED_OTHER);

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
						std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(_binaryRpc->getData(), _binaryRpc->getType() == Flows::BinaryRpc::Type::request));
						enqueue(0, queueEntry);
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
		Flows::PVariable result = invoke(methodName, parameters);
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

		if(index == 0) //IPC
		{
			if(queueEntry->isRequest)
			{
				std::string methodName;
				Flows::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

				if(parameters->size() < 2)
				{
					_out.printError("Error: Wrong parameter count while calling method " + methodName);
					return;
				}
				std::map<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
				if(localMethodIterator == _localRpcMethods.end())
				{
					_out.printError("Warning: RPC method not found: " + methodName);
					Flows::PVariable error = Flows::Variable::createError(-32601, "Requested method not found.");
					sendResponse(parameters->at(0), error);
					return;
				}

				if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Server is calling RPC method: " + methodName);

				Flows::PVariable result = localMethodIterator->second(parameters->at(1)->arrayValue);
				if(GD::bl->debugLevel >= 5)
				{
					_out.printDebug("Response: ");
					result->print(true, false);
				}
				sendResponse(parameters->at(0), result);
			}
			else
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
						PFlowsResponse element = responseIterator->second;
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
		}
		else //Node output
		{
			if(!queueEntry->nodeInfo || !queueEntry->message) return;
			Flows::PINode node = _nodeManager->getNode(queueEntry->nodeInfo->id);
			if(node)
			{
				Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
				timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(1000));
				nodeEvent(queueEntry->nodeInfo->id, "highlightNode/" + queueEntry->nodeInfo->id, timeout);
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
				_out.printError("Could not send data to client " + std::to_string(_fileDescriptor->descriptor) + ". Sent bytes: " + std::to_string(totallySentBytes) + " of " + std::to_string(data.size()) + (sentBytes == -1 ? ". Error message: " + std::string(strerror(errno)) : ""));
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

Flows::PVariable FlowsClient::invoke(std::string methodName, Flows::PArray& parameters)
{
	try
	{
		int64_t threadId = pthread_self();
		std::unique_lock<std::mutex> requestInfoGuard(_requestInfoMutex);
		RequestInfo& requestInfo = _requestInfo[threadId];
		requestInfoGuard.unlock();

		int32_t packetId;
		{
			std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
			packetId = _currentPacketId++;
		}
		Flows::PArray array(new Flows::Array{ Flows::PVariable(new Flows::Variable(threadId)), Flows::PVariable(new Flows::Variable(packetId)), Flows::PVariable(new Flows::Variable(parameters)) });
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, array, data);

		PFlowsResponse response;
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			auto result = _rpcResponses[threadId].emplace(packetId, std::make_shared<FlowsResponse>());
			if(result.second) response = result.first->second;
		}
		if(!response)
		{
			_out.printError("Critical: Could not insert response struct into map.");
			return Flows::Variable::createError(-32500, "Unknown application error.");
		}

		Flows::PVariable result = send(data);
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[threadId].erase(packetId);
			if (_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
			return result;
		}

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

Flows::PVariable FlowsClient::invokeNodeMethod(std::string nodeId, std::string methodName, Flows::PArray& parameters)
{
	try
	{
		Flows::PINode node = _nodeManager->getNode(nodeId);
		if(node) return node->invokeLocal(methodName, parameters);

		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(nodeId));
		parameters->push_back(std::make_shared<Flows::Variable>(methodName));
		parameters->push_back(std::make_shared<Flows::Variable>(parameters));

		return invoke("invokeNodeMethod", parameters);
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
		if(!message) return;

		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		auto nodesIterator = _nodes.find(nodeId);
		if(nodesIterator == _nodes.end()) return;

		if(message->structValue->find("payload") == message->structValue->end()) message->structValue->emplace("payload", std::make_shared<Flows::Variable>());

		if(index >= nodesIterator->second->wiresOut.size())
		{
			_out.printError("Error: " + nodeId + " has no output with index " + std::to_string(index) + ".");
			return;
		}

		message->structValue->emplace("source", std::make_shared<Flows::Variable>(nodeId));
		for(auto& node : nodesIterator->second->wiresOut.at(index))
		{
			auto nodeIterator = _nodes.find(node.id);
			if(nodeIterator == _nodes.end()) continue;
			std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(nodeIterator->second, node.port, message);
			enqueue(1, queueEntry);
		}

		Flows::PVariable timeout = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
		timeout->structValue->emplace("timeout", std::make_shared<Flows::Variable>(500));
		nodeEvent(nodeId, "highlightNode/" + nodeId, timeout);
		Flows::PVariable outputIndex = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
		outputIndex->structValue->emplace("index", std::make_shared<Flows::Variable>(index));
		nodeEvent(nodeId, "highlightLink/" + nodeId, outputIndex);
		Flows::PVariable status = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
		status->structValue->emplace("text", std::make_shared<Flows::Variable>(std::to_string(index) + ": " + message->structValue->at("payload")->toString()));
		status->structValue->emplace("position", std::make_shared<Flows::Variable>("top"));
		nodeEvent(nodeId, "statusTop/" + nodeId, status);
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

		Flows::PVariable result = invoke("nodeEvent", parameters);
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

		Flows::PVariable result = invoke("getNodeData", parameters);
		if(result->errorStruct)
		{
			GD::out.printError("Error calling setNodeData: " + result->structValue->at("faultString")->stringValue);
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

		Flows::PVariable result = invoke("setNodeData", parameters);
		if(result->errorStruct) GD::out.printError("Error calling setNodeData: " + result->structValue->at("faultString")->stringValue);
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

		return Flows::PVariable(new Flows::Variable());
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
		{
			std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
			for(auto& flow : _flows)
			{
				for(auto& nodeIterator : flow.second->nodes)
				{
					Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
					if(node) node->stop();
				}
			}
		}
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

		if(_maintenanceThread.joinable()) _maintenanceThread.join();
		_maintenanceThread = std::thread(&FlowsClient::dispose, this);

		return Flows::PVariable(new Flows::Variable());
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

		PFlowInfoClient flow = std::make_shared<FlowInfoClient>();
		flow->id = parameters->at(0)->integerValue;

		_out.printInfo("Info: Starting flow with ID " + std::to_string(flow->id) + "...");

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

		//{{{ Start nodes
			{
				std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
				for(auto& node : _nodes)
				{
					if(_bl->debugLevel >= 5) _out.printDebug("Starting node " + node.second->id + " of type " + node.second->type + ".");

					Flows::PINode nodeObject;
					int32_t result = _nodeManager->loadNode(node.second->nodeNamespace, node.second->type, node.second->id, nodeObject);
					if(result < 0)
					{
						_out.printError("Error: Could not load node " + node.second->type + ". Error code: " + std::to_string(result));
						continue;
					}

					nodeObject->setId(node.second->id);

					nodeObject->setLog(std::function<void(std::string, int32_t, std::string)>(std::bind(&FlowsClient::log, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setInvoke(std::function<Flows::PVariable(std::string, Flows::PArray&)>(std::bind(&FlowsClient::invoke, this, std::placeholders::_1, std::placeholders::_2)));
					nodeObject->setInvokeNodeMethod(std::function<Flows::PVariable(std::string, std::string, Flows::PArray&)>(std::bind(&FlowsClient::invokeNodeMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setSubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::subscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
					nodeObject->setUnsubscribePeer(std::function<void(std::string, uint64_t, int32_t, std::string)>(std::bind(&FlowsClient::unsubscribePeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
					nodeObject->setOutput(std::function<void(std::string, uint32_t, Flows::PVariable)>(std::bind(&FlowsClient::queueOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setNodeEvent(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::nodeEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setGetNodeData(std::function<Flows::PVariable(std::string, std::string)>(std::bind(&FlowsClient::getNodeData, this, std::placeholders::_1, std::placeholders::_2)));
					nodeObject->setSetNodeData(std::function<void(std::string, std::string, Flows::PVariable)>(std::bind(&FlowsClient::setNodeData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
					nodeObject->setGetConfigParameter(std::function<Flows::PVariable(std::string, std::string)>(std::bind(&FlowsClient::getConfigParameter, this, std::placeholders::_1, std::placeholders::_2)));

					if(!nodeObject->init(node.second))
					{
						_out.printError("Error: Could not load node " + node.second->type + ". Start failed.");
						continue;
					}
				}
			}
		//}}}

		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		_flows.emplace(flow->id, flow);

		return Flows::PVariable(new Flows::Variable());
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
			for(auto& nodeIterator : flow.second->nodes)
			{
				Flows::PINode node = _nodeManager->getNode(nodeIterator.second->id);
				if(node) node->start();
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

Flows::PVariable FlowsClient::setNodeVariable(Flows::PArray& parameters)
{
	try
	{
		if(parameters->size() != 3) return Flows::Variable::createError(-1, "Wrong parameter count.");
		Flows::PINode node = _nodeManager->getNode(parameters->at(0)->stringValue);
		if(node) node->setNodeVariable(parameters->at(1)->stringValue, parameters->at(2));
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

		return Flows::PVariable(new Flows::Variable());
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

		return Flows::PVariable(new Flows::Variable());
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

		return Flows::PVariable(new Flows::Variable());
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

		return Flows::PVariable(new Flows::Variable());
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

		return Flows::PVariable(new Flows::Variable());
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

		return Flows::PVariable(new Flows::Variable());
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
