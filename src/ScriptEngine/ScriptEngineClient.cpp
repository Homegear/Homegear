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

ScriptEngineClient::ScriptEngineClient() : IQueue(GD::bl.get(), 1000)
{
	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine (" + std::to_string(getpid()) + "): ");

	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));

	_localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>("executeScript", std::bind(&ScriptEngineClient::executeScript, this, std::placeholders::_1)));

	php_homegear_init();
}

ScriptEngineClient::~ScriptEngineClient()
{
	dispose();
}

void ScriptEngineClient::stopEventThreads()
{
	PhpEvents::eventsMapMutex.lock();
	for(std::map<pthread_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i)
	{
		if(i->second) i->second->stop();
	}
	PhpEvents::eventsMapMutex.unlock();
}

void ScriptEngineClient::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		stopQueue(0);
		while(_scriptThreads.size() > 0)
		{
			GD::out.printInfo("Info: Waiting for script threads to finish.");
			stopEventThreads();
			collectGarbage();
			if(_scriptThreads.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
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
				dispose();
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

void ScriptEngineClient::sendScriptFinished(zend_homegear_globals* globals, int32_t scriptId, std::string& output, int32_t exitCode)
{
	try
	{
		_out.printInfo("Info: Script " + std::to_string(scriptId) + " exited with code " + std::to_string(exitCode) + ".");
		std::string methodName("scriptFinished");
		std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>{BaseLib::PVariable(new BaseLib::Variable(scriptId)), BaseLib::PVariable(new BaseLib::Variable(exitCode)), BaseLib::PVariable(new BaseLib::Variable(output))});
		sendRequest(scriptId, globals->requestMutex, methodName, parameters);
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
		std::shared_ptr<std::list<BaseLib::PVariable>> parameters(new std::list<BaseLib::PVariable>{BaseLib::PVariable(new BaseLib::Variable(0)), BaseLib::PVariable(new BaseLib::Variable((int32_t)getpid()))});
		BaseLib::PVariable result = sendGlobalRequest(methodName, parameters);
		if(result->errorStruct)
		{
			_out.printCritical("Critical: Could not register client.");
			dispose();
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

			std::map<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
			if(localMethodIterator == _localRpcMethods.end())
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
			BaseLib::PVariable result = localMethodIterator->second(parameters);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print();
			}
			sendResponse(result);
		}
		else
		{
			BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
			{
				std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
				_rpcResponses[response->arrayValue->at(0)->integerValue] = response->arrayValue->at(1);
			}
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

BaseLib::PVariable ScriptEngineClient::sendRequest(int32_t scriptId, std::mutex& requestMutex, std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	try
	{
		std::unique_lock<std::mutex> requestLock(requestMutex);
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, parameters, data);

		for(int32_t i = 0; i < 5; i++)
		{
			{
				{
					std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
					_rpcResponses[scriptId].reset();
				}
				int32_t totallySentBytes = 0;
				std::lock_guard<std::mutex> sendGuard(_sendMutex);
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

			_requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(5000), [&]{ std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex); return (bool)(_rpcResponses[scriptId]); });
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			if(_rpcResponses[scriptId]) return _rpcResponses[scriptId];
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

BaseLib::PVariable ScriptEngineClient::sendGlobalRequest(std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters)
{
	try
	{
		std::unique_lock<std::mutex> requestLock(_requestMutex);
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, parameters, data);

		for(int32_t i = 0; i < 5; i++)
		{
			{
				{
					std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
					_rpcResponses[0].reset();
				}
				int32_t totallySentBytes = 0;
				std::lock_guard<std::mutex> sendGuard(_sendMutex);
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

			_requestConditionVariable.wait_for(requestLock, std::chrono::milliseconds(5000), [&]{ std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex); return (bool)(_rpcResponses[0]); });
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			if(_rpcResponses[0]) return _rpcResponses[0];
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

		std::lock_guard<std::mutex> sendGuard(_sendMutex);
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

void ScriptEngineClient::collectGarbage()
{
	try
	{
		std::vector<int32_t> threadsToRemove;
		std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
		try
		{
			for(std::map<int32_t, std::pair<std::thread, bool>>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
			{
				if(!i->second.second)
				{
					if(i->second.first.joinable()) i->second.first.join();
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
		_scriptThreadMutex.lock();
		try
		{
			_scriptThreads.at(threadId).second = false;
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		_scriptThreadMutex.unlock();
	}
}

ScriptEngineClient::ScriptGuard::~ScriptGuard()
{
	if(!_client || !_globals) return;
	std::string outputString;
	if(_output && !_output->empty()) outputString = std::move(std::string(&_output->at(0), _output->size()));
	if(BaseLib::HelperFunctions::trim(outputString).size() > 0) GD::out.printMessage("Script output:\n" + outputString);
	_client->sendScriptFinished(_globals, _scriptId, outputString, *_exitCode);
	if(tsrm_get_ls_cache() && (*((void ***)tsrm_get_ls_cache()))[sapi_globals_id - 1])
	{
		if(SG(request_info).path_translated)
		{
			efree(SG(request_info).query_string);
			SG(request_info).query_string = nullptr;
		}
		if(SG(request_info).argv)
		{
			free(SG(request_info).argv);
			SG(request_info).argv = nullptr;
		}
		SG(request_info).argc = 0;
	}
	ts_free_thread();
	PhpEvents::eventsMapMutex.lock();
	PhpEvents::eventsMap.erase(pthread_self());
	PhpEvents::eventsMapMutex.unlock();
	_client->setThreadNotRunning(_scriptId);
}

void ScriptEngineClient::executeCliScript(int32_t id, std::string& script, std::string& path, std::string& arguments, std::shared_ptr<std::vector<char>>& output, std::shared_ptr<int32_t>& exitCode)
{
	std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()

	try
	{
		zend_file_handle zendHandle;
		if(script.empty())
		{
			zendHandle.type = ZEND_HANDLE_FILENAME;
			zendHandle.filename = path.c_str();
			zendHandle.opened_path = NULL;
			zendHandle.free_filename = 0;
		}
		else
		{
			zendHandle.type = ZEND_HANDLE_MAPPED;
			zendHandle.handle.fp = nullptr;
			zendHandle.handle.stream.handle = nullptr;
			zendHandle.handle.stream.closer = nullptr;
			zendHandle.handle.stream.mmap.buf = (char*)script.c_str(); //String is not modified
			zendHandle.handle.stream.mmap.len = script.size();
			zendHandle.filename = path.c_str();
			zendHandle.opened_path = nullptr;
			zendHandle.free_filename = 0;
		}

		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals) exit(1);

		if(output) globals->output = output.get();
		globals->commandLine = true;
		globals->cookiesParsed = true;

		if(!tsrm_get_ls_cache() || !(*((void ***)tsrm_get_ls_cache()))[sapi_globals_id - 1] || !(*((void ***)tsrm_get_ls_cache()))[core_globals_id - 1])
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists.");
			exit(1);
		}

		if(!script.empty())
		{
			PhpEvents::eventsMapMutex.lock();
			PhpEvents::eventsMap.insert(std::pair<pthread_t, std::shared_ptr<PhpEvents>>(pthread_self(), std::shared_ptr<PhpEvents>()));
			PhpEvents::eventsMapMutex.unlock();
		}

		PG(register_argc_argv) = 1;
		PG(implicit_flush) = 1;
		PG(html_errors) = 0;
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise php_homegear_activate is not called.
		SG(options) |= SAPI_OPTION_NO_CHDIR;
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;
		SG(request_info).path_translated = estrndup(path.c_str(), path.size());

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			return;
		}

		std::vector<std::string> argv = getArgs(path, arguments);
		php_homegear_build_argv(argv);
		SG(request_info).argc = argv.size();
		SG(request_info).argv = (char**)malloc((argv.size() + 1) * sizeof(char*));
		for(uint32_t i = 0; i < argv.size(); ++i)
		{
			SG(request_info).argv[i] = (char*)argv[i].c_str(); //Value is not modified.
		}
		SG(request_info).argv[argv.size()] = nullptr;

		php_execute_script(&zendHandle);
		*exitCode = EG(exit_status);

		php_request_shutdown(NULL);
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

void ScriptEngineClient::executeScriptThread(int32_t id, std::string path, std::string arguments, bool sendOutput)
{
	try
	{
		std::string script;
		if(path.size() > 3 && path.compare(path.size() - 4, 4, ".hgs") == 0)
		{
			std::map<std::string, std::shared_ptr<CacheInfo>>::iterator scriptIterator = _scriptCache.find(path);
			if(scriptIterator != _scriptCache.end() && scriptIterator->second->lastModified == BaseLib::Io::getFileLastModifiedTime(path)) script = scriptIterator->second->script;
			else
			{
				std::vector<char> data = BaseLib::Io::getBinaryFileContent(path);
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
					GD::bl->out.printError("Error: License module id is missing in encrypted script file \"" + path + "\"");
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
				cacheInfo->lastModified = BaseLib::Io::getFileLastModifiedTime(path);
				if(!cacheInfo->script.empty())
				{
					_scriptCache[path] = cacheInfo;
					script = cacheInfo->script;
				}
			}
		}

		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals) exit(1);
		std::shared_ptr<std::vector<char>> output;
		if(sendOutput) output.reset(new std::vector<char>());
		std::shared_ptr<int32_t> exitCode(new int32_t(-1));
		ScriptGuard scriptGuard(this, globals, id, output, exitCode);

		executeCliScript(id, script, path, arguments, output, exitCode);
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
BaseLib::PVariable ScriptEngineClient::executeScript(BaseLib::PArray& parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");
		if(parameters->size() != 4) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
		if(parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Path is empty.");

		std::string path = std::move(parameters->at(1)->stringValue);
		if(!GD::bl->io.fileExists(path))
		{
			_out.printError("Error: PHP script \"" + path + "\" does not exist.");
			return BaseLib::Variable::createError(-1, "Script file does not exist.");
		}

		_scriptThreads.insert(std::pair<int32_t, std::pair<std::thread, bool>>(parameters->at(0)->integerValue, std::pair<std::thread, bool>(std::thread(&ScriptEngineClient::executeScriptThread, this, parameters->at(0)->integerValue, path, parameters->at(2)->stringValue, parameters->at(3)->booleanValue), true)));
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
// }}}

}
