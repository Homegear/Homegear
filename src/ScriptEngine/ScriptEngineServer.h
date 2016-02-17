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

#ifndef SCRIPTENGINESERVER_H_
#define SCRIPTENGINESERVER_H_

#include "php_config_fixes.h"
#include "ScriptEngineProcess.h"
#include "../RPC/RPCMethod.h"
#include "homegear-base/BaseLib.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

namespace ScriptEngine
{

class ScriptEngineServer : public BaseLib::IQueue
{
public:
	ScriptEngineServer();
	virtual ~ScriptEngineServer();

	bool start();
	void stop();
	void homegearShuttingDown();
	void processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped);
	uint32_t scriptCount();
	void executeScript(PScriptInfo& scriptInfo, bool wait);
	bool checkSessionId(const std::string& sessionId);
	void broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, BaseLib::PArray values);
	void broadcastNewDevices(BaseLib::PVariable deviceDescriptions);
	void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);

	BaseLib::PVariable getAllScripts();
private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(PScriptEngineClientData clientData, std::vector<char>& packet, bool isRequest) { this->clientData = clientData; this->packet = packet; this->isRequest = isRequest; }
		virtual ~QueueEntry() {}

		PScriptEngineClientData clientData;
		std::vector<char> packet;
		bool isRequest = false;
	};

	BaseLib::Output _out;
	std::string _socketPath;
	bool _shuttingDown = false;
	bool _stopServer = false;
	std::thread _mainThread;
	int32_t _backlog = 100;
	std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
	std::mutex _newProcessMutex;
	std::mutex _processMutex;
	std::map<pid_t, PScriptEngineProcess> _processes;
	std::mutex _currentScriptIdMutex;
	int32_t _currentScriptId = 0;
	std::mutex _stateMutex;
	std::map<int32_t, PScriptEngineClientData> _clients;
	int32_t _currentClientId = 0;
	int64_t _lastGargabeCollection = 0;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::shared_ptr<RPC::RPCMethod>> _rpcMethods;
	std::map<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>> _localRpcMethods;
	std::mutex _packetIdMutex;
	int32_t _currentPacketId = 0;
	std::mutex _checkSessionIdMutex;
	std::thread _checkSessionIdThread;

	std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;

	void collectGarbage();
	bool getFileDescriptor(bool deleteOldSocket = false);
	void mainThread();
	void readClient(PScriptEngineClientData& clientData);
	BaseLib::PVariable send(PScriptEngineClientData& clientData, std::vector<char>& data);
	BaseLib::PVariable sendRequest(PScriptEngineClientData& clientData, std::string methodName, BaseLib::PArray& parameters);
	void sendResponse(PScriptEngineClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable);
	void closeClientConnection(PScriptEngineClientData client);
	PScriptEngineProcess getFreeProcess();

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);

	void checkSessionIdThread(std::string sessionId, bool* result);

	// {{{ RPC methods
		BaseLib::PVariable registerScriptEngineClient(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		BaseLib::PVariable scriptFinished(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		BaseLib::PVariable scriptOutput(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		BaseLib::PVariable peerExists(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);

		BaseLib::PVariable listRpcClients(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);

		// {{{ User methods
			BaseLib::PVariable auth(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable createUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable deleteUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable updateUser(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable userExists(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable listUsers(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		// }}}

		// {{{ Module methods
			BaseLib::PVariable listModules(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable loadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable unloadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable reloadModule(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		// }}}

		// {{{ Licensing methods
			BaseLib::PVariable checkLicense(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable removeLicense(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
			BaseLib::PVariable getLicenseStates(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		// }}}
	// }}}
};

}
#endif
