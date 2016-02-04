/* Copyright 2013-2016 Sathya Laufer
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

#include "ScriptEngineClient.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

ScriptEngineClient::ScriptEngineClient() : IQueue(GD::bl.get(), 1000)
{
	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine (" + std::to_string(getpid()) + "): ");

	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
}

ScriptEngineClient::~ScriptEngineClient()
{
}

void ScriptEngineClient::cleanUp()
{
	try
	{
		stopQueue(0);
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

void ScriptEngineClient::start()
{
	try
	{
		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.log").c_str(), "a", stdout))
		{
			_out.printError("Error: Could not redirect output to log file.");
		}
		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.err").c_str(), "a", stderr))
		{
			_out.printError("Error: Could not redirect errors to log file.");
		}

		startQueue(0, GD::bl->settings.scriptEngineThreadCount(), 0, SCHED_OTHER);

		_socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
		for(int32_t i = 0; i < 2; i++)
		{
			_fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM, 0));
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
				GD::bl->fileDescriptorManager.shutdown(_fileDescriptor);
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
		if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Connected.");

		if(_registerClientThread.joinable()) _registerClientThread.join();
		_registerClientThread = std::thread(&ScriptEngineClient::registerClient, this);

		std::vector<char> buffer(1025);
		std::vector<uint8_t> packet;
		int32_t bytesRead = 0;
		uint32_t receivedDataLength = 0;
		uint32_t packetLength = 0;
		bool packetIsRequest = false;
		while(true)
		{
			bytesRead = read(_fileDescriptor->descriptor, &buffer[0], 1024);
			if(bytesRead <= 0)
			{
				_out.printInfo("Info: Connection to script server closed.");
				cleanUp();
				exit(0);
			}

			if(bytesRead < 8) return;
			if(bytesRead > (signed)buffer.size() - 1) bytesRead = buffer.size() - 1;
			buffer.at(bytesRead) = 0;

			if(receivedDataLength == 0 && !strncmp(&buffer[0], "Bin", 3))
			{
				packetIsRequest = !(buffer[3] & 1);
				GD::bl->hf.memcpyBigEndian((char*)&packetLength, &buffer[4], 4);
				if(packetLength == 0) return;
				if(packetLength > 10485760)
				{
					_out.printError("Error: Packet with data larger than 10 MiB received.");
					return;
				}
				packet.clear();
				packet.reserve(packetLength + 9);
				packet.insert(packet.end(), &buffer[0], &buffer[0] + bytesRead);
				if(packetLength > (unsigned)bytesRead - 8) receivedDataLength = bytesRead - 8;
				else
				{
					receivedDataLength = 0;
					std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(packet,packetIsRequest));
					enqueue(0, queueEntry);
				}
			}
			else if(receivedDataLength > 0)
			{
				if(receivedDataLength + bytesRead > packetLength)
				{
					_out.printError("Error: Packet length is wrong.");
					receivedDataLength = 0;
					return;
				}
				packet.insert(packet.end(), &buffer[0], &buffer[0] + bytesRead);
				receivedDataLength += bytesRead;
				if(receivedDataLength == packetLength)
				{
					packet.push_back('\0');
					packetLength = 0;
					std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(packet, packetIsRequest));
					enqueue(0, queueEntry);
				}
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
    return;
}

void ScriptEngineClient::registerClient()
{
	try
	{
		std::string methodName("registerScriptEngineClient");
		std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>{BaseLib::PVariable(new BaseLib::Variable((int32_t)getpid()))});
		BaseLib::PVariable result = sendGlobalRequest(methodName, parameters);
		if(result->errorStruct)
		{
			_out.printCritical("Critical: Could not register client.");
			cleanUp();
			exit(1);
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

void ScriptEngineClient::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry) return;

		if(queueEntry->isRequest)
		{
			std::string methodName;
			BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

			std::map<std::string, std::shared_ptr<RPC::RPCMethod>>::iterator methodIterator = _rpcMethods.find(methodName);
			if(methodIterator == _rpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + methodName);
				BaseLib::PVariable result = BaseLib::Variable::createError(-32601, ": Requested method not found.");
				sendResponse(result);
				return;
			}
			if(GD::bl->debugLevel >= 4)
			{
				_out.printInfo("Info: Server is calling RPC method: " + methodName + " Parameters:");
				for(std::vector<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i)
				{
					(*i)->print();
				}
			}
			BaseLib::PVariable result = _rpcMethods.at(methodName)->invoke(_dummyClientInfo, parameters);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print();
			}

			sendResponse(result);
		}
		else
		{
			_rpcResponse = _rpcDecoder->decodeResponse(queueEntry->packet);
			_requestConditionVariable.notify_one();
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

BaseLib::PVariable ScriptEngineClient::sendGlobalRequest(std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	try
	{
		std::unique_lock<std::mutex> requestLock(_requestMutex);
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, parameters, data);

		for(int32_t i = 0; i < 5; i++)
		{
			_rpcResponse.reset();
			int32_t totallySentBytes = 0;
			while (totallySentBytes < (signed)data.size())
			{
				int32_t sentBytes = ::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
				if(sentBytes == -1)
				{
					GD::out.printError("Could not send data to client: " + std::to_string(_fileDescriptor->descriptor));
					break;
				}
				totallySentBytes += sentBytes;
			}
			_requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(5000), [&]{ return (bool)_rpcResponse; });
			if(_rpcResponse) return _rpcResponse;
			else if(i == 4) _out.printError("Error: No response received to RPC request. Method: " + methodName);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineClient::sendResponse(BaseLib::PVariable& variable)
{
	try
	{
		std::vector<char> data;
		_rpcEncoder->encodeResponse(variable, data);
		int32_t totallySentBytes = 0;
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sentBytes = ::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
			if(sentBytes == -1)
			{
				GD::out.printError("Could not send data to client: " + std::to_string(_fileDescriptor->descriptor));
				break;
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
}
