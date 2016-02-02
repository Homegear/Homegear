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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <map>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include "RpcClient.h"
#include "homegear-base/BaseLib.h"

namespace RPC
{
class Client
{
public:
	struct Hint
	{
		enum Enum { updateHintAll = 0, updateHintLinks = 1 };
	};

	Client();
	virtual ~Client();
	void dispose();
	void init();
	bool lifetick();

	void initServerMethods(std::pair<std::string, std::string> address);
	void broadcastEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<BaseLib::PVariable>> values);
	void systemListMethods(std::pair<std::string, std::string> address);
	void listDevices(std::pair<std::string, std::string> address);
	void broadcastError(int32_t level, std::string message);
	void broadcastNewDevices(BaseLib::PVariable deviceDescriptions);
	void broadcastNewEvent(BaseLib::PVariable eventDescription);
	void broadcastDeleteDevices(BaseLib::PVariable deviceAddresses, BaseLib::PVariable deviceInfo);
	void broadcastDeleteEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, std::string address, Hint::Enum hint);
	void broadcastUpdateEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);
	void sendUnknownDevices(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRpcServer> addServer(std::pair<std::string, std::string> address, std::string path, std::string id);
	std::shared_ptr<RemoteRpcServer> addWebSocketServer(std::shared_ptr<BaseLib::SocketOperations> socket, std::string clientId, std::string address);
	void removeServer(std::pair<std::string, std::string> address);
	void removeServer(int32_t uid);
	std::shared_ptr<RemoteRpcServer> getServer(std::pair<std::string, std::string>);
	BaseLib::PVariable listClientServers(std::string id);
	BaseLib::PVariable clientServerInitialized(std::string id);
	void reset();
private:
	bool _disposing = false;
	std::shared_ptr<RpcClient> _client;
	std::mutex _serversMutex;
	int32_t _serverId = 0;
	std::map<int32_t, std::shared_ptr<RemoteRpcServer>> _servers;
	std::unique_ptr<BaseLib::RPC::JsonEncoder> _jsonEncoder;
	std::mutex _lifetick1Mutex;
	std::pair<int64_t, bool> _lifetick1;

	void collectGarbage();
};

}
#endif
