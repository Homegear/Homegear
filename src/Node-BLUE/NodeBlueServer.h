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

#ifndef NODEBLUESERVER_H_
#define NODEBLUESERVER_H_

#include "NodeBlueProcess.h"
#include <homegear-base/BaseLib.h>
#include "FlowInfoServer.h"
#include "NodeManager.h"

#include <queue>

namespace Homegear
{

namespace NodeBlue
{

class NodeBlueServer : public BaseLib::IQueue
{
public:
	NodeBlueServer();

	virtual ~NodeBlueServer();

	bool start();

	void stop();

	void restartFlows();

	void homegearShuttingDown();

	void homegearReloading();

	void processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped);

	uint32_t flowCount();

	void broadcastEvent(std::string& source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>>& variables, BaseLib::PArray& values);

	void broadcastNewDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceDescriptions);

	void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);

	void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);

	std::string handleGet(std::string& path, BaseLib::Http& http, std::string& responseEncoding);

	std::string handlePost(std::string& path, BaseLib::Http& http, std::string& responseEncoding);

	void nodeOutput(std::string nodeId, uint32_t index, BaseLib::PVariable message, bool synchronous);

	BaseLib::PVariable executePhpNodeBaseMethod(BaseLib::PArray& parameters);

	BaseLib::PVariable getNodesWithFixedInputs();

	BaseLib::PVariable getNodeVariable(std::string nodeId, std::string topic);

	void setNodeVariable(std::string nodeId, std::string topic, BaseLib::PVariable value);

	void enableNodeEvents();

	void disableNodeEvents();

private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}

		QueueEntry(PNodeBlueClientData clientData, std::vector<char>& packet)
		{
			this->clientData = clientData;
			this->packet = packet;
		}

		QueueEntry(PNodeBlueClientData clientData, std::string methodName, BaseLib::PArray parameters)
		{
			this->clientData = clientData;
			this->methodName = methodName;
			this->parameters = parameters;
		}

		virtual ~QueueEntry() {}

		PNodeBlueClientData clientData;

		// {{{ Request
		std::string methodName;
		BaseLib::PArray parameters;
		// }}}

		// {{{ Response
		std::vector<char> packet;
		// }}}
	};

	BaseLib::Output _out;
	std::string _socketPath;
	std::string _webroot;
	std::atomic_bool _shuttingDown;
	std::atomic_bool _stopServer;
	std::atomic_bool _nodeEventsEnabled;
	std::thread _mainThread;
	std::thread _maintenanceThread;
	int32_t _backlog = 100;
	std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
	std::mutex _processRequestMutex;
	std::mutex _newProcessMutex;
	std::mutex _processMutex;
	std::map<pid_t, PNodeBlueProcess> _processes;
	std::mutex _currentFlowIdMutex;
	int32_t _currentFlowId = 0;
	std::mutex _stateMutex;
	std::map<int32_t, PNodeBlueClientData> _clients;
	int32_t _currentClientId = 0;
	int64_t _lastGarbageCollection = 0;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> _rpcMethods;
	std::map<std::string, std::function<BaseLib::PVariable(PNodeBlueClientData& clientData, BaseLib::PArray& parameters)>> _localRpcMethods;
	std::mutex _packetIdMutex;
	int32_t _currentPacketId = 0;
	std::atomic_bool _flowsRestarting;
	std::mutex _restartFlowsMutex;
	std::mutex _flowsPostMutex;
	std::mutex _flowsFileMutex;
	std::map<std::string, uint32_t> _maxThreadCounts;
	std::vector<NodeManager::PNodeInfo> _nodeInfo;
	std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
	std::unique_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;
	std::mutex _nodeClientIdMapMutex;
	std::map<std::string, int32_t> _nodeClientIdMap;
	std::mutex _flowClientIdMapMutex;
	std::map<std::string, int32_t> _flowClientIdMap;

	std::atomic<int64_t> _lastNodeEvent;
	std::atomic<uint32_t> _nodeEventCounter;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

	// {{{ Debugging / Valgrinding
	pid_t _manualClientCurrentProcessId = 1;
	std::mutex _unconnectedProcessesMutex;
	std::queue<pid_t> _unconnectedProcesses;
	// }}}

	void collectGarbage();

	bool getFileDescriptor(bool deleteOldSocket = false);

	void mainThread();

	void readClient(PNodeBlueClientData& clientData);

	BaseLib::PVariable send(PNodeBlueClientData& clientData, std::vector<char>& data);

	BaseLib::PVariable sendRequest(PNodeBlueClientData& clientData, std::string methodName, BaseLib::PArray& parameters, bool wait);

	void sendResponse(PNodeBlueClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable);

	void sendShutdown();

	bool sendReset();

	void closeClientConnections();

	void closeClientConnection(PNodeBlueClientData client);

	PNodeBlueProcess getFreeProcess(uint32_t maxThreadCount);

	void getMaxThreadCounts();

	bool checkIntegrity(std::string flowsFile);

	void backupFlows();

	void startFlows();

	void stopNodes();

	std::set<std::string> insertSubflows(BaseLib::PVariable& subflowNode, std::unordered_map<std::string, BaseLib::PVariable>& subflowInfos, std::unordered_map<std::string, BaseLib::PVariable>& flowNodes, std::unordered_map<std::string, BaseLib::PVariable>& subflowNodes, std::set<std::string>& flowNodeIds, std::set<std::string>& allNodeIds);

	void startFlow(PFlowInfoServer& flowInfo, std::set<std::string>& nodes);

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);

	// {{{ RPC methods
	BaseLib::PVariable registerFlowsClient(PNodeBlueClientData& clientData, BaseLib::PArray& parameters);

	BaseLib::PVariable executePhpNode(PNodeBlueClientData& clientData, BaseLib::PArray& parameters);

	BaseLib::PVariable executePhpNodeMethod(PNodeBlueClientData& clientData, BaseLib::PArray& parameters);

	BaseLib::PVariable invokeNodeMethod(PNodeBlueClientData& clientData, BaseLib::PArray& parameters);

	BaseLib::PVariable nodeEvent(PNodeBlueClientData& clientData, BaseLib::PArray& parameters);
	// }}}
};

}

}

#endif
