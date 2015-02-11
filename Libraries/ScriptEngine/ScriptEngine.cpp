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

#ifdef SCRIPTENGINE
#include "ScriptEngine.h"
#include "php_sapi.h"
#include "../GD/GD.h"

ScriptEngine::ScriptEngine()
{
	try
	{
		php_homegear_init();
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
}

ScriptEngine::~ScriptEngine()
{
	dispose();
}

void ScriptEngine::dispose()
{
	if(_disposing) return;
	_disposing = true;
	php_homegear_shutdown();
	GD::out.printInfo("Info: Waiting for script threads to finish.");
	while(_scriptThreads.size() > 0)
	{
		collectGarbage();
		if(_scriptThreads.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void ScriptEngine::collectGarbage()
{
	try
	{
		std::vector<int32_t> threadsToRemove;
		_scriptThreadMutex.lock();
		try
		{
			for(std::map<int32_t, std::future<int32_t>>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
			{
				if(i->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) threadsToRemove.push_back(i->first);
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

int32_t ScriptEngine::execute(const std::string path, const std::string arguments, bool wait)
{
	try
	{
		if(_disposing) return 1;
		if(path.empty()) return -1;
		if(!wait)
		{
			collectGarbage();
			if(!scriptThreadMaxReached())
			{
				_scriptThreadMutex.lock();
				try
				{
					_scriptThreads.insert(std::pair<int32_t, std::future<int32_t>>(_currentScriptThreadID++, std::async(std::launch::async, &ScriptEngine::execute, this, path, arguments, false)));
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
			return 0;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		return -1;
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		return -1;
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		return -1;
	}
	TSRMLS_FETCH();
	try
	{
		zend_file_handle script;

		/* Set up a File Handle structure */
		script.type = ZEND_HANDLE_FILENAME;
		script.filename = path.c_str();
		script.opened_path = NULL;
		script.free_filename = 0;

		std::vector<char> output;
		php_homegear_get_globals(TSRMLS_C)->output = &output;
		php_homegear_get_globals(TSRMLS_C)->commandLine = true;

		PG(register_argc_argv) = 1;
		SG(options) |= SAPI_OPTION_NO_CHDIR;
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
		SG(request_info).path_translated = estrndup(path.c_str(), path.size());

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			return 1;
		}

		std::vector<std::string> argv = getArgs(path, arguments);
		php_homegear_build_argv(argv TSRMLS_CC);
		SG(request_info).argc = argv.size();
		SG(request_info).argv = (char**)malloc((argv.size() + 1) * sizeof(char*));
		for(uint32_t i = 0; i < argv.size(); ++i)
		{
			SG(request_info).argv[i] = (char*)argv[i].c_str(); //Value is not modified.
		}
		SG(request_info).argv[argv.size()] = nullptr;
		php_startup_auto_globals(TSRMLS_C);

		php_execute_script(&script TSRMLS_CC);
		int32_t exitCode = EG(exit_status);

		php_request_shutdown(NULL);
		if(output.size() > 0)
		{
			std::string outputString(&output.at(0), output.size());
			if(BaseLib::HelperFunctions::trim(outputString).size() > 0) GD::out.printMessage("Script output: " + outputString);
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
	return 1;
}

int32_t ScriptEngine::executeWebRequest(const std::string& path, BaseLib::HTTP& request, std::shared_ptr<RPC::ServerInfo::Info>& serverInfo, std::vector<char>& output)
{
	if(_disposing) return 1;
	TSRMLS_FETCH();
	try
	{
		zend_file_handle script;

		script.type = ZEND_HANDLE_FILENAME;
		script.filename = path.c_str();
		script.opened_path = NULL;
		script.free_filename = 0;

		php_homegear_get_globals(TSRMLS_C)->output = &output;
		php_homegear_get_globals(TSRMLS_C)->http = &request;
		php_homegear_get_globals(TSRMLS_C)->commandLine = false;
		php_homegear_get_globals(TSRMLS_C)->cookiesParsed = false;

		SG(server_context) = (void*)serverInfo.get(); //Must be defined! Otherwise POST data is not processed.
		SG(sapi_headers).http_response_code = 200;
		SG(request_info).content_length = request.getHeader()->contentLength;
		if(!request.getHeader()->contentType.empty()) SG(request_info).content_type = request.getHeader()->contentType.c_str();
		SG(request_info).request_method = request.getHeader()->method.c_str();
		SG(request_info).proto_num = request.getHeader()->protocol == BaseLib::HTTP::Protocol::http10 ? 1000 : 1001;
		std::string uri = request.getHeader()->path + request.getHeader()->pathInfo;
		if(!request.getHeader()->args.empty()) uri.append('?' + request.getHeader()->args);
		if(!request.getHeader()->args.empty()) SG(request_info).query_string = estrndup(&request.getHeader()->args.at(0), request.getHeader()->args.size());
		if(!uri.empty()) SG(request_info).request_uri = estrndup(&uri.at(0), uri.size());
		std::string pathTranslated = serverInfo->contentPath.substr(0, serverInfo->contentPath.size() - 1) + request.getHeader()->pathInfo;
		if(!path.empty()) SG(request_info).path_translated = estrndup(&pathTranslated.at(0), pathTranslated.size());

		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			return 1;
		}

		php_execute_script(&script TSRMLS_CC);
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
	output.insert(output.end(), error.begin(), error.end());
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
	return 1;
}
#endif
