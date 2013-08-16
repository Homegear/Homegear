#ifndef CLISERVER_H_
#define CLISERVER_H_

class HomeMaticDevice;

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

namespace CLI {

class ClientData
{
public:
	ClientData() {}
	ClientData(int32_t clientFileDescriptor) { fileDescriptor = clientFileDescriptor; }
	virtual ~ClientData() {}

	int32_t fileDescriptor;
};

class Server {
public:
	Server();
	virtual ~Server();

	void start();
	void stop();
private:
	bool _stopServer = false;
	std::thread _mainThread;
	int32_t _backlog = 10;
	int32_t _serverFileDescriptor = 0;
	int32_t _maxConnections = 100;
	std::mutex _stateMutex;
	std::vector<std::shared_ptr<ClientData>> _fileDescriptors;
	std::vector<std::thread> _readThreads;

	void handleCommand(std::string& command, std::shared_ptr<ClientData> clientData);
	void getFileDescriptor();
	int32_t getClientFileDescriptor();
	void removeClientData(int32_t clientFileDescriptor);
	void mainThread();
	void readClient(std::shared_ptr<ClientData> clientData);
};

} /* namespace CLI */
#endif /* CLISERVER_H_ */
