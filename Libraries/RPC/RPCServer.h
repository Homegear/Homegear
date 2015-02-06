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

#ifndef RPCSERVER_H_
#define RPCSERVER_H_

#include "../../Modules/Base/BaseLib.h"
#include "RPCMethod.h"
#include "Auth.h"
#include "ServerInfo.h"
#include "Webserver.h"

#include <thread>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <sstream>
#include <mutex>
#include <memory>
#include <map>
#include <unordered_map>
#include <utility>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include <gcrypt.h>
#include <gnutls/gnutls.h>

namespace RPC
{
	class RPCServer {
		public:
			class Client
			{
			public:
				int32_t id = -1;
				bool closed = false;
				bool binaryPacket = false;
				std::thread readThread;
				std::shared_ptr<BaseLib::FileDescriptor> socketDescriptor;
				std::shared_ptr<BaseLib::SocketOperations> socket;
				Auth auth;
				std::string address;
				int32_t port;

				Client();
				virtual ~Client();
			};

			struct PacketType
			{
				enum Enum { xmlRequest, xmlResponse, binaryRequest, binaryResponse };
			};

			RPCServer();
			virtual ~RPCServer();

			const std::shared_ptr<ServerInfo::Info> getInfo() { return _info; }
			bool isRunning() { return !_stopped; }
			void start(std::shared_ptr<ServerInfo::Info>& settings);
			void stop();
			void registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method);
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> getMethods() { return _rpcMethods; }
			uint32_t connectionCount();
			std::shared_ptr<BaseLib::RPC::Variable> callMethod(std::string& methodName, std::shared_ptr<BaseLib::RPC::Variable>& parameters);
		protected:
		private:
			BaseLib::Output _out;
			volatile int32_t _currentClientID = 0;
			std::shared_ptr<ServerInfo::Info> _info;
			gnutls_certificate_credentials_t _x509Cred = nullptr;
			gnutls_priority_t _tlsPriorityCache = nullptr;
			gnutls_dh_params_t _dhParams = nullptr;
			int32_t _threadPolicy = SCHED_OTHER;
			int32_t _threadPriority = 0;
			bool _stopServer = false;
			bool _stopped = true;
			std::thread _mainThread;
			int32_t _backlog = 10;
			int64_t _lastGargabeCollection = 0;
			std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
			std::mutex _stateMutex;
			std::map<int32_t, std::shared_ptr<Client>> _clients;
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> _rpcMethods;
			std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
			std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;
			std::unique_ptr<BaseLib::RPC::XMLRPCDecoder> _xmlRpcDecoder;
			std::unique_ptr<BaseLib::RPC::XMLRPCEncoder> _xmlRpcEncoder;
			std::unique_ptr<WebServer> _webServer;

			void collectGarbage();
			void getSocketDescriptor();
			std::shared_ptr<BaseLib::FileDescriptor> getClientSocketDescriptor(std::string& address, int32_t& port);
			void getSSLSocketDescriptor(std::shared_ptr<Client>);
			void mainThread();
			void readClient(std::shared_ptr<Client> client);
			void sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<BaseLib::RPC::Variable> error, PacketType::Enum packetType, bool keepAlive);
			void sendRPCResponseToClient(std::shared_ptr<Client> client, std::vector<char>& data, bool keepAlive);
			void packetReceived(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive);
			void handleConnectionUpgrade(std::shared_ptr<Client> client, BaseLib::HTTP& http);
			void analyzeRPC(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive);
			void analyzeRPCResponse(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive);
			void callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters, PacketType::Enum responseType, bool keepAlive);
			std::string getHttpResponseHeader(uint32_t contentLength, bool closeConnection);
			std::string getHttpHtmlResponseHeader(uint32_t contentLength, bool closeConnection);
			void closeClientConnection(std::shared_ptr<Client> client);
			bool clientValid(std::shared_ptr<Client>& client);
	};
}
#endif
