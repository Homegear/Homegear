#ifndef RPCSERVER_H_
#define RPCSERVER_H_

#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <sstream>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <utility>

#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "RPCVariable.h"
#include "RPCMethod.h"
#include "RPCDecoder.h"
#include "RPCEncoder.h"

namespace RPC
{
	class RPCServer {
		public:
			RPCServer();
			virtual ~RPCServer();

			void start();
			void registerMethod(std::string methodName, RPCMethod method);
		protected:
		private:
			struct PacketType
			{
				enum Enum { xmlRequest, xmlResponse, binaryRequest, binaryResponse };
			};
			bool _stopServer = false;
			std::thread _mainThread;
			std::string _port = "2001";
			int32_t _backlog = 10;
			int32_t _serverFileDescriptor = 0;
			int32_t _maxConnections = 100;
			std::mutex _stateMutex;
			std::vector<int32_t> _fileDescriptors;
			std::vector<std::thread> _readThreads;
			std::mutex _sendMutex;
			std::unordered_map<std::string, int32_t> _localClientFileDescriptors;
			std::unordered_map<std::string, RPCMethod> _rpcMethods;
			RPCDecoder _rpcDecoder;
			RPCEncoder _rpcEncoder;

			void createLocalClient(std::string address);
			void removeLocalClient(std::string address);
			std::pair<std::string, std::string> getAddressAndPort(std::string address);
			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void mainThread();
			void readClient(int32_t clientFileDescriptor);
			void sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<char> data, uint32_t dataLength);
			void packetReceived(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength, PacketType::Enum packetType);
			void analyzeBinaryRPC(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength);
			void analyzeBinaryRPCResponse(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength);
			void removeClientFileDescriptor(int32_t clientFileDescriptor);
	};
}
#endif /* RPCSERVER_H_ */
