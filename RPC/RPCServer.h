/* Copyright 2013 Sathya Laufer
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
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "HTTP.h"
#include "RPCVariable.h"
#include "RPCMethod.h"
#include "RPCHeader.h"
#include "RPCDecoder.h"
#include "RPCEncoder.h"
#include "XMLRPCDecoder.h"
#include "XMLRPCEncoder.h"
#include "SocketOperations.h"
#include "Auth.h"
#include "ServerSettings.h"

namespace RPC
{
	class RPCServer {
		public:
			class Client {
			public:
				int32_t id = -1;
				std::thread readThread;
				int32_t fileDescriptor = -1;
				SSL* ssl = nullptr;
				SocketOperations socket;
				Auth auth;
				bool connectionClosed = false;

				Client() {}
				virtual ~Client() { if(ssl) SSL_free(ssl); };
			};

			struct PacketType
			{
				enum Enum { xmlRequest, xmlResponse, binaryRequest, binaryResponse };
			};

			RPCServer();
			virtual ~RPCServer();

			void start(std::shared_ptr<ServerSettings::Settings>& settings);
			void stop();
			void registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method);
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> getMethods() { return _rpcMethods; }
			uint32_t connectionCount();
			std::shared_ptr<RPCVariable> callMethod(std::string& methodName, std::shared_ptr<RPCVariable>& parameters);
		protected:
		private:
			int32_t _currentClientID = 0;
			std::shared_ptr<ServerSettings::Settings> _settings;
			SSL_CTX* _sslCTX = nullptr;
			int32_t _threadPolicy = SCHED_OTHER;
			int32_t _threadPriority = 0;
			bool _stopServer = false;
			std::thread _mainThread;
			int32_t _backlog = 10;
			int32_t _serverFileDescriptor = 0;
			int32_t _maxConnections = 100;
			std::mutex _stateMutex;
			std::map<int32_t, std::shared_ptr<Client>> _clients;
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> _rpcMethods;
			RPCDecoder _rpcDecoder;
			RPCEncoder _rpcEncoder;
			XMLRPCDecoder _xmlRpcDecoder;
			XMLRPCEncoder _xmlRpcEncoder;

			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void getSSLFileDescriptor(std::shared_ptr<Client>);
			void mainThread();
			void readClient(std::shared_ptr<Client> client);
			void sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<RPCVariable> error, PacketType::Enum packetType, bool keepAlive);
			void sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> data, bool keepAlive);
			void packetReceived(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive);
			void analyzeRPC(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive);
			void analyzeRPCResponse(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive);
			void removeClientFileDescriptor(int32_t clientFileDescriptor);
			void callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, PacketType::Enum responseType, bool keepAlive);
			std::string getHttpResponseHeader(uint32_t contentLength);
			void closeClientConnection(std::shared_ptr<Client> client);
			bool clientValid(std::shared_ptr<Client>& client);
	};
}
#endif /* RPCSERVER_H_ */
