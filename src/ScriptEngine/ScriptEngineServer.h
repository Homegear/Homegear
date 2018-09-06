/* Copyright 2013-2017 Sathya Laufer
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

#ifndef NO_SCRIPTENGINE

#include "ScriptEngineProcess.h"
#include "../../config.h"
#include <homegear-base/BaseLib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

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
	void homegearReloading();
	void processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped);
	void devTestClient();
	uint32_t scriptCount();
	std::vector<std::tuple<int32_t, uint64_t, int32_t, std::string>> getRunningScripts();
	void executeScript(PScriptInfo& scriptInfo, bool wait);
	std::string checkSessionId(const std::string& sessionId);
	BaseLib::PVariable executePhpNodeMethod(BaseLib::PArray& parameters);
	BaseLib::PVariable executeDeviceMethod(BaseLib::PArray& parameters);
	void broadcastEvent(std::string& source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>>& variables, BaseLib::PArray& values);
	void broadcastNewDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceDescriptions);
	void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);

	BaseLib::PVariable getAllScripts();
private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(PScriptEngineClientData clientData, std::vector<char>& packet) { this->clientData = clientData; this->packet = packet; }
		QueueEntry(PScriptEngineClientData clientData, std::string methodName, BaseLib::PArray parameters) { this->clientData = clientData; this->methodName = methodName; this->parameters = parameters; }
		virtual ~QueueEntry() {}

		PScriptEngineClientData clientData;

		// {{{ Request
			std::string methodName;
			BaseLib::PArray parameters;
		// }}}

		// {{{ Response
			std::vector<char> packet;
		// }}}
	};

	struct ClientScriptInfo
	{
		int32_t scriptId = 0;
		BaseLib::PRpcClientInfo clientInfo;
	};
	typedef std::shared_ptr<ClientScriptInfo> PClientScriptInfo;

	BaseLib::Output _out;
#ifdef DEBUGSESOCKET
	std::mutex _socketOutputMutex;
	std::ofstream _socketOutput;
#endif
	std::string _socketPath;
	std::atomic_bool _shuttingDown;
	std::atomic_bool _stopServer;
	std::thread _mainThread;
	int32_t _backlog = 100;
	std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
	std::mutex _newProcessMutex;
	std::mutex _processMutex;
	std::mutex _resourceMutex;
	std::map<pid_t, PScriptEngineProcess> _processes;
	std::mutex _currentScriptIdMutex;
	int32_t _currentScriptId = 0;
	std::mutex _stateMutex;
	std::map<int32_t, PScriptEngineClientData> _clients;
	int32_t _currentClientId = 0;
	int64_t _lastGargabeCollection = 0;
	BaseLib::PRpcClientInfo _scriptEngineClientInfo;
	std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> _rpcMethods;
	std::map<std::string, std::function<BaseLib::PVariable(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters)>> _localRpcMethods;
	std::mutex _executeScriptMutex;
	std::mutex _packetIdMutex;
	int32_t _currentPacketId = 0;
	std::mutex _scriptFinishedThreadMutex;
	std::thread _scriptFinishedThread;
	std::mutex _nodeClientIdMapMutex;
	std::map<std::string, int32_t> _nodeClientIdMap;
    std::mutex _deviceClientIdMapMutex;
    std::map<uint64_t, int32_t> _deviceClientIdMap;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

#ifdef DEBUGSESOCKET
	void socketOutput(int32_t packetId, PScriptEngineClientData& clientData, bool serverRequest, bool request, std::vector<char> data);
#endif

	void collectGarbage();
	bool getFileDescriptor(bool deleteOldSocket = false);
	void mainThread();
	void readClient(PScriptEngineClientData& clientData);
	BaseLib::PVariable send(PScriptEngineClientData& clientData, std::vector<char>& data);
	BaseLib::PVariable sendRequest(PScriptEngineClientData& clientData, std::string methodName, BaseLib::PArray& parameters, bool wait);
	void sendResponse(PScriptEngineClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable);
	void closeClientConnection(PScriptEngineClientData client);
	PScriptEngineProcess getFreeProcess(bool nodeProcess, uint32_t maxThreadCount = 0);
	void unregisterNode(std::string nodeId);
    void unregisterDevice(uint64_t peerId);
	void invokeScriptFinished(PScriptEngineProcess process, int32_t id, int32_t exitCode);
	void invokeScriptFinishedEarly(PScriptInfo scriptInfo, int32_t exitCode);
	void stopDevices();

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);

	// {{{ RPC methods
		BaseLib::PVariable registerScriptEngineClient(PScriptEngineClientData& clientData, BaseLib::PArray& parameters);
		BaseLib::PVariable scriptFinished(PScriptEngineClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		BaseLib::PVariable scriptOutput(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable scriptHeaders(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable peerExists(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);

		BaseLib::PVariable listRpcClients(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable raiseDeleteDevice(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable getFamilySetting(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable setFamilySetting(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		BaseLib::PVariable deleteFamilySetting(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);

		// {{{ MQTT
			BaseLib::PVariable mqttPublish(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		// }}}

		// {{{ User methods
			BaseLib::PVariable auth(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable createUser(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable deleteUser(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable getUserMetadata(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable getUsersGroups(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable setUserMetadata(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable updateUser(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable userExists(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable listUsers(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);

			BaseLib::PVariable createOauthKeys(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable refreshOauthKey(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable verifyOauthKey(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		// }}}

        // {{{ Group methods
            BaseLib::PVariable createGroup(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
            BaseLib::PVariable deleteGroup(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable getGroup(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
            BaseLib::PVariable getGroups(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
            BaseLib::PVariable groupExists(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
            BaseLib::PVariable updateGroup(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
        // }}}

		// {{{ Module methods
			BaseLib::PVariable listModules(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable loadModule(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable unloadModule(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable reloadModule(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		// }}}

		// {{{ Licensing methods
			BaseLib::PVariable checkLicense(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable removeLicense(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable getLicenseStates(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable getTrialStartTime(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		// }}}

		// {{{ Flows
			BaseLib::PVariable nodeEvent(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable nodeOutput(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
			BaseLib::PVariable executePhpNodeBaseMethod(PScriptEngineClientData& clientData, PClientScriptInfo scriptInfo, BaseLib::PArray& parameters);
		// }}}
	// }}}
};

}
#endif
#endif
