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

#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

namespace XMLRPC
{

	class Server {
		public:
			Server();
			virtual ~Server();

			void start();
		protected:
		private:
			bool _stopServer = false;
			std::thread _mainThread;
			std::string _port = "2001";
			int32_t _backlog = 10;
			int32_t _serverFileDescriptor = 0;
			int32_t _maxConnections = 100;
			std::mutex _stateMutex;
			volatile fd_set _fileDescriptors;
			std::vector<std::thread> _readThreads;

			void getFileDescriptor();
			int32_t getClientFileDescriptor();
			void mainThread();
			void readClient(int32_t clientFileDescriptor);
			void sendToClient(int32_t clientFileDescriptor, std::string data);
	};
}
#endif /* XMLRPCSERVER_H_ */
