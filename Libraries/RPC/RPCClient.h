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

#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include "ClientSettings.h"
#include "../../Modules/Base/BaseLib.h"
#include "Auth.h"

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <list>
#include <mutex>
#include <map>

#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>

namespace RPC
{
class RemoteRPCServer
{
public:
	RemoteRPCServer();
	virtual ~RemoteRPCServer() {}

	std::shared_ptr<ClientSettings::Settings> settings;

	bool removed = false;
	bool initialized = false;
	bool useSSL = false;
	bool keepAlive = false;
	bool binary = false;
	bool useID = false;
	bool subscribePeers = false;
	std::string hostname;
	std::pair<std::string, std::string> address;
	std::string path;
	std::string id;
	std::shared_ptr<std::set<uint64_t>> knownDevices;
	std::map<std::string, bool> knownMethods;
	std::shared_ptr<BaseLib::SocketOperations> socket;
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	std::mutex sendMutex;
	Auth auth;
	int32_t lastPacketSent = -1;
	std::set<uint64_t> subscribedPeers;
};

class RPCClient {
public:
	RPCClient();
	virtual ~RPCClient();

	void invokeBroadcast(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);

	void reset();
protected:
	std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;
	std::unique_ptr<BaseLib::RPC::XMLRPCDecoder> _xmlRpcDecoder;
	std::unique_ptr<BaseLib::RPC::XMLRPCEncoder> _xmlRpcEncoder;
	int32_t _sendCounter = 0;

	void sendRequest(std::shared_ptr<RemoteRPCServer> server, std::vector<char>& data, std::vector<char>& responseData, bool insertHeader, bool& retry);
	std::string getIPAddress(std::string address);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
