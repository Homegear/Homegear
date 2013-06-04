#include "Server.h"

#include "../GD.h"

using namespace XMLRPC;

Server::Server()
{
}

Server::~Server()
{
	_stopServer = true;
	if(_mainThread.joinable()) _mainThread.join();
	for(std::vector<std::thread>::iterator i = _readThreads.begin(); i != _readThreads.end(); ++i)
	{
		if(i->joinable()) i->join();
	}
	if(GD::debugLevel >= 4) cout << "XML RPC Server successfully shut down." << endl;
}

void Server::start()
{

	_mainThread = std::thread(&Server::mainThread, this);
	_mainThread.detach();
}

void Server::mainThread()
{
	getFileDescriptor();
	FD_ZERO(&_fileDescriptors);
	int32_t clientFileDescriptor;
	while(!_stopServer)
	{
		clientFileDescriptor = getClientFileDescriptor();
		if(clientFileDescriptor < 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		if(clientFileDescriptor > _maxConnections)
		{
			if(GD::debugLevel >= 2) cout << "Error: Client connection rejected, because there are too many clients connected to me." << endl;
			close(clientFileDescriptor);
			continue;
		}
		_stateMutex.lock();
		FD_SET(clientFileDescriptor, &_fileDescriptors);
		_stateMutex.unlock();

		_readThreads.push_back(std::thread(&Server::readClient, this, clientFileDescriptor));
		_readThreads.back().detach();
	}
}

void Server::sendToClient(int32_t clientFileDescriptor, std::string data)
{
	int32_t ret = send(clientFileDescriptor, data.c_str(), strlen(data.c_str()),0);
    if(ret != (signed)data.size() && GD::debugLevel >= 3) cout << "Warning: Error sending data to client." << endl;
}

void Server::readClient(int32_t clientFileDescriptor)
{
	char buffer[2048];
	int32_t bytesRead;
	while(!_stopServer)
	{
		bytesRead = read(clientFileDescriptor, buffer, sizeof(buffer));
		if(bytesRead <= 0)
		{
			_stateMutex.lock();
			FD_CLR(clientFileDescriptor, &_fileDescriptors);
			_stateMutex.unlock();
			close(clientFileDescriptor);
			return;
		}
		cout << "Data received: " << std::string(buffer) << endl;
	}
}

int32_t Server::getClientFileDescriptor()
{
	struct sockaddr_storage clientInfo;
	socklen_t addressSize = sizeof(addressSize);
	int32_t clientFileDescriptor = accept(_serverFileDescriptor, (struct sockaddr *) &clientInfo, &addressSize);
	if(clientFileDescriptor == -1) return -1;
	getpeername(clientFileDescriptor, (struct sockaddr*)&clientInfo, &addressSize);

	uint32_t port;
	char ipString[INET6_ADDRSTRLEN];
	if (clientInfo.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&clientInfo;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipString, sizeof ipString);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clientInfo;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipString, sizeof ipString);
	}

	if(GD::debugLevel >= 4) cout << "Connection from " << ipString << " using port " << port << " accepted." << endl;

	return clientFileDescriptor;
}

void Server::getFileDescriptor()
{
	struct addrinfo hostInfo, *serverInfo;

	int32_t fileDescriptor;
	int32_t yes = 1;

	memset(&hostInfo, 0, sizeof(hostInfo));

	hostInfo.ai_family = AF_UNSPEC;
	hostInfo.ai_socktype = SOCK_STREAM;
	hostInfo.ai_flags = AI_PASSIVE;

	getaddrinfo(0, _port.c_str(), &hostInfo, &serverInfo);

	for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next) {
		fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
        if(fileDescriptor == -1) continue;
		if(setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw new Exception("Error: Could not set socket options.");
		if(bind(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) continue;
        break;
    }
	freeaddrinfo(serverInfo);
    if(listen(fileDescriptor, _backlog) == -1) throw new Exception("Error: Server could not start listening.");
    _serverFileDescriptor = fileDescriptor;
}
