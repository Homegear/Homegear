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
}

void RPCServer::start()
{
	try
	{
		_mainThread = std::thread(&RPCServer::mainThread, this);
		_mainThread.detach();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::stop()
{
	_stopServer = true;
}

void RPCServer::registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method)
{
	try
	{
		if(_rpcMethods->find(methodName) != _rpcMethods->end())
		{
			HelperFunctions::printWarning("Warning: Could not register RPC method, because a method with this name already exists.");
			return;
		}
		_rpcMethods->insert(std::pair<std::string, std::shared_ptr<RPCMethod>>(methodName, method));
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::mainThread()
{
	try
	{
		getFileDescriptor();
		int32_t clientFileDescriptor;
		while(!_stopServer)
		{
			try
			{
				clientFileDescriptor = getClientFileDescriptor();
				if(clientFileDescriptor < 0)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}
				if(clientFileDescriptor > _maxConnections)
				{
					HelperFunctions::printError("Error: Client connection rejected, because there are too many clients connected to me.");
					shutdown(clientFileDescriptor, 0);
					close(clientFileDescriptor);
					continue;
				}
				_stateMutex.lock();
				_fileDescriptors.push_back(clientFileDescriptor);
				_stateMutex.unlock();

				_readThreads.push_back(std::thread(&RPCServer::readClient, this, clientFileDescriptor));
				_readThreads.back().detach();
			}
			catch(const std::exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(Exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(...)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				_stateMutex.unlock();
			}
		}
		close(_serverFileDescriptor);
		_serverFileDescriptor = -1;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> data, bool closeConnection)
{
	try
	{
		if(!data || data->empty()) return;
		int32_t ret = send(clientFileDescriptor, &data->at(0), data->size(), MSG_NOSIGNAL);
		if(closeConnection) shutdown(clientFileDescriptor, 1);
		if(ret != (signed)data->size()) HelperFunctions::printWarning("Warning: Error sending data to client.");
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::analyzeRPC(int32_t clientFileDescriptor, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType)
{
	try
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
			HelperFunctions::printInfo("Info: Method called: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		if(_rpcMethods->find(methodName) != _rpcMethods->end()) callMethod(clientFileDescriptor, methodName, parameters, responseType);
		else if(GD::debugLevel >= 3)
		{
			HelperFunctions::printError("Warning: RPC method not found: " + methodName);
			sendRPCResponseToClient(clientFileDescriptor, RPCVariable::createError(-32601, ": Requested method not found."), responseType);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::sendRPCResponseToClient(int32_t clientFileDescriptor, std::shared_ptr<RPCVariable> variable, PacketType::Enum responseType)
{
	try
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
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::callMethod(int32_t clientFileDescriptor, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, PacketType::Enum responseType)
{
	try
	{
		std::shared_ptr<RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Response: ");
			ret->print();
		}
		sendRPCResponseToClient(clientFileDescriptor, ret, responseType);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
	try
	{
		std::shared_ptr<RPCVariable> response;
		if(packetType == PacketType::Enum::binaryResponse) response = _rpcDecoder.decodeResponse(packet);
		else if(packetType == PacketType::Enum::xmlResponse) response = _xmlRpcDecoder.decodeResponse(packet);
		if(!response) return;
		if(GD::debugLevel >= 7) response->print();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_stateMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_stateMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		struct timeval timeout;
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;

		HelperFunctions::printDebug("Listening for incoming packets from client number " + std::to_string(clientFileDescriptor) + ".");
		while(!_stopServer)
		{
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			FD_SET(clientFileDescriptor, &readFileDescriptor);
			bytesRead = select(clientFileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				removeClientFileDescriptor(clientFileDescriptor);
				close(clientFileDescriptor);
				HelperFunctions::printDebug("Connection to client number " + std::to_string(clientFileDescriptor) + " closed.");
				return;
			}

			bytesRead = read(clientFileDescriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				removeClientFileDescriptor(clientFileDescriptor);
				close(clientFileDescriptor);
				HelperFunctions::printDebug("Connection to client number " + std::to_string(clientFileDescriptor) + " closed.");
				return;
			}
			uBytesRead = bytesRead; //To prevent errors comparing signed and unsigned integers
			if(buffer[0] == 'B' && buffer[1] == 'i' && buffer[2] == 'n')
			{
				packetType = (buffer[3] == 0) ? PacketType::Enum::binaryRequest : PacketType::Enum::binaryResponse;
				if(uBytesRead < 8) continue;
				HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				HelperFunctions::printDebug("Receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
				if(dataSize == 0) continue;
				if(dataSize > 104857600)
				{
					HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
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
						if(GD::debugLevel >= 6) HelperFunctions::printDebug("Receiving xml rpc packet with content size: " + std::to_string(dataSize), 6);
						if(dataSize > 104857600)
						{
							HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
							break;
						}
						packet.reset(new std::vector<char>());
						uint32_t pos = (int32_t)stringstream.tellg() - line.length() - 1; //Don't count the new line after the header
						int32_t restLength = uBytesRead - pos;
						if(restLength < 1)
						{
							HelperFunctions::printError("Error: No data in xml rpc packet.");
							break;
						}
						if((unsigned)restLength >= dataSize)
						{
							packet->insert(packet->begin() + packetLength, buffer + pos, buffer + pos + dataSize);
							packet->push_back('\0');
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
					HelperFunctions::printError("Error: Packet length is wrong.");
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
					packet->push_back('\0');
				}
			}
		}
		//This point is only reached, when stopServer is true
		removeClientFileDescriptor(clientFileDescriptor);
		shutdown(clientFileDescriptor, 0);
		close(clientFileDescriptor);
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			inet_ntop(AF_INET, &s->sin_addr, ipString, sizeof(ipString));
		} else { // AF_INET6
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clientInfo;
			port = ntohs(s->sin6_port);
			inet_ntop(AF_INET6, &s->sin6_addr, ipString, sizeof(ipString));
		}
		std::string ipString2(&ipString[0]);
		HelperFunctions::printInfo("Info: Connection from " + ipString2 + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(clientFileDescriptor));

		return clientFileDescriptor;
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		char buffer[100];
		std::string port = std::to_string(GD::settings.rpcPort());

		if(getaddrinfo(GD::settings.rpcInterface().c_str(), port.c_str(), &hostInfo, &serverInfo) != 0) throw Exception("Error: Could not get address information.");

		bool bound = false;
		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next)
		{
			fileDescriptor = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if(fileDescriptor == -1) continue;
			if(!(fcntl(fileDescriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(fileDescriptor, F_SETFL, fcntl(fileDescriptor, F_GETFL) | O_NONBLOCK) < 0) throw Exception("Error: Could not set socket options.");
			}
			if(setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw Exception("Error: Could not set socket options.");
			if(bind(fileDescriptor, info->ai_addr, info->ai_addrlen) == -1) continue;
			std::string address;
			switch (info->ai_family)
			{
				case AF_INET:
				  inet_ntop (info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, buffer, 100);
				  address = std::string(buffer);
				  break;
				case AF_INET6:
				  inet_ntop (info->ai_family, &((struct sockaddr_in6 *) info->ai_addr)->sin6_addr, buffer, 100);
				  address = std::string(buffer);
				  break;
			}
			HelperFunctions::printInfo("Info: RPC Server started listening on address " + address);
			bound = true;
			break;
		}
		freeaddrinfo(serverInfo);
		if(!bound && fileDescriptor > -1) close(fileDescriptor);
		if(fileDescriptor == -1 || listen(fileDescriptor, _backlog) == -1 || !bound) throw Exception("Error: Server could not start listening.");
		_serverFileDescriptor = fileDescriptor;
    }
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
