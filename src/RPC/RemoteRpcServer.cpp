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

#include "RemoteRpcServer.h"
#include "../GD/GD.h"

namespace Homegear
{

namespace Rpc
{

RemoteRpcServer::RemoteRpcServer(BaseLib::PRpcClientInfo& serverClientInfo)
{
	_serverClientInfo = serverClientInfo;

	if(_serverClientInfo->sendEventsToRpcServer)
	{
		_rpcEncoder = std::make_shared<BaseLib::Rpc::RpcEncoder>(GD::bl.get(), true, true);
		_jsonEncoder = std::make_shared<BaseLib::Rpc::JsonEncoder>(GD::bl.get());
		_xmlRpcEncoder = std::make_shared<BaseLib::Rpc::XmlrpcEncoder>(GD::bl.get());
	}
	else
	{
		socket = std::shared_ptr<BaseLib::TcpSocket>(new BaseLib::TcpSocket(GD::bl.get()));
		socket->setReadTimeout(15000000);
		socket->setWriteTimeout(15000000);
		knownDevices.reset(new std::set<uint64_t>());
		fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
		path = "/RPC2";
	}

	_stopMethodProcessingThread = false;
	_methodBufferHead = 0;
	_methodBufferTail = 0;
	if(!GD::bl->threadManager.start(_methodProcessingThread, false, GD::bl->settings.rpcClientThreadPriority(), GD::bl->settings.rpcClientThreadPolicy(), &RemoteRpcServer::processMethods, this))
	{
		removed = true;
	}

	_droppedEntries = 0;
	_lastQueueFullError = 0;
}

RemoteRpcServer::RemoteRpcServer(std::shared_ptr<RpcClient>& client, BaseLib::PRpcClientInfo& serverClientInfo)
		: RemoteRpcServer(serverClientInfo)
{
	removed = false;
	initialized = false;
	_client = client;
}

RemoteRpcServer::~RemoteRpcServer()
{
	removed = true;
	if(_stopMethodProcessingThread) return;
	_stopMethodProcessingThread = true;
	_methodProcessingMessageAvailable = true;
	_methodProcessingConditionVariable.notify_one();
	GD::bl->threadManager.join(_methodProcessingThread);
	_client.reset();
}

void RemoteRpcServer::queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> method)
{
	try
	{
		if(removed) return;
		std::unique_lock<std::mutex> lock(_methodProcessingThreadMutex);
		int32_t tempHead = _methodBufferHead + 1;
		if(tempHead >= _methodBufferSize) tempHead = 0;
		if(tempHead == _methodBufferTail)
		{
			uint32_t droppedEntries = ++_droppedEntries;
			if(BaseLib::HelperFunctions::getTime() - _lastQueueFullError > 10000)
			{
				_lastQueueFullError = BaseLib::HelperFunctions::getTime();
				_droppedEntries = 0;
				std::cout << "Error: More than " << std::to_string(_methodBufferSize)
						  << " methods are queued to be sent to server " << address.first
						  << ". Your packet processing is too slow. Dropping method. This message won't repeat for 10 seconds. Dropped outputs since last message: "
						  << droppedEntries << std::endl;
				std::cerr << "Error: More than " << std::to_string(_methodBufferSize)
						  << " methods are queued to be sent to server " << address.first
						  << ". Your packet processing is too slow. Dropping method. This message won't repeat for 10 seconds. Dropped outputs since last message: "
						  << droppedEntries << std::endl;
			}
			return;
		}

		_methodBuffer[_methodBufferHead] = method;
		_methodBufferHead++;
		if(_methodBufferHead >= _methodBufferSize)
		{
			_methodBufferHead = 0;
		}
		_methodProcessingMessageAvailable = true;

		lock.unlock();
		_methodProcessingConditionVariable.notify_one();
	}
	catch(const std::exception& ex)
	{
		//Don't use the output object here => would cause deadlock because of error callback which is calling queueMethod again.
		std::cout << "Error in file " << __FILE__ << " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__
				  << ": " << ex.what() << std::endl;
		std::cerr << "Error in file " << __FILE__ << " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__
				  << ": " << ex.what() << std::endl;
	}
}

void RemoteRpcServer::processMethods()
{
	std::unique_lock<std::mutex> lock(_methodProcessingThreadMutex);
	while(!_stopMethodProcessingThread)
	{
		try
		{
			_methodProcessingConditionVariable.wait(lock, [&] { return _methodProcessingMessageAvailable || _stopMethodProcessingThread; });
			if(_stopMethodProcessingThread) return;

			while(_methodBufferHead != _methodBufferTail)
			{
				std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> message = _methodBuffer[_methodBufferTail];
				_methodBuffer[_methodBufferTail].reset();
				_methodBufferTail++;
				if(_methodBufferTail >= _methodBufferSize) _methodBufferTail = 0;
				if(_methodBufferHead == _methodBufferTail) _methodProcessingMessageAvailable = false; //Set here, because otherwise it might be set to "true" in publish and then set to false again after the while loop
				lock.unlock();
				if(!removed)
				{
					if(_serverClientInfo->sendEventsToRpcServer)
					{
						invokeClientMethod(message->first, message->second);
					}
					else if(_client)
					{
						_client->invokeBroadcast(this, message->first, message->second);
					}
					else removed = true;
				}
				lock.lock();
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
	}
}

BaseLib::PVariable RemoteRpcServer::invoke(std::string& methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	if(_serverClientInfo->sendEventsToRpcServer) return invokeClientMethod(methodName, parameters);
	else if(_client) return _client->invoke(this, methodName, parameters);
	else
	{
		removed = true;
		return BaseLib::Variable::createError(-32501, "Client was removed.");
	}
}

BaseLib::PVariable RemoteRpcServer::invokeClientMethod(std::string& methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	try
	{
		if(_serverClientInfo->closed)
		{
			removed = true;
			return BaseLib::Variable::createError(-32501, "Unknown client.");
		}

		std::lock_guard<std::mutex> invokeGuard(_serverClientInfo->invokeMutex);

		std::unique_lock<std::mutex> requestLock(_serverClientInfo->requestMutex);
		_serverClientInfo->rpcResponse.reset();
		_serverClientInfo->waitForResponse = true;

		std::vector<char> encodedPacket;

		if(binary)
		{
			_rpcEncoder->encodeRequest(methodName, parameters, encodedPacket);
			if(GD::bl->debugLevel >= 5)
			{
				GD::out.printDebug("Debug: Calling RPC method \"" + methodName + "\" on client " + _serverClientInfo->address + " (client ID " + std::to_string(_serverClientInfo->id) + ").");
				GD::out.printDebug("Parameters:");
				for(auto& parameter : *parameters)
				{
					parameter->print(true, false);
				}
			}
		}
		else if(webSocket)
		{
			std::vector<char> json;
			_jsonEncoder->encodeRequest(methodName, parameters, json);
			BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, encodedPacket);
		}
		else
		{
			if(json) _jsonEncoder->encodeRequest(methodName, parameters, encodedPacket);
			else _xmlRpcEncoder->encodeRequest(methodName, parameters, encodedPacket);

			const std::string header = "POST " + path + " HTTP/1.1\r\nUser-Agent: Homegear " + GD::baseLibVersion + "\r\nHost: " + hostname + ":" + address.second + "\r\nContent-Type: " + (json ? "application/json" : "text/xml") + "\r\nContent-Length: " + std::to_string(encodedPacket.size() + 2) + "\r\nConnection: Keep-Alive\r\n\r\n";
			encodedPacket.reserve(encodedPacket.size() + header.size() + 2);
			encodedPacket.push_back('\r');
			encodedPacket.push_back('\n');

			encodedPacket.insert(encodedPacket.begin(), header.begin(), header.end());
		}

		_serverClientInfo->socket->proofwrite(encodedPacket);

		auto startTime = BaseLib::HelperFunctions::getTime();
		while(!_serverClientInfo->requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(1000), [&]
		{
			return _serverClientInfo->rpcResponse || _serverClientInfo->closed || BaseLib::HelperFunctions::getTime() - startTime > 10000;
		}));
		_serverClientInfo->waitForResponse = false;
		if(!_serverClientInfo->rpcResponse) return BaseLib::Variable::createError(-32500, "No RPC response received.");

		if(_serverClientInfo->closed) removed = true;

		return _serverClientInfo->rpcResponse;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}

}