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
#include "php_sapi.h"
#include "PhpEvents.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <wordexp.h>

namespace ScriptEngine
{

std::mutex ScriptEngineClient::_resourceMutex;

ScriptEngineClient::ScriptEngineClient() : IQueue(GD::bl.get(), 1, 1000)
{
	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine (" + std::to_string(getpid()) + "): ");

#ifdef DEBUGSESOCKET
	std::string socketLogfile = GD::bl->settings.logfilePath() + "homegear-socket-client-" + std::to_string((uint32_t)getpid()) + ".pcap";
	BaseLib::Io::deleteFile(socketLogfile);
	_socketOutput.open(socketLogfile, std::ios::app | std::ios::binary);
	std::vector<uint8_t> buffer{ 0xa1, 0xb2, 0xc3, 0xd4, 0, 2, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0xe4 };
	_socketOutput.write((char*)buffer.data(), buffer.size());
#endif

	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_binaryRpc = std::unique_ptr<BaseLib::Rpc::BinaryRpc>(new BaseLib::Rpc::BinaryRpc(GD::bl.get()));
	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("reload", std::bind(&ScriptEngineClient::reload, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("shutdown", std::bind(&ScriptEngineClient::shutdown, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("executeScript", std::bind(&ScriptEngineClient::executeScript, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("scriptCount", std::bind(&ScriptEngineClient::scriptCount, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("getRunningScripts", std::bind(&ScriptEngineClient::getRunningScripts, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("broadcastEvent", std::bind(&ScriptEngineClient::broadcastEvent, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("broadcastNewDevices", std::bind(&ScriptEngineClient::broadcastNewDevices, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("broadcastDeleteDevices", std::bind(&ScriptEngineClient::broadcastDeleteDevices, this, std::placeholders::_1)));
	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("broadcastUpdateDevice", std::bind(&ScriptEngineClient::broadcastUpdateDevice, this, std::placeholders::_1)));

	php_homegear_init();
}

ScriptEngineClient::~ScriptEngineClient()
{
	dispose(true);
	if(_maintenanceThread.joinable()) _maintenanceThread.join();
#ifdef DEBUGSESOCKET
	_socketOutput.close();
#endif
}

void ScriptEngineClient::stopEventThreads()
{
	std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
	for(std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
	{
		if(i->second) i->second->stop();
	}
}

void ScriptEngineClient::dispose(bool broadcastShutdown)
{
	try
	{
		if(_disposing) return;
		std::lock_guard<std::mutex> disposeGuard(_disposeMutex);

		BaseLib::PArray eventData(new BaseLib::Array{ BaseLib::PVariable(new BaseLib::Variable(0)), BaseLib::PVariable(new BaseLib::Variable(-1)), BaseLib::PVariable(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(std::string("DISPOSING")))}))), BaseLib::PVariable(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(true))}))) });

		GD::bl->shuttingDown = true;
		if(broadcastShutdown) broadcastEvent(eventData);

		int32_t i = 0;
		while(_scriptThreads.size() > 0 && i < 31)
		{
			if(i > 0)
			{
				GD::out.printInfo("Info: Waiting for script threads to finish (1). Scripts still running: " + std::to_string(_scriptThreads.size()));

				if(i % 10 == 0)
				{
					std::string ids = "IDs of running scripts: ";
					std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
					for(std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
					{
						ids.append(std::to_string(i->first) + " ");
					}
					GD::out.printInfo(ids);
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			collectGarbage();
			i++;
		}
		if(i == 30)
		{
			GD::out.printError("Error: At least one script did not finish within 30 seconds during shutdown. Killing myself.");
			kill(getpid(), 9);
			return;
		}

		_disposing = true;
		stopEventThreads();
		stopQueue(0);
		php_homegear_shutdown();
		_scriptCache.clear();
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

#ifdef DEBUGSESOCKET
void ScriptEngineClient::socketOutput(int32_t packetId, bool clientRequest, bool request, std::vector<char> data)
{
	try
	{
		int64_t time = BaseLib::HelperFunctions::getTimeMicroseconds();
		int32_t timeSeconds = time / 1000000;
		int32_t timeMicroseconds = time % 1000000;

		uint32_t length = 20 + 8 + data.size();

		std::vector<uint8_t> buffer;
		buffer.reserve(length);
		buffer.push_back((uint8_t)(timeSeconds >> 24));
		buffer.push_back((uint8_t)(timeSeconds >> 16));
		buffer.push_back((uint8_t)(timeSeconds >> 8));
		buffer.push_back((uint8_t)timeSeconds);

		buffer.push_back((uint8_t)(timeMicroseconds >> 24));
		buffer.push_back((uint8_t)(timeMicroseconds >> 16));
		buffer.push_back((uint8_t)(timeMicroseconds >> 8));
		buffer.push_back((uint8_t)timeMicroseconds);

		buffer.push_back((uint8_t)(length >> 24)); //incl_len
		buffer.push_back((uint8_t)(length >> 16));
		buffer.push_back((uint8_t)(length >> 8));
		buffer.push_back((uint8_t)length);

		buffer.push_back((uint8_t)(length >> 24)); //orig_len
		buffer.push_back((uint8_t)(length >> 16));
		buffer.push_back((uint8_t)(length >> 8));
		buffer.push_back((uint8_t)length);

		//{{{ IPv4 header
			buffer.push_back(0x45); //Version 4 (0100....); Header length 20 (....0101)
			buffer.push_back(0); //Differentiated Services Field

			buffer.push_back((uint8_t)(length >> 8)); //Length
			buffer.push_back((uint8_t)length);

			buffer.push_back((uint8_t)((packetId % 65536) >> 8)); //Identification
			buffer.push_back((uint8_t)(packetId % 65536));

			buffer.push_back(0); //Flags: 0 (000.....); Fragment offset 0 (...00000 00000000)
			buffer.push_back(0);

			buffer.push_back(0x80); //TTL

			buffer.push_back(17); //Protocol UDP

			buffer.push_back(0); //Header checksum
			buffer.push_back(0);

			int32_t clientId = getpid();
			if(request)
			{
				if(clientRequest)
				{
					buffer.push_back(0x80 | (uint8_t)(clientId >> 24)); //Source
					buffer.push_back((uint8_t)(clientId >> 16));
					buffer.push_back((uint8_t)(clientId >> 8));
					buffer.push_back((uint8_t)clientId);

					buffer.push_back(2); //Destination
					buffer.push_back(2);
					buffer.push_back(2);
					buffer.push_back(2);
				}
				else
				{
					buffer.push_back(1); //Source
					buffer.push_back(1);
					buffer.push_back(1);
					buffer.push_back(1);

					buffer.push_back(0x80 | (uint8_t)(clientId >> 24)); //Destination
					buffer.push_back((uint8_t)(clientId >> 16));
					buffer.push_back((uint8_t)(clientId >> 8));
					buffer.push_back((uint8_t)clientId);
				}
			}
			else
			{
				if(clientRequest)
				{
					buffer.push_back(2); //Source
					buffer.push_back(2);
					buffer.push_back(2);
					buffer.push_back(2);

					buffer.push_back(0x80 | (uint8_t)(clientId >> 24)); //Destination
					buffer.push_back((uint8_t)(clientId >> 16));
					buffer.push_back((uint8_t)(clientId >> 8));
					buffer.push_back((uint8_t)clientId);
				}
				else
				{
					buffer.push_back(0x80 | (uint8_t)(clientId >> 24)); //Source
					buffer.push_back((uint8_t)(clientId >> 16));
					buffer.push_back((uint8_t)(clientId >> 8));
					buffer.push_back((uint8_t)clientId);

					buffer.push_back(1); //Destination
					buffer.push_back(1);
					buffer.push_back(1);
					buffer.push_back(1);
				}
			}
		// }}}
		// {{{ UDP header
			buffer.push_back(0); //Source port
			buffer.push_back(1);

			buffer.push_back((uint8_t)(clientId >> 8)); //Destination port
			buffer.push_back((uint8_t)clientId);

			length -= 20;
			buffer.push_back((uint8_t)(length >> 8)); //Length
			buffer.push_back((uint8_t)length);

			buffer.push_back(0); //Checksum
			buffer.push_back(0);
		// }}}

		buffer.insert(buffer.end(), data.begin(), data.end());

		std::lock_guard<std::mutex> socketOutputGuard(_socketOutputMutex);
		_socketOutput.write((char*)buffer.data(), buffer.size());
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
#endif

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

		startQueue(0, 5, 0, SCHED_OTHER);

		_socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
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
		if(GD::bl->debugLevel >= 4) _out.printMessage("Connected.");

		if(_maintenanceThread.joinable()) _maintenanceThread.join();
		_maintenanceThread = std::thread(&ScriptEngineClient::registerClient, this);

		std::vector<char> buffer(1024);
		int32_t result = 0;
		int32_t bytesRead = 0;
		int32_t processedBytes = 0;
		while(!_disposing)
		{
			try
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
				if(result == 0)
				{
					if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000) collectGarbage();
					continue;
				}
				else if(result == -1)
				{
					if(errno == EINTR) continue;
					GD::bl->fileDescriptorManager.close(_fileDescriptor);
					_out.printMessage("Connection to script server closed (1). Exiting.");
					return;
				}

				bytesRead = read(_fileDescriptor->descriptor, buffer.data(), buffer.size());
				if(bytesRead <= 0) //read returns 0, when connection is disrupted.
				{
					GD::bl->fileDescriptorManager.close(_fileDescriptor);
					_out.printMessage("Connection to script server closed (2). Exiting.");
					return;
				}

				if(bytesRead > (signed)buffer.size()) bytesRead = buffer.size();

				try
				{
					processedBytes = 0;
					while(processedBytes < bytesRead)
					{
						processedBytes += _binaryRpc->process(buffer.data() + processedBytes, bytesRead - processedBytes);
						if(_binaryRpc->isFinished())
						{
	#ifdef DEBUGSESOCKET
							if(_binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request)
							{
								std::string methodName;
								BaseLib::PArray request = _rpcDecoder->decodeRequest(_binaryRpc->getData(), methodName);
								socketOutput(request->at(0)->integerValue, false, true, _binaryRpc->getData());
							}
							else
							{
								BaseLib::PVariable response = _rpcDecoder->decodeResponse(_binaryRpc->getData());
								socketOutput(response->arrayValue->at(1)->integerValue, true, false, _binaryRpc->getData());
							}
	#endif
							std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(_binaryRpc->getData(), _binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request);
							enqueue(0, queueEntry);
							_binaryRpc->reset();
						}
					}
				}
				catch(BaseLib::Rpc::BinaryRpcException& ex)
				{
					_out.printError("Error processing packet: " + ex.what());
					_binaryRpc->reset();
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

std::vector<std::string> ScriptEngineClient::getArgs(const std::string& path, const std::string& args)
{
	std::vector<std::string> argv;
	if(!path.empty() && path.back() != '/') argv.push_back(path.substr(path.find_last_of('/') + 1));
	else argv.push_back(path);
	wordexp_t p;
	if(wordexp(args.c_str(), &p, 0) == 0)
	{
		for (size_t i = 0; i < p.we_wordc; i++)
		{
			argv.push_back(std::string(p.we_wordv[i]));
		}
		wordfree(&p);
	}
	return argv;
}

void ScriptEngineClient::sendScriptFinished(int32_t exitCode)
{
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		std::string methodName("scriptFinished");
		BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(exitCode))});
		sendRequest(globals->id, methodName, parameters);
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

void ScriptEngineClient::registerClient()
{
	try
	{
		std::string methodName("registerScriptEngineClient");
		BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable((int32_t)getpid()))});
		BaseLib::PVariable result = sendGlobalRequest(methodName, parameters);
		if(result->errorStruct)
		{
			_out.printCritical("Critical: Could not register client.");
			dispose(false);
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
		if(_disposing) return;
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry) return;

		if(queueEntry->isRequest)
		{
			std::string methodName;
			BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

			if(parameters->size() < 2)
			{
				_out.printError("Error: Wrong parameter count while calling method " + methodName);
				return;
			}
			std::map<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
			if(localMethodIterator == _localRpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + methodName);
				BaseLib::PVariable error = BaseLib::Variable::createError(-32601, ": Requested method not found.");
				sendResponse(parameters->at(0), error);
				return;
			}

			if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Server is calling RPC method: " + methodName);

			BaseLib::PVariable result = localMethodIterator->second(parameters->at(1)->arrayValue);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print(true, false);
			}
			sendResponse(parameters->at(0), result);
		}
		else
		{
			BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
			if(response->arrayValue->size() < 3)
			{
				_out.printError("Error: Response has wrong array size.");
				return;
			}
			int32_t scriptId = response->arrayValue->at(0)->integerValue;
			int32_t packetId = response->arrayValue->at(1)->integerValue;

			{
				std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
				auto responseIterator = _rpcResponses[scriptId].find(packetId);
				if(responseIterator != _rpcResponses[scriptId].end())
				{
					PScriptEngineResponse element = responseIterator->second;
					if(element)
					{
						element->response = response;
						element->packetId = packetId;
						element->finished = true;
					}
				}
			}
			if(scriptId != 0)
			{
				std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
				std::map<int32_t, PRequestInfo>::iterator requestIterator = _requestInfo.find(scriptId);
				if(requestIterator != _requestInfo.end()) requestIterator->second->conditionVariable.notify_all();
			}
			else _requestConditionVariable.notify_all();
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

void ScriptEngineClient::sendOutput(std::string& output)
{
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		std::string methodName("scriptOutput");
		BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(output))});
		sendRequest(globals->id, methodName, parameters);
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

void ScriptEngineClient::sendHeaders(BaseLib::PVariable& headers)
{
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		std::string methodName("scriptHeaders");
		BaseLib::PArray parameters(new BaseLib::Array{headers});
		sendRequest(globals->id, methodName, parameters);
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

BaseLib::PVariable ScriptEngineClient::callMethod(std::string& methodName, BaseLib::PVariable& parameters)
{
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		return sendRequest(globals->id, methodName, parameters->arrayValue);
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

BaseLib::PVariable ScriptEngineClient::send(std::vector<char>& data)
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
				return BaseLib::Variable::createError(-32500, "Unknown application error.");
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
    return BaseLib::PVariable(new BaseLib::Variable());
}

BaseLib::PVariable ScriptEngineClient::sendRequest(int32_t scriptId, std::string methodName, BaseLib::PArray& parameters)
{
	try
	{
		PRequestInfo requestInfo;
		{
			std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
			requestInfo = _requestInfo[scriptId];
		}
		int32_t packetId;
		{
			std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
			packetId = _currentPacketId++;
		}
		BaseLib::PArray array(new BaseLib::Array{ BaseLib::PVariable(new BaseLib::Variable(scriptId)), BaseLib::PVariable(new BaseLib::Variable(packetId)), BaseLib::PVariable(new BaseLib::Variable(parameters)) });
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, array, data);

		PScriptEngineResponse response;
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			auto result = _rpcResponses[scriptId].emplace(packetId, std::make_shared<ScriptEngineResponse>());
			if(result.second) response = result.first->second;
		}
		if(!response)
		{
			_out.printCritical("Critical: Could not insert response struct into map.");
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

#ifdef DEBUGSESOCKET
		socketOutput(packetId, true, true, data);
#endif
		BaseLib::PVariable result = send(data);
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[scriptId].erase(packetId);
			return result;
		}

		std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
		while(!requestInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&]{
			return response->finished || _disposing;
		}));

		if(!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId)
		{
			_out.printError("Error: No response received to RPC request. Method: " + methodName);
			result = BaseLib::Variable::createError(-1, "No response received.");
		}
		else result = response->response->arrayValue->at(2);

		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[scriptId].erase(packetId);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::sendGlobalRequest(std::string methodName, BaseLib::PArray& parameters)
{
	try
	{
		std::lock_guard<std::mutex> requestGuard(_requestMutex);
		int32_t packetId;
		{
			std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
			packetId = _currentPacketId++;
		}
		BaseLib::PArray array(new BaseLib::Array{ BaseLib::PVariable(new BaseLib::Variable(0)), BaseLib::PVariable(new BaseLib::Variable(packetId)), BaseLib::PVariable(new BaseLib::Variable(parameters)) });
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, array, data);

		PScriptEngineResponse response;
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			auto result = _rpcResponses[0].emplace(packetId, std::make_shared<ScriptEngineResponse>());
			if(result.second) response = result.first->second;
		}
		if(!response)
		{
			_out.printError("Critical: Could not insert response struct into map.");
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

#ifdef DEBUGSESOCKET
		socketOutput(packetId, true, true, data);
#endif
		BaseLib::PVariable result = send(data);
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[0].erase(packetId);
			return result;
		}

		std::unique_lock<std::mutex> waitLock(_waitMutex);
		while(!_requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&]{
			return response->finished || _disposing;
		}));

		if(!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId)
		{
			_out.printError("Error: No response received to RPC request. Method: " + methodName);
			result = BaseLib::Variable::createError(-1, "No response received.");
		}
		else result = response->response->arrayValue->at(2);

		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[0].erase(packetId);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineClient::sendResponse(BaseLib::PVariable& packetId, BaseLib::PVariable& variable)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{ packetId, variable })));
		std::vector<char> data;
		_rpcEncoder->encodeResponse(array, data);
#ifdef DEBUGSESOCKET
		socketOutput(packetId->integerValue, false, false, data);
#endif
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

void ScriptEngineClient::collectGarbage()
{
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();

		std::vector<int32_t> threadsToRemove;
		std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
		try
		{
			for(std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
			{
				if(!i->second->running)
				{
					if(i->second->thread.joinable()) i->second->thread.join();
					threadsToRemove.push_back(i->first);
				}
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		for(std::vector<int32_t>::iterator i = threadsToRemove.begin(); i != threadsToRemove.end(); ++i)
		{
			try
			{
				{
					std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
					_rpcResponses.erase(*i);
				}
				{
					std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
					_requestInfo.erase(*i);
				}
				_scriptThreads.erase(*i);
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ScriptEngineClient::setThreadNotRunning(int32_t threadId)
{
	if(threadId != -1)
	{
		std::lock_guard<std::mutex> scriptThreadsGuard(_scriptThreadMutex);
		_scriptThreads.at(threadId)->running = false;
	}
}

ScriptEngineClient::ScriptGuard::~ScriptGuard()
{
	if(!_client) return;

	{
		std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
		PhpEvents::eventsMap.erase(_scriptId);
	}

	{
		std::lock_guard<std::mutex> resourceGuard(_resourceMutex);
		if(tsrm_get_ls_cache())
		{
			SG(server_context) = nullptr; //Pointer is invalid - cleaned up already.
			if(SG(request_info).path_translated)
			{
				efree(SG(request_info).path_translated);
				SG(request_info).path_translated = nullptr;
			}
			if(SG(request_info).query_string)
			{
				efree(SG(request_info).query_string);
				SG(request_info).query_string = nullptr;
			}
			if(SG(request_info).request_uri)
			{
				efree(SG(request_info).request_uri);
				SG(request_info).request_uri = nullptr;
			}
			if(SG(request_info).argv)
			{
				free(SG(request_info).argv);
				SG(request_info).argv = nullptr;
			}
			SG(request_info).argc = 0;

			zend_homegear_globals* globals = php_homegear_get_globals();
			if(globals && globals->executionStarted)
			{
				php_request_shutdown(NULL);

				ts_free_thread();
			}
		}
	}
	if(!GD::bl->shuttingDown) _client->sendScriptFinished(_scriptInfo->exitCode);
	if(_scriptInfo->peerId > 0) GD::out.printInfo("Info: PHP script of peer " + std::to_string(_scriptInfo->peerId) + " exited with code " + std::to_string(_scriptInfo->exitCode) + ".");
	else GD::out.printInfo("Info: Script " + std::to_string(_scriptInfo->id) + " exited with code " + std::to_string(_scriptInfo->exitCode) + ".");
	_client->setThreadNotRunning(_scriptId);
}

void ScriptEngineClient::runScript(int32_t id, PScriptInfo scriptInfo)
{
	try
	{
		BaseLib::Rpc::PServerInfo serverInfo(new BaseLib::Rpc::ServerInfo::Info());
		zend_file_handle zendHandle;

		{
			std::lock_guard<std::mutex> resourceGuard(_resourceMutex);
			ts_resource(0); //Replaces TSRMLS_FETCH()

			ScriptInfo::ScriptType type = scriptInfo->getType();
			if(!scriptInfo->script.empty())
			{
				zendHandle.type = ZEND_HANDLE_MAPPED;
				zendHandle.handle.fp = nullptr;
				zendHandle.handle.stream.handle = nullptr;
				zendHandle.handle.stream.closer = nullptr;
				zendHandle.handle.stream.mmap.buf = (char*)scriptInfo->script.c_str(); //String is not modified
				zendHandle.handle.stream.mmap.len = scriptInfo->script.size();
				zendHandle.filename = scriptInfo->fullPath.c_str();
				zendHandle.opened_path = nullptr;
				zendHandle.free_filename = 0;
			}
			else
			{
				zendHandle.type = ZEND_HANDLE_FILENAME;
				zendHandle.filename = scriptInfo->fullPath.c_str();
				zendHandle.opened_path = NULL;
				zendHandle.free_filename = 0;
			}

			zend_homegear_globals* globals = php_homegear_get_globals();
			if(!globals) return;

			if(type == ScriptInfo::ScriptType::web)
			{
				globals->webRequest = true;
				globals->commandLine = false;
				globals->cookiesParsed = !scriptInfo->script.empty();
				globals->http = scriptInfo->http;
				serverInfo = scriptInfo->serverInfo;
			}
			else
			{
				globals->commandLine = true;
				globals->cookiesParsed = true;
				globals->peerId = scriptInfo->peerId;
			}

			ZEND_TSRMLS_CACHE_UPDATE();

			if(type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device)
			{
				BaseLib::Base64::encode(BaseLib::HelperFunctions::getRandomBytes(16), globals->token);
				std::shared_ptr<PhpEvents> phpEvents = std::make_shared<PhpEvents>(globals->token, globals->outputCallback, globals->rpcCallback);
				phpEvents->setPeerId(scriptInfo->peerId);
				std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
				PhpEvents::eventsMap.emplace(id, phpEvents);
			}

			SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise php_homegear_activate is not called.
			SG(default_mimetype) = nullptr;
			SG(default_charset) = nullptr;
			if(type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device)
			{
				PG(register_argc_argv) = 1;
				PG(implicit_flush) = 1;
				PG(html_errors) = 0;
				SG(options) |= SAPI_OPTION_NO_CHDIR;
				SG(headers_sent) = 1;
				SG(request_info).no_headers = 1;
				SG(request_info).path_translated = estrndup(scriptInfo->fullPath.c_str(), scriptInfo->fullPath.size());
			}
			else if(type == ScriptInfo::ScriptType::web)
			{
				SG(sapi_headers).http_response_code = 200;
				SG(request_info).content_length = globals->http.getHeader().contentLength;
				if(!globals->http.getHeader().contentTypeFull.empty()) SG(request_info).content_type = globals->http.getHeader().contentTypeFull.c_str();
				SG(request_info).request_method = globals->http.getHeader().method.c_str();
				SG(request_info).proto_num = globals->http.getHeader().protocol == BaseLib::Http::Protocol::http10 ? 1000 : 1001;
				std::string uri = globals->http.getHeader().path + globals->http.getHeader().pathInfo;
				if(!globals->http.getHeader().args.empty()) uri.append('?' + globals->http.getHeader().args);
				if(!globals->http.getHeader().args.empty()) SG(request_info).query_string = estrndup(&globals->http.getHeader().args.at(0), globals->http.getHeader().args.size());
				if(!uri.empty()) SG(request_info).request_uri = estrndup(&uri.at(0), uri.size());
				std::string pathTranslated = serverInfo->contentPath.substr(0, serverInfo->contentPath.size() - 1) + scriptInfo->relativePath;
				SG(request_info).path_translated = estrndup(&pathTranslated.at(0), pathTranslated.size());
			}

			if (php_request_startup() == FAILURE) {
				GD::bl->out.printError("Error calling php_request_startup...");
				return;
			}
			globals->executionStarted = true;

			if(type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device)
			{
				std::vector<std::string> argv = getArgs(scriptInfo->fullPath, scriptInfo->arguments);
				if(type == ScriptInfo::ScriptType::device) argv[0] = std::to_string(scriptInfo->peerId);
				php_homegear_build_argv(argv);
				SG(request_info).argc = argv.size();
				SG(request_info).argv = (char**)malloc((argv.size() + 1) * sizeof(char*));
				for(uint32_t i = 0; i < argv.size(); ++i)
				{
					SG(request_info).argv[i] = (char*)argv[i].c_str(); //Value is not modified.
				}
				SG(request_info).argv[argv.size()] = nullptr;
			}

			if(scriptInfo->peerId > 0) GD::out.printInfo("Info: Starting PHP script of peer " + std::to_string(scriptInfo->peerId) + ".");
		}

		php_execute_script(&zendHandle);
		scriptInfo->exitCode = EG(exit_status);

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
    std::string error("Error executing script. Check Homegear log for more details.");
    sendOutput(error);
}

void ScriptEngineClient::scriptThread(int32_t id, PScriptInfo scriptInfo, bool sendOutput)
{
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals) exit(1);
		globals->id = id;
		globals->scriptInfo = scriptInfo;
		if(sendOutput)
		{
			globals->outputCallback = std::bind(&ScriptEngineClient::sendOutput, this, std::placeholders::_1);
			globals->sendHeadersCallback = std::bind(&ScriptEngineClient::sendHeaders, this, std::placeholders::_1);
		}
		globals->rpcCallback = std::bind(&ScriptEngineClient::callMethod, this, std::placeholders::_1, std::placeholders::_2);
		{
			std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
			_requestInfo[id].reset(new RequestInfo());
		}
		ScriptGuard scriptGuard(this, globals, id, scriptInfo);

		if(scriptInfo->script.empty() && scriptInfo->fullPath.size() > 3 && scriptInfo->fullPath.compare(scriptInfo->fullPath.size() - 4, 4, ".hgs") == 0)
		{
			std::map<std::string, std::shared_ptr<CacheInfo>>::iterator scriptIterator = _scriptCache.find(scriptInfo->fullPath);
			if(scriptIterator != _scriptCache.end() && scriptIterator->second->lastModified == BaseLib::Io::getFileLastModifiedTime(scriptInfo->fullPath)) scriptInfo->script = scriptIterator->second->script;
			else
			{
				std::vector<char> data = BaseLib::Io::getBinaryFileContent(scriptInfo->fullPath);
				int32_t pos = -1;
				for(uint32_t i = 0; i < 11 && i < data.size(); i++)
				{
					if(data[i] == ' ')
					{
						pos = (int32_t)i;
						break;
					}
				}
				if(pos == -1)
				{
					GD::bl->out.printError("Error: License module id is missing in encrypted script file \"" + scriptInfo->fullPath + "\"");
					return;
				}
				std::string moduleIdString(&data.at(0), pos);
				int32_t moduleId = BaseLib::Math::getNumber(moduleIdString);
				std::vector<char> input(&data.at(pos + 1), &data.at(data.size() - 1) + 1);
				if(input.empty()) return;
				std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
				if(i == GD::licensingModules.end() || !i->second)
				{
					GD::out.printError("Error: Could not decrypt script file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
					return;
				}
				std::shared_ptr<CacheInfo> cacheInfo(new CacheInfo());
				i->second->decryptScript(input, cacheInfo->script);
				cacheInfo->lastModified = BaseLib::Io::getFileLastModifiedTime(scriptInfo->fullPath);
				if(!cacheInfo->script.empty())
				{
					_scriptCache[scriptInfo->fullPath] = cacheInfo;
					scriptInfo->script = cacheInfo->script;
				}
			}
		}

		runScript(id, scriptInfo);
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

// {{{ RPC methods
BaseLib::PVariable ScriptEngineClient::reload(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");

		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.log").c_str(), "a", stdout))
		{
			GD::out.printError("Error: Could not redirect output to new log file.");
		}
		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.err").c_str(), "a", stderr))
		{
			GD::out.printError("Error: Could not redirect errors to new log file.");
		}

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::shutdown(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");

		if(_maintenanceThread.joinable()) _maintenanceThread.join();
		_maintenanceThread = std::thread(&ScriptEngineClient::dispose, this, true);

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::executeScript(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() < 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
		PScriptInfo scriptInfo;
		bool sendOutput = false;
		ScriptInfo::ScriptType type = (ScriptInfo::ScriptType)parameters->at(1)->integerValue;
		if(type == ScriptInfo::ScriptType::cli)
		{
			if(parameters->at(2)->stringValue.empty() ) return BaseLib::Variable::createError(-1, "Path is empty.");
			scriptInfo.reset(new ScriptInfo(type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4)->stringValue, parameters->at(5)->stringValue));

			if(scriptInfo->script.empty() && !GD::bl->io.fileExists(scriptInfo->fullPath))
			{
				_out.printError("Error: PHP script \"" + parameters->at(2)->stringValue + "\" does not exist.");
				return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
			}
			sendOutput = parameters->at(6)->booleanValue;
		}
		else if(type == ScriptInfo::ScriptType::web)
		{
			if(parameters->at(2)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Path is empty.");
			scriptInfo.reset(new ScriptInfo(type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4), parameters->at(5)));

			if(scriptInfo->script.empty() && !GD::bl->io.fileExists(scriptInfo->fullPath))
			{
				_out.printError("Error: PHP script \"" + parameters->at(2)->stringValue + "\" does not exist.");
				return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
			}
			sendOutput = true;
		}
		else if(type == ScriptInfo::ScriptType::device)
		{
			scriptInfo.reset(new ScriptInfo(type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4)->stringValue, parameters->at(5)->stringValue, parameters->at(6)->integerValue64));
		}

		scriptInfo->id = parameters->at(0)->integerValue;

		{
			std::lock_guard<std::mutex> scriptGuard(_scriptThreadMutex);
			if(_scriptThreads.find(scriptInfo->id) == _scriptThreads.end())
			{
				PThreadInfo threadInfo = std::make_shared<ThreadInfo>();
				threadInfo->filename = scriptInfo->fullPath;
				threadInfo->peerId = scriptInfo->peerId;
				threadInfo->thread = std::thread(&ScriptEngineClient::scriptThread, this, scriptInfo->id, scriptInfo, sendOutput);
				_scriptThreads.emplace(scriptInfo->id, threadInfo);
			}
			else _out.printError("Error: Tried to execute script with ID of already running script.");
		}
		collectGarbage();

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::scriptCount(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return std::make_shared<BaseLib::Variable>(0);

		collectGarbage();

		return std::make_shared<BaseLib::Variable>((int32_t)_scriptThreads.size());
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

BaseLib::PVariable ScriptEngineClient::getRunningScripts(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::PVariable(new BaseLib::Variable(0));

		BaseLib::PVariable scripts = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
		std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
		for(std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
		{
			if(i->second->running)
			{
				BaseLib::Array data{std::make_shared<BaseLib::Variable>(i->second->peerId), std::make_shared<BaseLib::Variable>(i->first), std::make_shared<BaseLib::Variable>(i->second->filename)};
				scripts->arrayValue->push_back(std::make_shared<BaseLib::Variable>(std::make_shared<BaseLib::Array>(std::move(data))));
			}
		}
		return scripts;
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

BaseLib::PVariable ScriptEngineClient::broadcastEvent(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() != 4) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
		for(std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
		{
			if(i->second && (parameters->at(0)->integerValue64 == 0 || i->second->peerSubscribed(parameters->at(0)->integerValue64)))
			{
				for(uint32_t j = 0; j < parameters->at(2)->arrayValue->size(); j++)
				{
					std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
					eventData->type = "event";
					eventData->id = parameters->at(0)->integerValue64;
					eventData->channel = parameters->at(1)->integerValue;
					eventData->variable = parameters->at(2)->arrayValue->at(j)->stringValue;
					eventData->value = parameters->at(3)->arrayValue->at(j);
					if(!i->second->enqueue(eventData)) _out.printError("Error: Could not queue event as event buffer is full. Dropping it.");
				}
			}
		}

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::broadcastNewDevices(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
		for(std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
		{
			if(i->second)
			{
				std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
				eventData->type = "newDevices";
				eventData->value = parameters->at(0);
				i->second->enqueue(eventData);
			}
		}

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::broadcastDeleteDevices(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
		for(std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
		{
			if(i->second)
			{
				std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
				eventData->type = "deleteDevices";
				eventData->value = parameters->at(0);
				i->second->enqueue(eventData);
			}
		}

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable ScriptEngineClient::broadcastUpdateDevice(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() != 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

		std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
		for(std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
		{
			if(i->second && i->second->peerSubscribed(parameters->at(0)->integerValue64))
			{
				std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
				eventData->type = "updateDevice";
				eventData->id = parameters->at(0)->integerValue64;
				eventData->channel = parameters->at(1)->integerValue;
				eventData->hint = parameters->at(2)->integerValue;
				i->second->enqueue(eventData);
			}
		}

		return BaseLib::PVariable(new BaseLib::Variable());
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
// }}}

}
