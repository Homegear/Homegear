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

#ifndef FLOWSCLIENT_H_
#define FLOWSCLIENT_H_

#include "FlowsResponseClient.h"
#include "FlowInfoClient.h"
#include "NodeManager.h"

#include <homegear-node/BinaryRpc.h>
#include <homegear-node/RpcDecoder.h>
#include <homegear-node/RpcEncoder.h>

#include <homegear-base/BaseLib.h>

#include <thread>
#include <mutex>
#include <string>

namespace Flows
{

class FlowsClient : public BaseLib::IQueue
{
public:
	FlowsClient();
	virtual ~FlowsClient();
	void dispose();

	void start();
private:
	struct RequestInfo
	{
		std::mutex waitMutex;
		std::condition_variable conditionVariable;
	};
	typedef std::shared_ptr<RequestInfo> PRequestInfo;

	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(std::string& methodName, Flows::PArray parameters) { this->methodName = methodName; this->parameters = parameters; }
		QueueEntry(std::vector<char>& packet) { this->packet = packet; }
		QueueEntry(Flows::PNodeInfo nodeInfo, uint32_t targetPort, Flows::PVariable message) { this->nodeInfo = nodeInfo; this->targetPort = targetPort; this->message = message; }
		virtual ~QueueEntry() {}

		//{{{ Request
			std::string methodName;
			Flows::PArray parameters;
		//}}}

		//{{{ Response
			std::vector<char> packet;
		//}}}

		//{{{ Node output
			PNodeInfo nodeInfo;
			uint32_t targetPort = 0;
			Flows::PVariable message;
		//}}}
	};

	BaseLib::Output _out;
    std::atomic_int _threadCount;
    std::atomic_int _processingThreadCount1;
    std::atomic_int _processingThreadCount2;
    std::atomic_int _processingThreadCount3;
    std::atomic<int64_t> _processingThreadCountMaxReached1;
    std::atomic<int64_t> _processingThreadCountMaxReached2;
    std::atomic<int64_t> _processingThreadCountMaxReached3;
	std::atomic_bool _startUpComplete;
	std::atomic_bool _shuttingDownOrRestarting;
    std::atomic_bool _shutdownComplete;
	std::atomic_bool _disposed;
	std::string _socketPath;
	std::shared_ptr<BaseLib::FileDescriptor> _fileDescriptor;
	std::atomic_bool _stopped;
	std::mutex _sendMutex;
	std::mutex _rpcResponsesMutex;
	std::map<int64_t, std::map<int32_t, PFlowsResponseClient>> _rpcResponses;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::function<Flows::PVariable(Flows::PArray& parameters)>> _localRpcMethods;
	std::thread _maintenanceThread;
    std::thread _watchdogThread;
	std::mutex _requestInfoMutex;
	std::map<int64_t, PRequestInfo> _requestInfo;
	std::mutex _packetIdMutex;
	int32_t _currentPacketId = 0;
	std::atomic_bool _nodesStopped;
	std::unique_ptr<NodeManager> _nodeManager;
	std::atomic_bool _frontendConnected;
	std::atomic<int64_t> _lastQueueSize;
    std::mutex _eventFlowIdMutex;
    std::string _eventFlowId;

	std::unique_ptr<Flows::BinaryRpc> _binaryRpc;
	std::unique_ptr<Flows::RpcDecoder> _rpcDecoder;
	std::unique_ptr<Flows::RpcEncoder> _rpcEncoder;

	std::mutex _startFlowMutex;

	std::mutex _flowsMutex;
	std::unordered_map<int32_t, PFlowInfoClient> _flows;

	std::mutex _nodesMutex;
	std::unordered_map<std::string, PNodeInfo> _nodes;

	std::mutex _peerSubscriptionsMutex;
	std::unordered_map<uint64_t, std::unordered_map<int32_t, std::unordered_map<std::string, std::set<std::string>>>> _peerSubscriptions;

