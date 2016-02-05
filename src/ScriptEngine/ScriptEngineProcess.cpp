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

#include "ScriptEngineProcess.h"
#include "../GD/GD.h"

namespace ScriptEngine
{

ScriptEngineProcess::ScriptEngineProcess()
{

}

ScriptEngineProcess::~ScriptEngineProcess()
{

}

uint32_t ScriptEngineProcess::scriptCount()
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		return _scripts.size();
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
    return (uint32_t)-1;
}

void ScriptEngineProcess::invokeScriptFinished(int32_t exitCode)
{
	try
	{
		std::string output;
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		for(std::map<int32_t, PScriptInfo>::iterator i = _scripts.begin(); i != _scripts.end(); ++i)
		{
			if(i->second->scriptFinishedCallback) i->second->scriptFinishedCallback(i->second, exitCode, output);
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

void ScriptEngineProcess::invokeScriptFinished(int32_t id, int32_t exitCode, std::string& output)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptInfo>::iterator scriptsIterator = _scripts.find(id);
		if(scriptsIterator != _scripts.end() && scriptsIterator->second->scriptFinishedCallback) scriptsIterator->second->scriptFinishedCallback(scriptsIterator->second, exitCode, output);
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

PScriptInfo ScriptEngineProcess::getScript(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptInfo>::iterator scriptsIterator = _scripts.find(id);
		if(scriptsIterator != _scripts.end()) return scriptsIterator->second;
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
    return PScriptInfo();
}

void ScriptEngineProcess::registerScript(int32_t id, PScriptInfo& scriptInfo)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		_scripts[id] = scriptInfo;
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

void ScriptEngineProcess::unregisterScript(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		_scripts.erase(id);
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

}
