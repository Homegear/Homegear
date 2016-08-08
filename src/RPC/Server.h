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

#ifndef SERVER_H_
#define SERVER_H_

#include <memory>
#include <string>

#include <homegear-base/BaseLib.h>
#include "RPCServer.h"
#include "RPCMethods.h"

namespace Rpc {
class Server {
public:
	Server();
	virtual ~Server();

	void dispose();
	void registerMethods();
	void start(BaseLib::Rpc::PServerInfo& serverInfo);
	void stop();
	bool lifetick();
	bool isRunning();
	const std::vector<std::shared_ptr<BaseLib::RpcClientInfo>> getClientInfo();
	const std::shared_ptr<RPCServer> getServer();
	const BaseLib::Rpc::PServerInfo getInfo();
	uint32_t connectionCount();
	BaseLib::PVariable callMethod(std::string methodName, BaseLib::PVariable parameters);

	/**
	 * Checks if a client is an addon on all RPC servers.
	 *
	 * @param clientID The id of the client to check.
	 * @return Returns 1 if the client is known and is an addon, 0 if the client is known and no addon and -1 if the client is unknown.
	 */
	static int32_t isAddonClientAll(int32_t clientID);

	/**
	 * Searches for a client on all RPC servers and returns the IP address.
	 *
	 * @param clientID The id of the client to check.
	 * @return Returns the IP address or an empty string if the client is unknown.
	 */
	static std::string getClientIPAll(int32_t clientID);

	int32_t isAddonClient(int32_t clientID);
	std::string getClientIP(int32_t clientID);
	BaseLib::PEventHandler addWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler);
	void removeWebserverEventHandler(BaseLib::PEventHandler eventHandler);
protected:
	std::shared_ptr<RPCServer> _server;
};

} /* namespace Rpc */
#endif /* SERVER_H_ */
