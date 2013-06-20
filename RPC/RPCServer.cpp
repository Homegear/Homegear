#include "RPCServer.h"
#include "../HelperFunctions.h"
#include "../GD.h"

using namespace RPC;

RPCServer::RPCServer()
{
	_rpcMethods.reset(new std::map<std::string, std::shared_ptr<RPCMethod>>);
}

RPCServer::~RPCServer()
{
	stop();
	if(GD::debugLevel >= 4) std::cout << "XML RPC Server successfully shut down." << std::endl;
}

void RPCServer::start()
{

	_mainThread = std::thread(&RPCServer::mainThread, this);
	_mainThread.detach();
}

void RPCServer::stop()
{
	_stopServer = true;
	if(_mainThread.joinable()) _mainThread.join();
	for(std::vector<std::thread>::iterator i = _readThreads.begin(); i != _readThreads.end(); ++i)
	{
		if(i->joinable()) i->join();
	}
}

void RPCServer::registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method)
{
	if(_rpcMethods->find(methodName) != _rpcMethods->end())
	{
		if(GD::debugLevel >= 3) std::cout << "Warning: Could not register RPC method, because a method with this name already exists." << std::endl;
		return;
	}
	_rpcMethods->insert(std::pair<std::string, std::shared_ptr<RPCMethod>>(methodName, method));
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
			if(GD::debugLevel >= 2) std::cout << "Error: Client connection rejected, because there are too many clients connected to me." << std::endl;
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

void RPCServer::sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> data, bool closeConnection)
{
	try
	{
		if(!data) return;
		_sendMutex.lock();
		int32_t ret = send(clientFileDescriptor, &data->at(0), data->size(), 0);
		if(closeConnection) shutdown(clientFileDescriptor, 1);
		_sendMutex.unlock();
		if(ret != (signed)data->size() && GD::debugLevel >= 3) std::cout << "Warning: Error sending data to client." << std::endl;
	}
    catch(const std::exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_sendMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void RPCServer::analyzeRPC(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType)
{
	std::string methodName;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters;
	if(packetType == PacketType::Enum::binaryRequest) parameters = _rpcDecoder.decodeRequest(packet, methodName);
	else if(packetType == PacketType::Enum::xmlRequest) parameters = _xmlRpcDecoder.decodeRequest(packet, methodName);
	if(!parameters) return;
	PacketType::Enum responseType = (packetType == PacketType::Enum::binaryRequest) ? PacketType::Enum::binaryResponse : PacketType::Enum::xmlResponse;
	if(!parameters->empty() && parameters->at(0)->errorStruct)
	{
		sendRPCResponseToClient(clientFileDescriptor, parameters->at(0), responseType);
		return;
	}
	if(GD::debugLevel >= 4)
	{
		std::cout << "Method called: " << methodName << " Parameters:" << std::endl;
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
		{
			(*i)->print();
		}
	}
	if(_rpcMethods->find(methodName) != _rpcMethods->end()) callMethod(clientFileDescriptor, methodName, parameters, responseType);
	else if(GD::debugLevel >= 3)
	{
		std::cerr << "Warning: RPC method not found: " << methodName << std::endl;
		sendRPCResponseToClient(clientFileDescriptor, RPCVariable::createError(-32601, ": Requested method not found."), responseType);
	}
}

void RPCServer::sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<RPCVariable> variable, PacketType::Enum responseType)
{
	std::shared_ptr<std::vector<char>> data;
	if(responseType == PacketType::Enum::xmlResponse)
	{
		std::string xml = _xmlRpcEncoder.encodeResponse(variable);
		std::string header = getHttpResponseHeader(xml.size());
		xml.push_back('\n');
		data.reset(new std::vector<char>(header.size() + xml.size()));
		data->insert(data->begin(), header.begin(), header.end());
		data->insert(data->begin() + header.size(), xml.begin(), xml.end());
		sendRPCResponseToClient(clientFileDescriptor, data, true);
	}
	else if(responseType == PacketType::Enum::binaryResponse)
	{
		data = _rpcEncoder.encodeResponse(variable);
		sendRPCResponseToClient(clientFileDescriptor, data, false);
	}
}

void RPCServer::callMethod(int32_t clientFileDescriptor, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, PacketType::Enum responseType)
{
	try
	{
		std::shared_ptr<RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::debugLevel >= 5)
		{
			std::cout << "Response: " << std::endl;
			ret->print();
		}
		sendRPCResponseToClient(clientFileDescriptor, ret, responseType);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::string RPCServer::getHttpResponseHeader(uint32_t contentLength)
{
	std::string header;
	header.append("HTTP/1.1 200 OK\n");
	header.append("Connection: close\n");
	header.append("Content-Type: text/xml\n");
	header.append("Content-Length: ").append(std::to_string(contentLength)).append("\n\n");
	header.append("<?xml version=\"1.0\"?>");
	return header;
}

void RPCServer::analyzeRPCResponse(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType)
{
	std::shared_ptr<RPCVariable> response;
	if(packetType == PacketType::Enum::binaryResponse) response = _rpcDecoder.decodeResponse(packet);
	else if(packetType == PacketType::Enum::xmlResponse) response = _xmlRpcDecoder.decodeResponse(packet);
	if(!response) return;
	if(GD::debugLevel >= 7) response->print();
}

void RPCServer::packetReceived(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType)
{
	try
	{
		if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest) analyzeRPC(clientFileDescriptor, packet, packetType);
		if(packetType == PacketType::Enum::binaryResponse || packetType == PacketType::Enum::xmlResponse) analyzeRPCResponse(clientFileDescriptor, packet, packetType);
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_stateMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_stateMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void RPCServer::readClient(int32_t clientFileDescriptor)
{
	try
	{
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		int32_t lineBufferMax = 128;
		char lineBuffer[lineBufferMax];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t uBytesRead;
		uint32_t dataSize = 0;
		PacketType::Enum packetType;
		/*struct timeval timeout;
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;*/

		if(GD::debugLevel >= 5) std::cout << "Listening for incoming packets from client number " << clientFileDescriptor << "." << std::endl;
		while(!_stopServer)
		{
			/*fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			FD_SET(clientFileDescriptor, &readFileDescriptor);
			bytesRead = select(clientFileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				removeClientFileDescriptor(clientFileDescriptor);
				close(clientFileDescriptor);
				if(GD::debugLevel >= 5) std::cout << "Connection to client number " << clientFileDescriptor << " closed." << std::endl;
				return;
			}*/

			bytesRead = read(clientFileDescriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				removeClientFileDescriptor(clientFileDescriptor);
				close(clientFileDescriptor);
				if(GD::debugLevel >= 5) std::cout << "Connection to client number " << clientFileDescriptor << " closed." << std::endl;
				return;
			}
			uBytesRead = bytesRead; //To prevent errors comparing signed and unsigned integers
			if(buffer[0] == 'B' && buffer[1] == 'i' && buffer[2] == 'n')
			{
				packetType = (buffer[3] == 0) ? PacketType::Enum::binaryRequest : PacketType::Enum::binaryResponse;
				if(uBytesRead < 8) continue;
				HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				if(GD::debugLevel >= 6) std::cout << "Receiving binary rpc packet with size: " << dataSize << std::endl;
				if(dataSize == 0) continue;
				if(dataSize > 104857600)
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Packet with data larger than 100 MiB received." << std::endl;
					continue;
				}
				packet.reset(new std::vector<char>());
				packet->insert(packet->begin(), buffer + 8, buffer + uBytesRead);
				if(dataSize > uBytesRead - 8) packetLength = uBytesRead - 8;
				else
				{
					packetLength = 0;
					std::thread t(&RPCServer::packetReceived, this, clientFileDescriptor, packet, packetType);
					t.detach();
				}
			}
			else if(buffer[0] == 'P' && buffer[1] == 'O' && buffer[2] == 'S' && buffer[3] == 'T')
			{
				buffer[uBytesRead] = '\0';
				std::istringstream stringstream(buffer);
				std::string line;
				dataSize = 0;
				bool contentType = false;
				bool host = false;
				while(stringstream.good() && !stringstream.eof())
				{
					stringstream.getline(lineBuffer, lineBufferMax);
					line = std::string(lineBuffer);
					HelperFunctions::toLower(line);
					if(line.size() > 14 && line.substr(0, 13) == "content-type:")
					{
						contentType = true;
						if(line.substr(14) != "text/xml") break;
					}
					else if(line.size() > 16 && line.substr(0, 15) == "content-length:")
					{
						try	{ dataSize = std::stoll(line.substr(16)); } catch(...) {}
						if(dataSize == 0) break;
					}
					else if(line.size() > 6 && line.substr(0, 5) == "host:")
					{
						host = true; //Required by xml rpc standard
					}
					else if(contentType && host && line.size() > 5 && line.substr(0, 5) == "<?xml")
					{
						packetType = PacketType::Enum::xmlRequest;
						if(GD::debugLevel >= 6) std::cout << "Receiving xml rpc packet with content size: " << dataSize << std::endl;
						if(dataSize > 104857600)
						{
							if(GD::debugLevel >= 2) std::cout << "Error: Packet with data larger than 100 MiB received." << std::endl;
							break;
						}
						packet.reset(new std::vector<char>(dataSize + 1));
						uint32_t pos = (int32_t)stringstream.tellg() - line.length() - 1; //Don't count the new line after the header
						int32_t restLength = uBytesRead - pos;
						if(restLength < 1)
						{
							if(GD::debugLevel >= 2) std::cout << "Error: No data in xml rpc packet." << std::endl;
							break;
						}
						if((unsigned)restLength >= dataSize)
						{
							packet->insert(packet->begin() + packetLength, buffer + pos, buffer + pos + dataSize);
							packet->at(dataSize) = '\0';
							packetLength = 0;
							std::thread t(&RPCServer::packetReceived, this, clientFileDescriptor, packet, packetType);
							t.detach();
						}
						else
						{
							packetLength = restLength;
							packet->insert(packet->begin() + packetLength, buffer + pos, buffer + pos + restLength);
						}
					}
				}
			}
			else if(packetLength > 0)
			{
				if(packetLength + uBytesRead == dataSize + 1)
				{
					//For XML RPC packets the last byte is "\n", which is not counted in "Content-Length"
					uBytesRead -= 1;
				}
				if(packetLength + uBytesRead > dataSize)
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Packet length is wrong." << std::endl;
					packetLength = 0;
					continue;
				}
				packet->insert(packet->begin() + packetLength, buffer, buffer + uBytesRead);
				packetLength += uBytesRead;
				if(packetLength == dataSize)
				{
					std::thread t(&RPCServer::packetReceived, this, clientFileDescriptor, packet, packetType);
					t.detach();
					packetLength = 0;
					packet->at(dataSize) = '\0';
				}
			}
		}
		//This point is only reached, when stopServer is true
		removeClientFileDescriptor(clientFileDescriptor);
		close(clientFileDescriptor);
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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

		if(GD::debugLevel >= 4) std::cout << "Connection from " << ipString << ":" << port << " accepted. Client number: " << clientFileDescriptor << std::endl;

		return clientFileDescriptor;
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return -1;
}

void RPCServer::getFileDescriptor()
{
	try
	{
		struct addrinfo hostInfo, *serverInfo;

		int32_t fileDescriptor = -1;
		int32_t yes = 1;

		memset(&hostInfo, 0, sizeof(hostInfo));

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;
		hostInfo.ai_flags = AI_PASSIVE;

		if(getaddrinfo(0, _port.c_str(), &hostInfo, &serverInfo) != 0) throw Exception("Error: Could not get address information.");

		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next) {
			fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
			if(fileDescriptor == -1) continue;
			if(setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw Exception("Error: Could not set socket options.");
			if(bind(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) continue;
			break;
		}
		freeaddrinfo(serverInfo);
		if(fileDescriptor == -1 || listen(fileDescriptor, _backlog) == -1) throw Exception("Error: Server could not start listening.");
		_serverFileDescriptor = fileDescriptor;
    }
    catch(const std::exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_sendMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_sendMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}
