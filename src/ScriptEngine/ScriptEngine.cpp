/* Copyright 2013-2015 Sathya Laufer
*
* Homegear is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Homegear is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
*
* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the
* OpenSSL library under certain conditions as described in each
* individual source file, and distribute linked combinations
* including the two.
* You must obey the GNU General Public License in all respects
* for all of the code used other than OpenSSL.  If you modify
* file(s) with this exception, you may extend this exception to your
* version of the file(s), but you are not obligated to do so.  If you
* do not wish to do so, delete this exception statement from your
* version.  If you delete this exception statement from all source
* files in the program, then also delete it here.
*/

#include "../GD/GD.h"
#include "ScriptEngine.h"
#include "php_sapi.h"

ScriptEngine::ScriptEngine()
{
	php_homegear_init();
}

ScriptEngine::~ScriptEngine()
{
	dispose();
}

void ScriptEngine::dispose()
{
	if(_disposing) return;
	_disposing = true;
	while(_scriptThreads.size() > 0)
	{
		GD::out.printInfo("Info: Waiting for script threads to finish.");
		collectGarbage();
		if(_scriptThreads.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	php_homegear_shutdown();
}

void ScriptEngine::collectGarbage()
{
	try
	{
		std::vector<int32_t> threadsToRemove;
		_scriptThreadMutex.lock();
		try
		{
			for(std::map<int32_t, std::pair<std::thread, bool>>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
			{
				if(!i->second.second)
				{
					i->second.first.join();
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
	_scriptThreadMutex.unlock();
}

bool ScriptEngine::scriptThreadMaxReached()
{
	try
	{
		_scriptThreadMutex.lock();
		if(_scriptThreads.size() >= GD::bl->settings.scriptThreadMax() * 100 / 125)
		{
			_scriptThreadMutex.unlock();
			collectGarbage();
			_scriptThreadMutex.lock();
			if(_scriptThreads.size() >= GD::bl->settings.scriptThreadMax())
			{
				_scriptThreadMutex.unlock();
				GD::out.printError("Error: Your script processing is too slow. More than " + std::to_string(GD::bl->settings.scriptThreadMax()) + " scripts are queued. Not executing script.");
				return true;
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
	_scriptThreadMutex.unlock();
	return false;
}

std::vector<std::string> ScriptEngine::getArgs(const std::string& path, const std::string& args)
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

void ScriptEngine::setThreadNotRunning(int32_t threadId)
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

void ScriptEngine::executeScript(const std::string& script, uint64_t peerId, const std::string& args)
{
	if(_disposing || script.empty()) return;
	std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals)
		{
			ts_free_thread();
			return;
		}

		zend_file_handle zendHandle;
		zendHandle.type = ZEND_HANDLE_MAPPED;
		zendHandle.handle.stream.mmap.buf = (char*)script.c_str(); //String is not modified
		zendHandle.handle.stream.mmap.len = script.size();
		zendHandle.filename = "";
		zendHandle.opened_path = nullptr;
		zendHandle.free_filename = 0;

		globals->output = nullptr;
		globals->commandLine = true;
		globals->cookiesParsed = true;

		if(!tsrm_get_ls_cache() || !((sapi_globals_struct *) (*((void ***) tsrm_get_ls_cache()))[((sapi_globals_id)-1)]))
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists.");
			ts_free_thread();
			return;
		}
		PG(register_argc_argv) = 1;
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise php_homegear_activate is not called.
		SG(options) |= SAPI_OPTION_NO_CHDIR;
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			ts_free_thread();
			return;
		}

		std::vector<std::string> argv = getArgs("", args);
		argv[0] = std::to_string(peerId);
		php_homegear_build_argv(argv);
		SG(request_info).argc = argv.size();
		SG(request_info).argv = (char**)malloc((argv.size() + 1) * sizeof(char*));
		for(uint32_t i = 0; i < argv.size(); ++i)
		{
			SG(request_info).argv[i] = (char*)argv[i].c_str(); //Value is not modified.
		}
		SG(request_info).argv[argv.size()] = nullptr;

		/*zval returnValue;
		zval function;
		zval params[1];

		std::string script2 = "<?php \\Homegear\\Homegear::logLevel(); ?>";
		ZVAL_STRINGL(&function, "eval", sizeof("eval") - 1);
		ZVAL_STRINGL(&params[0], script2.c_str(), script2.size());
		call_user_function(EG(function_table), NULL, &function, &returnValue, 1, params);
		if(Z_TYPE(returnValue) == IS_FALSE)
		{
			GD::bl->out.printError("Script engine (peer id: " + std::to_string(peerId) + "): Script exited with errors.");
		}*/

		php_execute_script(&zendHandle);
		if(EG(exit_status) != 0)
		{
			GD::bl->out.printError("Error: Script engine (peer id: " + std::to_string(peerId) + "): Script exited with exit code " + std::to_string(EG(exit_status)));
		}

		php_request_shutdown(NULL);

		ts_free_thread();

		return;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	ts_free_thread();
}

void ScriptEngine::execute(const std::string path, const std::string arguments, std::shared_ptr<std::vector<char>> output, int32_t* exitCode, bool wait, int32_t threadId)
{
	try
	{
		if(_disposing)
		{
			setThreadNotRunning(threadId);
			return;
		}
		if(path.empty())
		{
			//No thread yet
			return;
		}
		if(!wait)
		{
			collectGarbage();
			if(!scriptThreadMaxReached())
			{
				_scriptThreadMutex.lock();
				try
				{
					int32_t threadId = _currentScriptThreadID++;
					if(threadId == -1) threadId = _currentScriptThreadID++;
					_scriptThreads.insert(std::pair<int32_t, std::pair<std::thread, bool>>(threadId, std::pair<std::thread, bool>(std::thread(&ScriptEngine::execute, this, path, arguments, output, nullptr, true, threadId), true)));
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
			if(exitCode) *exitCode = 0;
			//No thread yet
			return;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		setThreadNotRunning(threadId);
		return;
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		setThreadNotRunning(threadId);
		return;
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		setThreadNotRunning(threadId);
		return;
	}
	if(exitCode) *exitCode = -1;
	if(!GD::bl->io.fileExists(path))
	{
		GD::out.printError("Error: PHP script \"" + path + "\" does not exist.");
		setThreadNotRunning(threadId);
		return;
	}
	std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()
	try
	{
		zend_file_handle script;
		script.type = ZEND_HANDLE_FILENAME;
		script.filename = path.c_str();
		script.opened_path = NULL;
		script.free_filename = 0;

		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals)
		{
			ts_free_thread();
			setThreadNotRunning(threadId);
			return;
		}
		if(output) globals->output = output.get();
		globals->commandLine = true;
		globals->cookiesParsed = true;

		if(!tsrm_get_ls_cache() || !((sapi_globals_struct *) (*((void ***) tsrm_get_ls_cache()))[((sapi_globals_id)-1)]) || !((php_core_globals *) (*((void ***) tsrm_get_ls_cache()))[((core_globals_id)-1)]))
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists.");
			ts_free_thread();
			setThreadNotRunning(threadId);
			return;
		}
		PG(register_argc_argv) = 1;
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise php_homegear_activate is not called.
		SG(options) |= SAPI_OPTION_NO_CHDIR;
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;
		SG(request_info).path_translated = estrndup(path.c_str(), path.size());

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			ts_free_thread();
			setThreadNotRunning(threadId);
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

		php_execute_script(&script);
		if(exitCode) *exitCode = EG(exit_status);

		php_request_shutdown(NULL);
		if(output && output->size() > 0)
		{
			std::string outputString(&output->at(0), output->size());
			if(BaseLib::HelperFunctions::trim(outputString).size() > 0) GD::out.printMessage("Script output:\n" + outputString);
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
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
	ts_free_thread();
	setThreadNotRunning(threadId);
}

int32_t ScriptEngine::executeWebRequest(const std::string& path, BaseLib::HTTP& request, std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo, std::shared_ptr<BaseLib::SocketOperations>& socket)
{
	if(_disposing) return 1;
	if(!GD::bl->io.fileExists(path))
	{
		GD::out.printError("Error: PHP script \"" + path + "\" does not exist.");
		return -1;
	}
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()
	try
	{
		zend_file_handle script;

		script.type = ZEND_HANDLE_FILENAME;
		script.filename = path.c_str();
		script.opened_path = NULL;
		script.free_filename = 0;

		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals)
		{
			ts_free_thread();
			return -1;
		}
		globals->socket = socket.get();
		globals->http = &request;

		if(!tsrm_get_ls_cache())
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists (1).");
			ts_free_thread();
			return -1;
		}
		if(!((sapi_globals_struct *) (*((void ***) tsrm_get_ls_cache()))[((sapi_globals_id)-1)]))
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists (2).");
			ts_free_thread();
			return -1;
		}
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise POST data is not processed.
		SG(sapi_headers).http_response_code = 200;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;
		SG(request_info).content_length = request.getHeader()->contentLength;
		if(!request.getHeader()->contentType.empty()) SG(request_info).content_type = request.getHeader()->contentType.c_str();
		SG(request_info).request_method = request.getHeader()->method.c_str();
		SG(request_info).proto_num = request.getHeader()->protocol == BaseLib::HTTP::Protocol::http10 ? 1000 : 1001;
		std::string uri = request.getHeader()->path + request.getHeader()->pathInfo;
		if(!request.getHeader()->args.empty()) uri.append('?' + request.getHeader()->args);
		if(!request.getHeader()->args.empty()) SG(request_info).query_string = estrndup(&request.getHeader()->args.at(0), request.getHeader()->args.size());
		if(!uri.empty()) SG(request_info).request_uri = estrndup(&uri.at(0), uri.size());
		std::string pathTranslated = serverInfo->contentPath.substr(0, serverInfo->contentPath.size() - 1) + request.getHeader()->pathInfo;
		SG(request_info).path_translated = estrndup(&pathTranslated.at(0), pathTranslated.size());

		if (php_request_startup() == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			ts_free_thread();
			return 1;
		}

		php_execute_script(&script);

		int32_t exitCode = EG(exit_status);

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
		if(SG(request_info).path_translated)
		{
			efree(SG(request_info).path_translated);
			SG(request_info).path_translated = nullptr;
		}

		php_request_shutdown(NULL);

		ts_free_thread();
		return exitCode;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	std::string error("Error executing script. Check Homegear log for more details.");
	if(socket) php_homegear_write_socket(socket.get(), error.c_str(), error.length());
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
	if(SG(request_info).path_translated)
	{
		efree(SG(request_info).path_translated);
		SG(request_info).path_translated = nullptr;
	}
	ts_free_thread();
	return 1;
}

bool ScriptEngine::checkSessionId(const std::string& sessionId)
{
	if(_disposing) return false;
	std::unique_ptr<BaseLib::HTTP> request(new BaseLib::HTTP());
	std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());
	ts_resource_ex(0, NULL); //Replaces TSRMLS_FETCH()
	try
	{
		zend_homegear_globals* globals = php_homegear_get_globals();
		if(!globals)
		{
			ts_free_thread();
			return false;
		}
		globals->http = request.get();

		if(!tsrm_get_ls_cache() || !((sapi_globals_struct *) (*((void ***) tsrm_get_ls_cache()))[((sapi_globals_id)-1)]))
		{
			GD::out.printCritical("Critical: Error in PHP: No thread safe resource exists.");
			ts_free_thread();
			return false;
		}
		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise POST data is not processed.
		SG(sapi_headers).http_response_code = 200;
		SG(default_mimetype) = nullptr;
		SG(default_charset) = nullptr;
		SG(request_info).content_length = 0;
		SG(request_info).request_method = "GET";
		SG(request_info).proto_num = 1001;
		request->getHeader()->cookie = "PHPSESSID=" + sessionId;

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			ts_free_thread();
			return false;
		}

		zval returnValue;
		zval function;

		ZVAL_STRINGL(&function, "session_start", sizeof("session_start") - 1);
		call_user_function(EG(function_table), NULL, &function, &returnValue, 0, nullptr);

		bool result = false;
		zval* reference = zend_hash_str_find(&EG(symbol_table), "_SESSION", sizeof("_SESSION") - 1);
		if(reference != NULL)
		{
			if(Z_ISREF_P(reference) && Z_RES_P(reference)->ptr && Z_TYPE_P(Z_REFVAL_P(reference)) == IS_ARRAY)
			{
				zval* token = zend_hash_str_find(Z_ARRVAL_P(Z_REFVAL_P(reference)), "authorized", sizeof("authorized") - 1);
				if(token != NULL)
				{
					result = (Z_TYPE_P(token) == IS_TRUE);
				}
			}
        }

		php_request_shutdown(NULL);

		ts_free_thread();
		return result;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	ts_free_thread();
	return false;
}

BaseLib::PVariable ScriptEngine::getAllScripts()
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));
		if(_disposing) return array;

		std::vector<std::string> scripts = GD::bl->io.getFiles(GD::bl->settings.scriptPath(), true);
		for(std::vector<std::string>::iterator i = scripts.begin(); i != scripts.end(); ++i)
		{
			array->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(*i)));
		}

		return array;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
