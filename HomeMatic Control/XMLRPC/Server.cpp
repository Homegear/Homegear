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
	try
	{
		_sendMutex.lock();
		int32_t ret = send(clientFileDescriptor, data.c_str(), strlen(data.c_str()),0);
		_sendMutex.unlock();
		if(ret != (signed)data.size() && GD::debugLevel >= 3) cout << "Warning: Error sending data to client." << endl;
	}
    catch(const std::exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(const Exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(...)
    {
    	_sendMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << endl;
    }
}

int32_t Server::getInteger(char* packet, int32_t packetLength, int32_t* position)
{
	if(*position + 4 > packetLength) return 0;
	int32_t integer = (*(packet + *position) << 24) + (*(packet + *position + 1) << 16) + (*(packet + *position + 2) << 8) + *(packet + *position + 3);
	*position += 4;
	return integer;
}

RPCVariableType Server::getVariableType(char* packet, int32_t packetLength, int32_t* position)
{
	return (RPCVariableType)getInteger(packet, packetLength, position);
}

std::string Server::getString(char* packet, int32_t packetLength, int32_t* position)
{
	int32_t stringLength = getInteger(packet, packetLength, position);
	if(*position + stringLength > packetLength) return "";
	char string[stringLength + 1];
	memcpy(string, packet + *position, stringLength);
	string[stringLength] = '\0';
	*position += stringLength;
	return std::string(string);
}

RPCVariable Server::getParameter(char* packet, int32_t packetLength, int32_t* position)
{
	RPCVariableType type = getVariableType(packet, packetLength, position);
	RPCVariable variable(type);
	if(type == RPCVariableType::rpcString)
	{
		variable.stringValue = getString(packet, packetLength, position);
	}
	else if(type == RPCVariableType::rpcInteger)
	{
		variable.integerValue = getInteger(packet, packetLength, position);
	}
	return variable;
}

void Server::analyzeBinaryRPC(std::shared_ptr<char> packet, int32_t packetLength)
{
	int32_t position = 0;
	std::string methodName = getString(packet.get(), packetLength, &position);
	int32_t parameterCount = getInteger(packet.get(), packetLength, &position);
	std::vector<RPCVariable> parameters;
	for(int32_t i = 0; i < parameterCount; i++)
	{
		parameters.push_back(getParameter(packet.get(), packetLength, &position));
	}
	cout << "Method called: " << methodName << " Parameters:";
	for(std::vector<RPCVariable>::iterator i = parameters.begin(); i != parameters.end(); ++i)
	{
		cout << " Type: " << (int32_t)i->type;
		if(i->type == RPCVariableType::rpcString)
		{
			cout << " Value: " << i->stringValue;
		}
		else if(i->type == RPCVariableType::rpcInteger)
		{
			cout << " Value: " << i->integerValue;
		}
	}
	cout << endl;
}

void Server::packetReceived(std::shared_ptr<char> packet, int32_t packetLength, packetType::Enum packetType)
{
	try
	{
		if(packetType == packetType::Enum::binary) analyzeBinaryRPC(packet, packetLength);
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << endl;
    }
}

void Server::readClient(int32_t clientFileDescriptor)
{
	try
	{
		char buffer[1024];
		std::shared_ptr<char> packet(new char[1]);
		int32_t packetLength;
		int32_t bytesRead;
		int32_t dataSize;
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
			if(bytesRead < 8) continue;
			if(buffer[0] == 'B' && buffer[1] == 'i' && buffer[2] == 'n')
			{
				dataSize = (buffer[4] << 24) + (buffer[5] << 16) + (buffer[6] << 8) + buffer[7];
				if(GD::debugLevel >= 6) cout << "Receiving packet with size: " << dataSize << endl;
				if(dataSize == 0) continue;
				if(dataSize > 104857600)
				{
					if(GD::debugLevel >= 2) cout << "Error: Packet with data larger than 100 MiB received." << endl;
					continue;
				}
				packet.reset(new char[dataSize]);
				if(dataSize > bytesRead - 8)
				{
					memcpy(packet.get(), buffer + 8, bytesRead - 8);
					packetLength = bytesRead - 8;
				}
				else
				{
					packetLength = 0;
					memcpy(packet.get(), buffer + 8, bytesRead - 8);
					std::thread t(&Server::packetReceived, this, packet, dataSize, packetType::Enum::binary);
					t.detach();
				}
			}
			else if(packetLength > 0)
			{
				cout << "else if" << endl;
				if(packetLength + bytesRead > dataSize)
				{
					if(GD::debugLevel >= 2) cout << "Error: Packet length is wrong." << endl;
					packetLength = 0;
					continue;
				}
				memcpy(packet.get() + packetLength, buffer, bytesRead);
				packetLength += bytesRead;
				if(packetLength == dataSize)
				{
					std::thread t(&Server::packetReceived, this, packet, dataSize, packetType::Enum::binary);
					t.detach();
					packetLength = 0;
				}
			}
			else continue;
		}
	}
    catch(const std::exception& ex)
    {
    	_stateMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(const Exception& ex)
    {
    	_stateMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(...)
    {
    	_stateMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << endl;
    }
}

int32_t Server::getClientFileDescriptor()
{
	try
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

		if(GD::debugLevel >= 4) cout << "Connection from " << ipString << ":" << port << " accepted." << endl;

		return clientFileDescriptor;
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << endl;
    }
    return -1;
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
