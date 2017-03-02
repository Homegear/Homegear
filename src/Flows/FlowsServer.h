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

#ifndef FLOWSSERVER_H_
#define FLOWSSERVER_H_

#include "FlowsProcess.h"
#include "../RPC/RPCMethod.h"
#include <homegear-base/BaseLib.h>
#include "FlowInfoServer.h"

namespace Flows
{

class FlowsServer : public BaseLib::IQueue
{
public:
	FlowsServer();
	virtual ~FlowsServer();

	bool start();
	void stop();
	void homegearShuttingDown();
	void homegearReloading();
	void processKilled(pid_t pid, int32_t exitCode, int32_t signal, bool coreDumped);
	uint32_t flowCount();
	void broadcastEvent(uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, BaseLib::PArray values);
	void broadcastNewDevices(BaseLib::PVariable deviceDescriptions);
	void broadcastDeleteDevices(BaseLib::PVariable deviceInfo);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint);
	std::string handleGet(std::string& path, BaseLib::Http& http, std::string& responseEncoding);
	std::string handlePost(std::string& path, BaseLib::Http& http, std::string& responseEncoding);
private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(PFlowsClientData clientData, std::vector<char>& packet, bool isRequest) { this->clientData = clientData; this->packet = packet; this->isRequest = isRequest; }
		virtual ~QueueEntry() {}

		PFlowsClientData clientData;
		std::vector<char> packet;
		bool isRequest = false;
	};

	BaseLib::Output _out;
	std::string _socketPath;
	std::string _webroot;
	std::atomic_bool _shuttingDown;
	std::atomic_bool _stopServer;
	std::thread _mainThread;
	int32_t _backlog = 100;
	std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
	std::mutex _newProcessMutex;
	std::mutex _processMutex;
	std::map<pid_t, PFlowsProcess> _processes;
	std::mutex _currentFlowIdMutex;
	int32_t _currentFlowId = 0;
	std::mutex _stateMutex;
	std::map<int32_t, PFlowsClientData> _clients;
	int32_t _currentClientId = 0;
	int64_t _lastGargabeCollection = 0;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::shared_ptr<Rpc::RPCMethod>> _rpcMethods;
	std::map<std::string, std::function<BaseLib::PVariable(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters)>> _localRpcMethods;
	std::mutex _packetIdMutex;
	int32_t _currentPacketId = 0;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

	void collectGarbage();
	bool getFileDescriptor(bool deleteOldSocket = false);
	void mainThread();
	void readClient(PFlowsClientData& clientData);
	BaseLib::PVariable send(PFlowsClientData& clientData, std::vector<char>& data);
	BaseLib::PVariable sendRequest(PFlowsClientData& clientData, std::string methodName, BaseLib::PArray& parameters);
	void sendResponse(PFlowsClientData& clientData, BaseLib::PVariable& scriptId, BaseLib::PVariable& packetId, BaseLib::PVariable& variable);
	void closeClientConnection(PFlowsClientData client);
	PFlowsProcess getFreeProcess();
	void startFlows();
	void executeFlow(PFlowInfoServer& flowInfo);

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);

	void checkSessionIdThread(std::string sessionId, bool* result);

	// {{{ RPC methods
		BaseLib::PVariable registerFlowsClient(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
		BaseLib::PVariable flowFinished(PFlowsClientData& clientData, int32_t scriptId, BaseLib::PArray& parameters);
	// }}}
};

}
#endif
