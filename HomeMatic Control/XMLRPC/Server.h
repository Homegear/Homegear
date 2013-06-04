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

#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "RPCVariable.h"

namespace XMLRPC
{
	class Server {
		public:
			Server();
			virtual ~Server();

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
			volatile fd_set _fileDescriptors;
			std::vector<std::thread> _readThreads;
			std::mutex _sendMutex;

			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void mainThread();
			void readClient(int32_t clientFileDescriptor);
			void sendToClient(int32_t clientFileDescriptor, std::string data);
			void packetReceived(std::shared_ptr<char> packet, int32_t packetLength, packetType::Enum packetType);
			void analyzeBinaryRPC(std::shared_ptr<char> packet, int32_t packetLength);
			int32_t getInteger(char* packet, int32_t packetLength, int32_t* position);
			std::string getString(char* packet, int32_t packetLength, int32_t* position);
			RPCVariable getParameter(char* packet, int32_t packetLength, int32_t* position);
			RPCVariableType getVariableType(char* packet, int32_t packetLength, int32_t* position);
	};
}
#endif /* XMLRPCSERVER_H_ */
