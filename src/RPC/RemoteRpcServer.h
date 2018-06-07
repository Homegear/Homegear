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

#ifndef REMOTERPCSERVER_H_
#define REMOTERPCSERVER_H_

#include <homegear-base/BaseLib.h>
#include "Auth.h"
#include "ClientSettings.h"

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <mutex>
#include <map>

namespace Rpc
{

class RpcClient;

class RemoteRpcServer
{
public:
	std::shared_ptr<ClientSettings::Settings> settings;

	int32_t creationTime = 0;
	bool removed = false;
	bool initialized = false;
	bool useSSL = false;
	bool keepAlive = false;
	bool autoConnect = true;
	bool binary = false;
	bool json = false;
	bool webSocket = false;
	bool newFormat = false;
	bool subscribePeers = false;
	bool nodeEvents = false;
	bool reconnectInfinitely = false;
	bool sendNewDevices = true;
	std::string hostname;
	std::pair<std::string, std::string> address;
	std::string path;
	std::string id;
	BaseLib::RpcClientType type = BaseLib::RpcClientType::generic;
	int32_t uid = -1;
	std::shared_ptr<std::set<uint64_t>> knownDevices;
	std::map<std::string, bool> knownMethods;
	std::shared_ptr<BaseLib::TcpSocket> socket;
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	std::mutex sendMutex;
	int32_t lastPacketSent = -1;
	std::set<uint64_t> subscribedPeers;

    BaseLib::PRpcClientInfo& getServerClientInfo() { return _serverClientInfo; }

    RemoteRpcServer(BaseLib::PRpcClientInfo& serverClientInfo);
	RemoteRpcServer(std::shared_ptr<RpcClient>& client, BaseLib::PRpcClientInfo& serverClientInfo);
	virtual ~RemoteRpcServer();

	/**
	 * Queues a method for sending to this event server.
	 *
	 * @param method The method to queue. The first part of the pair is the method name, the second part the parameters.
	 */
	void queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> method);

    /**
     * Invokes a client RPC method.
     * @param methodName
     * @param parameters
     * @return Returns the result of the method call.
     */
    BaseLib::PVariable invoke(std::string& methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters);
private:
	std::shared_ptr<RpcClient> _client;
    BaseLib::PRpcClientInfo _serverClientInfo;
    std::shared_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;
    std::shared_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
    std::shared_ptr<BaseLib::Rpc::XmlrpcEncoder> _xmlRpcEncoder;

	//{{{ Method queue
	static const int32_t _methodBufferSize = 1000;
	int32_t _methodBufferHead = 0;
	int32_t _methodBufferTail = 0;
	std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> _methodBuffer[_methodBufferSize];
	std::mutex _methodProcessingThreadMutex;
	std::thread _methodProcessingThread;
	bool _methodProcessingMessageAvailable = false;
	std::condition_variable _methodProcessingConditionVariable;
	std::atomic_bool _stopMethodProcessingThread;

	std::atomic<uint32_t> _droppedEntries;
	std::atomic<int64_t> _lastQueueFullError;
    //}}}

	void processMethods();
	BaseLib::PVariable invokeClientMethod(std::string& methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters);
};

}

#endif
