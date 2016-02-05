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

#ifndef SCRIPTENGINECLIENT_H_
#define SCRIPTENGINECLIENT_H_

#include "php_config_fixes.h"
#include "../RPC/RPCMethod.h"
#include "homegear-base/BaseLib.h"

#include <thread>
#include <mutex>
#include <string>

using namespace BaseLib::ScriptEngine;

typedef struct _zend_homegear_globals zend_homegear_globals;

namespace ScriptEngine
{

class ScriptEngineClient : public BaseLib::IQueue {
public:
	ScriptEngineClient();
	virtual ~ScriptEngineClient();
	void dispose();

	void start();
private:
	struct CacheInfo
	{
		int32_t lastModified;
		std::string script;
	};

	class ScriptGuard
	{
	private:
		ScriptEngineClient* _client = nullptr;
		zend_homegear_globals* _globals = nullptr;
		int32_t _scriptId = 0;
		std::shared_ptr<std::vector<char>> _output;
		std::shared_ptr<int32_t> _exitCode;
	public:
		ScriptGuard(ScriptEngineClient* client, zend_homegear_globals* globals, int32_t scriptId, std::shared_ptr<std::vector<char>>& output, std::shared_ptr<int32_t>& exitCode) : _client(client), _globals(globals), _scriptId(scriptId), _output(output), _exitCode(exitCode) {}
		virtual ~ScriptGuard();
	};

	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(std::vector<uint8_t> packet, bool isRequest) { this->packet = packet; this->isRequest = isRequest; }
		virtual ~QueueEntry() {}

		std::vector<uint8_t> packet;
		bool isRequest = false;
	};

	bool _disposing = false;
	BaseLib::Output _out;
	std::string _socketPath;
	std::shared_ptr<BaseLib::FileDescriptor> _fileDescriptor;
	bool _closed = false;
	std::mutex _sendMutex;
	std::mutex _requestMutex;
	std::mutex _rpcResponsesMutex;
	std::map<int32_t, BaseLib::PVariable> _rpcResponses;
	std::condition_variable _requestConditionVariable;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>> _localRpcMethods;
	std::thread _registerClientThread;
	std::mutex _scriptThreadMutex;
	std::map<int32_t, std::pair<std::thread, bool>> _scriptThreads;
	std::map<std::string, std::shared_ptr<CacheInfo>> _scriptCache;

	std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;

	void collectGarbage();
	std::vector<std::string> getArgs(const std::string& path, const std::string& args);
	void registerClient();
	BaseLib::PVariable sendRequest(int32_t scriptId, std::mutex& requestMutex, std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters);
	BaseLib::PVariable sendGlobalRequest(std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters);
	void sendResponse(BaseLib::PVariable& variable);
	void sendScriptFinished(zend_homegear_globals* globals, int32_t scriptId, std::string& output, int32_t exitCode);
	void setThreadNotRunning(int32_t threadId);
	void stopEventThreads();

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
	void executeScriptThread(int32_t id, std::string path, std::string arguments, bool sendOutput);
	void executeCliScript(int32_t id, std::string& script, std::string& path, std::string& arguments, std::shared_ptr<std::vector<char>>& output, std::shared_ptr<int32_t>& exitCode);

	// {{{ RPC methods
		BaseLib::PVariable executeScript(BaseLib::PArray& parameters);
	// }}}
};

}
#endif
