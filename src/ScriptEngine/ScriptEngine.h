/* Copyright 2013-2015 Sathya Laufer
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

#ifndef SCRIPTENGINE_H_
#define SCRIPTENGINE_H_

#include "php_config_fixes.h"
#include "homegear-base/BaseLib.h"

#include <mutex>
#include <wordexp.h>

class ScriptEngine
{
public:
	ScriptEngine();
	virtual ~ScriptEngine();
	void stopEventThreads();
	void dispose();

	std::vector<std::string> getArgs(const std::string& path, const std::string& args);
	void executeDeviceScript(const std::string& script, uint64_t peerId, const std::string& args, bool keepAlive = false, int32_t interval = -1);
	int32_t executeWebScript(const std::string& script, BaseLib::HTTP& request, std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo, std::shared_ptr<BaseLib::SocketOperations>& socket);
	void execute(const std::string path, const std::string arguments, std::shared_ptr<std::vector<char>> output = nullptr, int32_t* exitCode = nullptr, bool wait = true);
	int32_t executeWebRequest(const std::string& path, BaseLib::HTTP& request, std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo, std::shared_ptr<BaseLib::SocketOperations>& socket);

	void broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values);
	void broadcastNewDevices(BaseLib::PVariable deviceDescriptions);
	void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);
	bool checkSessionId(const std::string& sessionId);

	BaseLib::PVariable getAllScripts();
protected:
	struct CacheInfo
	{
		int32_t lastModified;
		std::string script;
	};

	bool _disposing = false;
	volatile int32_t _currentScriptThreadID = 0;
	std::map<int32_t, std::pair<std::thread, bool>> _scriptThreads;
	std::mutex _scriptThreadMutex;
	std::map<std::string, std::shared_ptr<CacheInfo>> _scriptCache;

	void collectGarbage();
	bool scriptThreadMaxReached();
	void setThreadNotRunning(int32_t threadId);
	void executeThread(const std::string path, const std::string arguments, std::shared_ptr<std::vector<char>> output = nullptr, int32_t* exitCode = nullptr, int32_t threadId = -1, std::shared_ptr<std::mutex> lockMutex = std::shared_ptr<std::mutex>(), std::shared_ptr<bool> mutexReady = std::shared_ptr<bool>(), std::shared_ptr<std::condition_variable> conditionVariable = std::shared_ptr<std::condition_variable>());
	void executeScriptThread(const std::string script, const std::string path, const std::string arguments, std::shared_ptr<std::vector<char>> output = nullptr, int32_t* exitCode = nullptr, int32_t threadId = -1, std::shared_ptr<std::mutex> lockMutex = std::shared_ptr<std::mutex>(), std::shared_ptr<bool> mutexReady = std::shared_ptr<bool>(), std::shared_ptr<std::condition_variable> conditionVariable = std::shared_ptr<std::condition_variable>());
};
#endif
