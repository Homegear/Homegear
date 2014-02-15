/* Copyright 2013-2014 Sathya Laufer
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
#include "../Threads/Threads.h"
#include "../Libraries/Types/RPCVariable.h"

namespace RPC
{
class Client
{
public:
	struct Hint
	{
		enum Enum { updateHintAll = 0, updateHintLinks = 1 };
	};

	Client() { _servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>()); }
	virtual ~Client();

	void initServerMethods(std::pair<std::string, std::string> address);
	void broadcastEvent(std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> values);
	void systemListMethods(std::pair<std::string, std::string> address);
	void listDevices(std::pair<std::string, std::string> address);
	void broadcastNewDevices(std::shared_ptr<RPCVariable> deviceDescriptions);
	void broadcastDeleteDevices(std::shared_ptr<RPCVariable> deviceAddresses);
	void broadcastUpdateDevice(std::string address, Hint::Enum hint);
	void sendUnknownDevices(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRPCServer> addServer(std::pair<std::string, std::string> address, std::string path, std::string id);
	void removeServer(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRPCServer> getServer(std::pair<std::string, std::string>);
	std::shared_ptr<RPCVariable> listClientServers(std::string id);
	std::shared_ptr<RPCVariable> clientServerInitialized(std::string id);
	void reset();
private:
	RPCClient _client;
	std::mutex _serversMutex;
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> _servers;
};

} /* namespace RPC */
#endif /* CLIENT_H_ */
