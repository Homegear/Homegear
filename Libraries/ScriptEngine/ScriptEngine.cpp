/* Copyright 2013-2014 Sathya Laufer
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

int32_t logScriptOutput(const void *output, unsigned int outputLen, void *userData /* Unused */)
{
	if(!output || outputLen == 0) return PH7_OK;
	std::string stringOutput((const char*)output, (const char*)output + outputLen);
	GD::out.printMessage("Script output: " + stringOutput);
    return PH7_OK;
 }

ScriptEngine::ScriptEngine()
{
	try
	{
		if(ph7_init(&_engine) != PH7_OK)
		{
			GD::out.printError("Error while allocating a new PH7 engine instance.");
			ph7_lib_shutdown();
			_engine = nullptr;
			return;
		}
		if(!ph7_lib_is_threadsafe())
		{
			GD::out.printError("Error: PH7 library is not thread safe.");
			ph7_release(_engine);
			_engine = nullptr;
			ph7_lib_shutdown();
		}
		ph7_lib_config(PH7_LIB_CONFIG_THREAD_LEVEL_MULTI);
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
	ph7_release(_engine);
	_engine = nullptr;
	ph7_lib_shutdown();
	clearPrograms();
}

void ScriptEngine::clearPrograms()
{
	_executeMutex.lock();
	_programsMutex.lock();
	for(std::map<std::string, ScriptInfo>::iterator i = _programs.begin(); i != _programs.end(); ++i)
	{
		ph7_vm_release(i->second.compiledProgram);
		i->second.compiledProgram = nullptr;
	}
	_programs.clear();
	_programsMutex.unlock();
	_executeMutex.unlock();
}

ph7_vm* ScriptEngine::addProgram(std::string path)
{
	try
	{
		if(!_engine)
		{
			GD::out.printError("Error: Script engine is nullptr.");
			return nullptr;
		}
		std::string content;
		try
		{
			content = GD::bl->hf.getFileContent(path);
		}
		catch(BaseLib::Exception& ex)
		{
			GD::bl->out.printError("Error in script engine: Script \"" + path + "\" not found.");
		}
		if(content.empty())
		{
			GD::out.printError("Error: Script file \"" + path + "\" is empty.");
			return nullptr;
		}
		ph7_vm* compiledProgram = nullptr;
		int32_t result = ph7_compile_v2(_engine, content.c_str(), -1, &compiledProgram, 0);
		if(result != PH7_OK)
		{
			if(result == PH7_COMPILE_ERR)
			{
				const char* errorLog;
				int32_t length;
				ph7_config(_engine, PH7_CONFIG_ERR_LOG,	&errorLog, &length);
				if(length > 0) GD::out.printError("Error compiling script \"" + path + "\": " + std::string(errorLog));
			}
			return nullptr;
		}
		result = ph7_vm_config(compiledProgram, PH7_VM_CONFIG_OUTPUT, logScriptOutput, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error while installing the VM output consumer callback for script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		_programsMutex.lock();
		if(_programs.find(path) != _programs.end())
		{
			_executeMutex.lock(); //Don't release program while executing it
			ph7_vm_release(_programs.at(path).compiledProgram);
			_programs.at(path).compiledProgram = nullptr;
			_programs.erase(path);
			_executeMutex.unlock();
		}
		if(!_disposing)
		{
			_programs[path].compiledProgram = compiledProgram;
			_programs[path].lastModified = GD::bl->hf.getFileLastModifiedTime(path);
		}
		else
		{
			ph7_vm_release(compiledProgram);
			compiledProgram = nullptr;
		}
		_programsMutex.unlock();
		return compiledProgram;
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
	_programsMutex.unlock();
	return nullptr;
}

ph7_vm* ScriptEngine::getProgram(std::string path)
{
	try
	{
		ph7_vm* compiledProgram = nullptr;
		_programsMutex.lock();
		if(!_disposing && _programs.find(path) != _programs.end())
		{
			if(GD::bl->hf.getFileLastModifiedTime(path) > _programs.at(path).lastModified)
			{
				_programsMutex.unlock();
				return addProgram(path);
			}
			else
			{
				compiledProgram = _programs.at(path).compiledProgram;
				_programsMutex.unlock();
			}
		}
		else _programsMutex.unlock();
		return compiledProgram;
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
	_programsMutex.unlock();
	return nullptr;
}

void ScriptEngine::removeProgram(std::string path)
{
	try
	{
		_programsMutex.lock();
		if(_programs.find(path) != _programs.end())
		{
			_executeMutex.lock(); //Don't release program while executing it
			ph7_vm_release(_programs.at(path).compiledProgram);
			_programs.at(path).compiledProgram = nullptr;
			_programs.erase(path);
			_executeMutex.unlock();
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
	_programsMutex.unlock();
}

bool ScriptEngine::isValid(const std::string& path, ph7_vm* compiledProgram)
{
	try
	{
		_programsMutex.lock();
		if(_programs.find(path) != _programs.end())
		{
			if(_programs.at(path).compiledProgram == compiledProgram)
			{
				_programsMutex.unlock();
				return true;
			}
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
	_programsMutex.unlock();
	return false;
}

void ScriptEngine::execute(std::string path)
{
	try
	{
		ph7_vm* compiledProgram = getProgram(path);
		if(!compiledProgram) compiledProgram = addProgram(path);
		if(!compiledProgram) return;
		_executeMutex.lock();
		if(_disposing)
		{
			_executeMutex.unlock();
			return;
		}
		if(!isValid(path, compiledProgram)) //Check if compiledProgram is valid within mutex
		{
			GD::out.printError("Error: Script changed during execution: " + path);
			_executeMutex.unlock();
			return;
		}
		if(ph7_vm_exec(compiledProgram, 0) != PH7_OK)
		{
			GD::out.printError("Error executing script \"" + path + "\".");
		}
		if(ph7_vm_reset(compiledProgram) != PH7_OK)
		{
			GD::out.printError("Error resetting script \"" + path + "\".");
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
	_executeMutex.unlock();
}
