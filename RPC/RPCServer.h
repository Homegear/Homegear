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

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

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
			struct PacketType
			{
				enum Enum { xmlRequest, xmlResponse, binaryRequest, binaryResponse };
			};

			RPCServer();
			virtual ~RPCServer();

			void start();
			void stop();
			void registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method);
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> getMethods() { return _rpcMethods; }
		protected:
		private:
			bool _stopServer = false;
			std::thread _mainThread;
			int32_t _backlog = 10;
			int32_t _serverFileDescriptor = 0;
			int32_t _maxConnections = 100;
			std::mutex _stateMutex;
			std::vector<int32_t> _fileDescriptors;
			std::vector<std::thread> _readThreads;
			std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> _rpcMethods;
			RPCDecoder _rpcDecoder;
			RPCEncoder _rpcEncoder;
			XMLRPCDecoder _xmlRpcDecoder;
			XMLRPCEncoder _xmlRpcEncoder;

			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void mainThread();
			void readClient(int32_t clientFileDescriptor);
			void sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<RPCVariable> error, PacketType::Enum packetType);
			void sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> data, bool closeConnection);
			void packetReceived(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType);
			void analyzeRPC(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType);
			void analyzeRPCResponse(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType);
			void removeClientFileDescriptor(int32_t clientFileDescriptor);
			void callMethod(int32_t clientFileDescriptor, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, PacketType::Enum responseType);
			std::string getHttpResponseHeader(uint32_t contentLength);
	};
}
#endif /* RPCSERVER_H_ */
