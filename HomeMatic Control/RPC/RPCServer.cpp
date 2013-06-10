#include "RPCServer.h"
#include "../HelperFunctions.h"
#include "../GD.h"

using namespace RPC;

RPCServer::RPCServer()
{
}

RPCServer::~RPCServer()
{
	_stopServer = true;
	if(_mainThread.joinable()) _mainThread.join();
	for(std::vector<std::thread>::iterator i = _readThreads.begin(); i != _readThreads.end(); ++i)
	{
		if(i->joinable()) i->join();
	}
	if(GD::debugLevel >= 4) cout << "XML RPC Server successfully shut down." << endl;
}

void RPCServer::start()
{

	_mainThread = std::thread(&RPCServer::mainThread, this);
	_mainThread.detach();
}

void RPCServer::registerMethod(std::string methodName, shared_ptr<RPCMethod> method)
{
	if(_rpcMethods.find(methodName) != _rpcMethods.end())
	{
		if(GD::debugLevel >= 3) cout << "Warning: Could not register RPC method, because a method with this name already exists." << endl;
		return;
	}
	_rpcMethods[methodName] = method;
}

void RPCServer::mainThread()
{
	getFileDescriptor();
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
		_fileDescriptors.push_back(clientFileDescriptor);
		_stateMutex.unlock();

		_readThreads.push_back(std::thread(&RPCServer::readClient, this, clientFileDescriptor));
		_readThreads.back().detach();
	}
	close(_serverFileDescriptor);
	_serverFileDescriptor = -1;
}

