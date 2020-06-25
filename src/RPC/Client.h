/* Copyright 2013-2020 Homegear GmbH
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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <map>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include "RpcClient.h"
#include <homegear-base/BaseLib.h>

namespace Homegear
{

namespace Rpc
{


class Client
{
public:
	struct Hint
	{
		enum Enum
		{
			updateHintAll = 0,
			updateHintLinks = 1
		};
	};

	struct EventInfo
	{
		int64_t time = -1;
		int32_t uniqueId = 0;
		uint64_t id = 0;
		int32_t channel = -1;
		std::string name;
		BaseLib::PVariable value;
	};

	Client();

	virtual ~Client();

	void dispose();

	void init();

	bool lifetick();

	void disconnectRega();

	void initServerMethods(std::pair<std::string, std::string> address);

	void broadcastNodeEvent(const std::string& nodeId, const std::string& topic, const BaseLib::PVariable& value);

	void broadcastEvent(const std::string& source, uint64_t id, int32_t channel, const std::string& deviceAddress, const std::shared_ptr<std::vector<std::string>>& valueKeys, const std::shared_ptr<std::vector<BaseLib::PVariable>>& values);

	void systemListMethods(std::pair<std::string, std::string>& address);

	void listDevices(std::pair<std::string, std::string>& address);

	void broadcastError(int32_t level, const std::string& message);

	void broadcastNewDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceDescriptions);

	void broadcastNewEvent(BaseLib::PVariable eventDescription);

	void broadcastDeleteDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceAddresses, BaseLib::PVariable deviceInfo);

	void broadcastDeleteEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);

	void broadcastUpdateDevice(uint64_t id, int32_t channel, std::string address, Hint::Enum hint);

	void broadcastUpdateEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);

    void broadcastVariableProfileStateChanged(uint64_t profileId, bool state);

    void broadcastRequestUiRefresh(const std::string& id);

	void broadcastPtyOutput(std::string& output);

	void sendUnknownDevices(std::pair<std::string, std::string>& address);

	void sendError(const std::pair<std::string, std::string>& address, int32_t level, const std::string& message);

	std::shared_ptr<RemoteRpcServer> addServer(std::pair<std::string, std::string> address, BaseLib::PRpcClientInfo clientInfo, std::string path, std::string id);

	std::shared_ptr<RemoteRpcServer> addSingleConnectionServer(std::pair<std::string, std::string> address, BaseLib::PRpcClientInfo clientInfo, std::string id);

	std::shared_ptr<RemoteRpcServer> addWebSocketServer(std::shared_ptr<BaseLib::TcpSocket> socket, std::string clientId, BaseLib::PRpcClientInfo clientInfo, std::string address, bool nodeEvents);

	void removeServer(std::pair<std::string, std::string> address);

	void removeServer(int32_t uid);

	std::shared_ptr<RemoteRpcServer> getServer(std::pair<std::string, std::string>);

	BaseLib::PVariable listClientServers(std::string id);

	BaseLib::PVariable clientServerInitialized(std::string id);

	void reset();

	BaseLib::PVariable getLastEvents(std::set<uint64_t> ids, uint32_t timespan);

	BaseLib::PVariable getNodeEvents();

private:
	bool _disposing = false;
	std::shared_ptr<RpcClient> _client;
	std::mutex _serversMutex;
	int32_t _serverId = 0;
	std::map<int32_t, std::shared_ptr<RemoteRpcServer>> _servers;
	std::mutex _nodeClientsMutex;
	std::set<int32_t> _nodeClients;
	std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
	std::mutex _lifetick1Mutex;
	std::pair<int64_t, bool> _lifetick1;
	std::mutex _eventBufferMutex;
	int32_t _eventBufferPosition = 0;
	std::atomic_int _uniqueEventId;
	std::array<EventInfo, 1024> _eventBuffer;
	int64_t _lastGarbageCollection = 0;
	std::mutex _nodeEventCacheMutex;
	std::unordered_map<std::string, std::unordered_map<std::string, BaseLib::PVariable>> _nodeEventCache;

	void collectGarbage();

	std::string getIPAddress(std::string address);
};

}

}

#endif
