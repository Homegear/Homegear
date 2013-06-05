#ifndef XMLRPCSERVER_H_
#define XMLRPCSERVER_H_

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

namespace RPC
{
	class RPCServer {
		public:
			RPCServer();
			virtual ~RPCServer();

			void start();
		protected:
		private:
			struct packetType
			{
				enum Enum { xml, binary };
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

			void createLocalClient(std::string address);
			void removeLocalClient(std::string address);
			std::pair<std::string, std::string> getAddressAndPort(std::string address);
			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void mainThread();
			void readClient(int32_t clientFileDescriptor);
			void sendRPCResponseToClient(int32_t clientFileDescriptor, std::string data);
			void packetReceived(std::shared_ptr<char> packet, uint32_t packetLength, packetType::Enum packetType);
			void analyzeBinaryRPC(std::shared_ptr<char> packet, uint32_t packetLength);
			int32_t getInteger(char* packet, uint32_t packetLength, uint32_t* position);
			std::string getString(char* packet, uint32_t packetLength, uint32_t* position);
			RPCVariable getParameter(char* packet, uint32_t packetLength, uint32_t* position);
			RPCVariableType getVariableType(char* packet, uint32_t packetLength, uint32_t* position);
			void removeClientFileDescriptor(int32_t clientFileDescriptor);
	};
}
#endif /* XMLRPCSERVER_H_ */
