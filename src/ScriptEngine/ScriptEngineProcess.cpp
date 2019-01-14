/* Copyright 2013-2019 Homegear GmbH
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

#ifndef NO_SCRIPTENGINE

#include "ScriptEngineProcess.h"
#include "../GD/GD.h"

#include <homegear-base/Encoding/GZip.h>

namespace Homegear
{

namespace ScriptEngine
{

ScriptEngineProcess::ScriptEngineProcess(bool nodeProcess)
{
	_nodeProcess = nodeProcess;
	_nodeThreadCount = 0;
	lastExecution = 0;
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
	return (uint32_t) -1;
}

uint32_t ScriptEngineProcess::nodeThreadCount()
{
	return _nodeThreadCount;
}

void ScriptEngineProcess::invokeScriptOutput(int32_t id, std::string& output, bool error)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptInfo>::iterator scriptsIterator = _scripts.find(id);
		if(scriptsIterator != _scripts.end())
		{
			if(scriptsIterator->second->scriptOutputCallback) scriptsIterator->second->scriptOutputCallback(scriptsIterator->second, output, error);
			if(scriptsIterator->second->returnOutput)
			{
				if(scriptsIterator->second->output.size() + output.size() > scriptsIterator->second->output.capacity()) scriptsIterator->second->output.reserve(scriptsIterator->second->output.capacity() + (((output.size() / 1024) + 1) * 1024));
				scriptsIterator->second->output.append(output);
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

void ScriptEngineProcess::invokeScriptHeaders(int32_t id, BaseLib::PVariable& headers)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptInfo>::iterator scriptsIterator = _scripts.find(id);
		if(scriptsIterator != _scripts.end() && scriptsIterator->second->scriptHeadersCallback) scriptsIterator->second->scriptHeadersCallback(scriptsIterator->second, headers);
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

void ScriptEngineProcess::invokeScriptFinished(int32_t exitCode)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		for(std::map<int32_t, PScriptInfo>::iterator i = _scripts.begin(); i != _scripts.end(); ++i)
		{
			if(GD::bl->debugLevel >= 5 || (i->second->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode && i->second->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode))
			{
				GD::out.printInfo("Info: Script with id " + std::to_string(i->first) + " finished with exit code " + std::to_string(exitCode));
			}
			_nodeThreadCount -= i->second->maxThreadCount + 1;
			i->second->finished = true;
			i->second->exitCode = exitCode;
		}
		for(std::map<int32_t, PScriptFinishedInfo>::iterator i = _scriptFinishedInfo.begin(); i != _scriptFinishedInfo.end(); ++i)
		{
			i->second->finished = true;
			i->second->conditionVariable.notify_all();
		}
		for(std::map<int32_t, PScriptInfo>::iterator i = _scripts.begin(); i != _scripts.end(); ++i)
		{
			if(i->second->scriptFinishedCallback) i->second->scriptFinishedCallback(i->second, exitCode);
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

void ScriptEngineProcess::invokeScriptFinished(int32_t id, int32_t exitCode)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptInfo>::iterator scriptsIterator = _scripts.find(id);
		if(scriptsIterator != _scripts.end())
		{
			if(GD::bl->debugLevel >= 5 || (scriptsIterator->second->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode && scriptsIterator->second->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode))
			{
				GD::out.printInfo("Info: Script with id " + std::to_string(id) + " finished with exit code " + std::to_string(exitCode));
			}
			if(scriptsIterator->second->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode && scriptsIterator->second->nodeInfo)
			{
				_nodeThreadCount -= scriptsIterator->second->maxThreadCount + 1;
				if(_unregisterNode) _unregisterNode(scriptsIterator->second->nodeInfo->structValue->at("id")->stringValue);
			}
			else if(scriptsIterator->second->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::device2)
			{
				if(_unregisterDevice) _unregisterDevice(scriptsIterator->second->peerId);
			}
			scriptsIterator->second->exitCode = exitCode;
			scriptsIterator->second->finished = true;
		}
		std::map<int32_t, PScriptFinishedInfo>::iterator scriptFinishedIterator = _scriptFinishedInfo.find(id);
		if(scriptFinishedIterator != _scriptFinishedInfo.end())
		{
			scriptFinishedIterator->second->finished = true;
			scriptFinishedIterator->second->conditionVariable.notify_all();
		}
		if(scriptsIterator != _scripts.end() && scriptsIterator->second->scriptFinishedCallback) scriptsIterator->second->scriptFinishedCallback(scriptsIterator->second, exitCode);
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

PScriptFinishedInfo ScriptEngineProcess::getScriptFinishedInfo(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		std::map<int32_t, PScriptFinishedInfo>::iterator scriptsIterator = _scriptFinishedInfo.find(id);
		if(scriptsIterator != _scriptFinishedInfo.end()) return scriptsIterator->second;
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
	return PScriptFinishedInfo();
}

void ScriptEngineProcess::registerScript(int32_t id, PScriptInfo& scriptInfo)
{
	try
	{
		std::lock_guard<std::mutex> scriptsGuard(_scriptsMutex);
		_scripts[id] = scriptInfo;
		_scriptFinishedInfo[id] = PScriptFinishedInfo(new ScriptFinishedInfo());
		_nodeThreadCount += scriptInfo->maxThreadCount + 1;
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
		_scriptFinishedInfo.erase(id);
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

}

#endif
