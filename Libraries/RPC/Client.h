/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
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

#include "RPCClient.h"
#include "../../Modules/Base/BaseLib.h"

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

	void initServerMethods(std::pair<std::string, std::string> address);
	void broadcastEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values);
	void systemListMethods(std::pair<std::string, std::string> address);
	void listDevices(std::pair<std::string, std::string> address);
	void broadcastError(int32_t level, std::string message);
	void broadcastNewDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceDescriptions);
	void broadcastNewEvent(std::shared_ptr<BaseLib::RPC::Variable> eventDescription);
	void broadcastDeleteDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceAddresses, std::shared_ptr<BaseLib::RPC::Variable> deviceInfo);
	void broadcastDeleteEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);
	void broadcastUpdateDevice(uint64_t id, int32_t channel, std::string address, Hint::Enum hint);
	void broadcastUpdateEvent(std::string id, int32_t type, uint64_t peerID, int32_t channel, std::string variable);
	void sendUnknownDevices(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRPCServer> addServer(std::pair<std::string, std::string> address, std::string path, std::string id);
	std::shared_ptr<RemoteRPCServer> addWebSocketServer(std::shared_ptr<BaseLib::FileDescriptor> socketDescriptor, std::shared_ptr<BaseLib::SocketOperations> socket);
	void removeServer(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRPCServer> getServer(std::pair<std::string, std::string>);
	std::shared_ptr<BaseLib::RPC::Variable> listClientServers(std::string id);
	std::shared_ptr<BaseLib::RPC::Variable> clientServerInitialized(std::string id);
	void reset();
private:
	bool _disposing = false;
	std::unique_ptr<RPCClient> _client;
	std::mutex _serversMutex;
	int32_t _serverId = 0;
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> _servers;
	std::mutex _invokeBroadcastThreadsMutex;
	uint32_t _currentInvokeBroadcastThreadID = 0;
	std::map<uint32_t, std::shared_ptr<std::thread>> _invokeBroadcastThreads;

	void removeBroadcastThread(uint32_t threadID);
	void startInvokeBroadcastThread(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
	void invokeBroadcastThread(uint32_t threadID, std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

}
#endif
