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

#ifndef SCRIPTENGINEPROCESS_H_
#define SCRIPTENGINEPROCESS_H_

#ifndef NO_SCRIPTENGINE

#include "ScriptEngineClientData.h"
#include <homegear-base/BaseLib.h>

using namespace BaseLib::ScriptEngine;

namespace Homegear
{

namespace ScriptEngine
{

struct ScriptFinishedInfo
{
	bool finished = false;
	std::mutex mutex;
	std::condition_variable conditionVariable;
};
typedef std::shared_ptr<ScriptFinishedInfo> PScriptFinishedInfo;

class ScriptEngineProcess
{
private:
	pid_t _pid = 0;
	std::mutex _scriptsMutex;
	std::map<int32_t, PScriptInfo> _scripts;
	std::map<int32_t, PScriptFinishedInfo> _scriptFinishedInfo;
	PScriptEngineClientData _clientData;
	bool _nodeProcess = false;
	bool _exited = false;
	std::atomic_uint _nodeThreadCount;
	std::function<void(std::string)> _unregisterNode;
	std::function<void(uint64_t)> _unregisterDevice;
public:
	ScriptEngineProcess(bool nodeProcess);

	virtual ~ScriptEngineProcess();

	std::atomic<int64_t> lastExecution;
	std::condition_variable requestConditionVariable;

	pid_t getPid() { return _pid; }

	void setPid(pid_t value) { _pid = value; }

	PScriptEngineClientData& getClientData() { return _clientData; }

	void setClientData(PScriptEngineClientData& value) { _clientData = value; }

	void setUnregisterNode(std::function<void(std::string)> value) { _unregisterNode.swap(value); }

	void setUnregisterDevice(std::function<void(uint64_t)> value) { _unregisterDevice.swap(value); }

	bool getExited() { return _exited; }

	void setExited(bool value) { _exited = value; }

	bool isNodeProcess() { return _nodeProcess; }

	void invokeScriptOutput(int32_t id, std::string& output, bool error);

	void invokeScriptHeaders(int32_t id, BaseLib::PVariable& headers);

	void invokeScriptFinished(int32_t exitCode);

	void invokeScriptFinished(int32_t id, int32_t exitCode);

	uint32_t scriptCount();

	uint32_t nodeThreadCount();

	BaseLib::ScriptEngine::PScriptInfo getScript(int32_t id);

	PScriptFinishedInfo getScriptFinishedInfo(int32_t id);

	void registerScript(int32_t id, PScriptInfo& scriptInfo);

	void unregisterScript(int32_t id);
};

typedef std::shared_ptr<ScriptEngineProcess> PScriptEngineProcess;

}

}

#endif
#endif
