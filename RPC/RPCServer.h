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
#include "RPCDecoder.h"
#include "RPCEncoder.h"
#include "XMLRPCDecoder.h"
#include "XMLRPCEncoder.h"

namespace RPC
{
	class RPCServer {
		public:
			class Client {
			public:
				std::thread readThread;
				int32_t fileDescriptor = -1;
				SSL* ssl = nullptr;

				Client() {}
				virtual ~Client() { if(ssl) SSL_free(ssl); };
			};

			struct PacketType
			{
				enum Enum { xmlRequest, xmlResponse, binaryRequest, binaryResponse };
			};

			RPCServer();
			virtual ~RPCServer();

			void start(bool ssl);
			void stop();
			void registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method);
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> getMethods() { return _rpcMethods; }
			uint32_t connectionCount();
		protected:
		private:
			bool _useSSL = false;
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
	};
}
#endif /* RPCSERVER_H_ */