void RPCServer::sendRPCResponseToClient(int32_t clientFileDescriptor, shared_ptr<char> data, uint32_t dataLength)
{
	try
	{
		_sendMutex.lock();
		int32_t ret = send(clientFileDescriptor, data.get(), dataLength, 0);
		_sendMutex.unlock();
		if(ret != (signed)dataLength && GD::debugLevel >= 3) cout << "Warning: Error sending data to client." << endl;
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

void RPCServer::analyzeBinaryRPC(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength)
{
	std::string methodName;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters = _rpcDecoder.decodeRequest(packet, packetLength, &methodName);
	if(parameters == nullptr) return; //parameters should never be nullptr, but just to make sure
	cout << "Method called: " << methodName << " Parameters:";
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
	{
		cout << " Type: " << (int32_t)(*i)->type;
		if((*i)->type == RPCVariableType::rpcString)
		{
			cout << " Value: " << (*i)->stringValue;
		}
		else if((*i)->type == RPCVariableType::rpcInteger)
		{
			cout << " Value: " << (*i)->integerValue;
		}
	}
	cout << endl;
	if(_rpcMethods.find(methodName) != _rpcMethods.end())
	{
		shared_ptr<RPCVariable> ret = _rpcMethods[methodName]->invoke(parameters);
		std::shared_ptr<char> data;
		uint32_t dataLength = _rpcEncoder.encodeResponse(data, ret);
		_rpcDecoder.decodeResponse(data, dataLength, 8)->print();
		sendRPCResponseToClient(clientFileDescriptor, data, dataLength);
	}
	else if(GD::debugLevel >= 3)
	{
		cout << "Warning: RPC method not found: " << methodName << endl;
		std::shared_ptr<char> data;
		uint32_t dataLength = _rpcEncoder.encodeResponse(data, RPCVariable::createError(-1, ": unknown method name"));
		sendRPCResponseToClient(clientFileDescriptor, data, dataLength);
	}
}

void RPCServer::analyzeBinaryRPCResponse(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength)
{
	std::shared_ptr<RPCVariable> response = _rpcDecoder.decodeResponse(packet, packetLength);
	if(response == nullptr) return; //parameters should never be nullptr, but just to make sure
	if(GD::debugLevel >= 7) response->print();
}

std::pair<std::string, std::string> RPCServer::getAddressAndPort(std::string address)
{
	try
	{
		if(address.size() < 8) return std::pair<std::string, std::string>();
		if(address.substr(0, 7) == "http://")
		{
			address = address.substr(7);
		}
		if(address.substr(0, 8) == "https://")
		{
			if(GD::debugLevel >= 2) cout << "Error: Cannot connect to RPC server, because SSL is not supported." << endl;
			return std::pair<std::string, std::string>();
		}
		uint32_t pos = address.find(':');
		if(pos == std::string::npos || pos + 1 >= address.size())
		{
			if(GD::debugLevel >= 2) cout << "Error: Cannot connect to RPC server, because no port was specified." << endl;
			return std::pair<std::string, std::string>();
		}
		if(pos < 7)
		{
			if(GD::debugLevel >= 2) cout << "Error: Cannot connect to RPC server, because no address was specified." << endl;
			return std::pair<std::string, std::string>();
		}
		return std::pair<std::string, std::string>(address.substr(0, pos), address.substr(pos + 1));
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
    return std::pair<std::string, std::string>();
}

void RPCServer::createLocalClient(std::string address)
{
	try
	{

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

void RPCServer::removeLocalClient(std::string address)
{

}

void RPCServer::packetReceived(int32_t clientFileDescriptor, std::shared_ptr<char> packet, uint32_t packetLength, PacketType::Enum packetType)
{
	try
	{
		if(packetType == PacketType::Enum::binaryRequest) analyzeBinaryRPC(clientFileDescriptor, packet, packetLength);
		if(packetType == PacketType::Enum::binaryResponse) analyzeBinaryRPCResponse(clientFileDescriptor, packet, packetLength);
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

void RPCServer::removeClientFileDescriptor(int32_t clientFileDescriptor)
{
	try
	{
		_stateMutex.lock();
		for(std::vector<int32_t>::iterator i = _fileDescriptors.begin(); i != _fileDescriptors.end(); ++i)
		{
			if(*i == clientFileDescriptor)
			{
				_fileDescriptors.erase(i);
				_stateMutex.unlock();
				return;
			}
		}
		_stateMutex.unlock();
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

void RPCServer::readClient(int32_t clientFileDescriptor)
{
	try
	{
		char buffer[1024];
		std::shared_ptr<char> packet(new char[1]);
		uint32_t packetLength;
		int32_t bytesRead;
		uint32_t bufferSize;
		uint32_t dataSize = 0;
		PacketType::Enum packetType;
		cout << "Listening for incoming packets from client number " << clientFileDescriptor << "." << endl;
		while(!_stopServer)
		{
			bytesRead = read(clientFileDescriptor, buffer, sizeof(buffer));
			if(bytesRead <= 0)
			{
				removeClientFileDescriptor(clientFileDescriptor);
				close(clientFileDescriptor);
				if(GD::debugLevel >= 5) cout << "Connection to client number " << clientFileDescriptor << " closed." << endl;
				return;
			}
			bufferSize = bytesRead; //To prevent errors comparing signed and unsigned integers
			if(buffer[0] == 'B' && buffer[1] == 'i' && buffer[2] == 'n')
			{
				packetType = (buffer[3] == 0) ? PacketType::Enum::binaryRequest : PacketType::Enum::binaryResponse;
				if(bufferSize < 8) continue;
				HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				if(GD::debugLevel >= 6) cout << "Receiving packet with size: " << dataSize << endl;
				if(dataSize == 0) continue;
				if(dataSize > 104857600)
				{
					if(GD::debugLevel >= 2) cout << "Error: Packet with data larger than 100 MiB received." << endl;
					continue;
				}
				packet.reset(new char[dataSize]);
				if(dataSize > bufferSize - 8)
				{
					memcpy(packet.get(), buffer + 8, bufferSize - 8);
					packetLength = bufferSize - 8;
				}
				else
				{
					packetLength = 0;
					memcpy(packet.get(), buffer + 8, bufferSize - 8);
					std::thread t(&RPCServer::packetReceived, this, clientFileDescriptor, packet, dataSize, packetType);
					t.detach();
				}
			}
			else if(packetLength > 0)
			{
				if(packetLength + bufferSize > dataSize)
				{
					if(GD::debugLevel >= 2) cout << "Error: Packet length is wrong." << endl;
					packetLength = 0;
					continue;
				}
				memcpy(packet.get() + packetLength, buffer, bufferSize);
				packetLength += bufferSize;
				if(packetLength == dataSize)
				{
					std::thread t(&RPCServer::packetReceived, this, clientFileDescriptor, packet, dataSize, packetType);
					t.detach();
					packetLength = 0;
				}
			}
		}
		//This point is only reached, when stopServer is true
		removeClientFileDescriptor(clientFileDescriptor);
		close(clientFileDescriptor);
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

int32_t RPCServer::getClientFileDescriptor()
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

		if(GD::debugLevel >= 4) cout << "Connection from " << ipString << ":" << port << " accepted. Client number: " << clientFileDescriptor << endl;

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

void RPCServer::getFileDescriptor()
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