	std::mutex _internalMessagesMutex;
	std::unordered_map<std::string, Flows::PVariable> _internalMessages;

    void watchdog();

    void resetClient(Flows::PVariable packetId);

	void registerClient();
	Flows::PVariable invoke(std::string methodName, Flows::PArray parameters, bool wait);
	Flows::PVariable invokeNodeMethod(std::string nodeId, std::string methodName, Flows::PArray parameters, bool wait);
	void sendResponse(Flows::PVariable& packetId, Flows::PVariable& variable);

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
	Flows::PVariable send(std::vector<char>& data);

	void log(std::string nodeId, int32_t logLevel, std::string message);
	void subscribePeer(std::string nodeId, uint64_t peerId, int32_t channel, std::string variable);
	void unsubscribePeer(std::string nodeId, uint64_t peerId, int32_t channel, std::string variable);
	void queueOutput(std::string nodeId, uint32_t index, Flows::PVariable message, bool synchronous);
	void nodeEvent(std::string nodeId, std::string topic, Flows::PVariable value);
	Flows::PVariable getNodeData(std::string nodeId, std::string key);
	void setNodeData(std::string nodeId, std::string key, Flows::PVariable value);
	void setInternalMessage(std::string nodeId, Flows::PVariable message);
	Flows::PVariable getConfigParameter(std::string nodeId, std::string name);

	// {{{ RPC methods
		/**
		 * Causes the log files to be reopened.
		 * @param parameters Irrelevant for this method.
		 */
		Flows::PVariable reload(Flows::PArray& parameters);

		/**
		 * Causes the script engine client to exit.
		 * @param parameters Irrelevant for this method.
		 */
		Flows::PVariable shutdown(Flows::PArray& parameters);

		/**
		 * Starts a new flow.
		 * @param parameters
		 */
		Flows::PVariable startFlow(Flows::PArray& parameters);

		/**
		 * Executes the method "start" on all nodes. It is run after all nodes are initialized.
		 * @param parameters
		 */
		Flows::PVariable startNodes(Flows::PArray& parameters);

		/**
		 * Executed when all config nodes are available.
		 * @param parameters
		 */
		Flows::PVariable configNodesStarted(Flows::PArray& parameters);

		/**
		 * Executed when start up is complete. Nodes can output data from within this method.
		 * @param parameters
		 */
		Flows::PVariable startUpComplete(Flows::PArray& parameters);

		/**
		 * Executes the method "stop" on all nodes. RPC methods can still be called within "stop", but not afterwards.
		 * @param parameters
		 */
		Flows::PVariable stopNodes(Flows::PArray& parameters);

		/**
		 * Stops a flow.
		 * @param parameters
		 */
		Flows::PVariable stopFlow(Flows::PArray& parameters);

		/**
		 * Returns the number of flows currently running.
		 * @param parameters Irrelevant for this method.
		 * @return Returns the number of running flows.
		 */
		Flows::PVariable flowCount(Flows::PArray& parameters);

		Flows::PVariable nodeOutput(Flows::PArray& parameters);
		Flows::PVariable invokeExternalNodeMethod(Flows::PArray& parameters);
		Flows::PVariable executePhpNodeBaseMethod(Flows::PArray& parameters);

		Flows::PVariable setNodeVariable(Flows::PArray& parameters);

        Flows::PVariable setFlowVariable(Flows::PArray& parameters);
		Flows::PVariable enableNodeEvents(Flows::PArray& parameters);
		Flows::PVariable disableNodeEvents(Flows::PArray& parameters);

		Flows::PVariable broadcastEvent(Flows::PArray& parameters);
		Flows::PVariable broadcastNewDevices(Flows::PArray& parameters);
		Flows::PVariable broadcastDeleteDevices(Flows::PArray& parameters);
		Flows::PVariable broadcastUpdateDevice(Flows::PArray& parameters);
	// }}}
};

}
#endif
