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

#include "ScriptEngine.h"
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
}

int32_t ScriptEngine::execute(const std::string& path, const std::string& arguments)
{
	try
	{
		if(_disposing) return 1;
		zend_file_handle script;

		/* Set up a File Handle structure */
		script.type = ZEND_HANDLE_FILENAME;
		script.filename = path.c_str();
		script.opened_path = NULL;
		script.free_filename = 0;

		TSRMLS_FETCH();

		SG(options) |= SAPI_OPTION_NO_CHDIR;
		if (php_request_startup(TSRMLS_C) == FAILURE) {
			GD::bl->out.printError("Error calling php_request_startup...");
			return 1;
		}

		php_execute_script(&script TSRMLS_CC);
		php_request_shutdown(NULL);
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
	return 1;
}

int32_t ScriptEngine::executeWebRequest(const std::string& path, const std::vector<char>& request, std::vector<char>& output, std::vector<std::string>& headers)
{
	try
	{
		if(_disposing) return 1;
		if(request.empty()) return 1;

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
	return 1;
}
