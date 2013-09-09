#include "RPCServer.h"
#include "../HelperFunctions.h"
#include "../GD.h"

using namespace RPC;

RPCServer::RPCServer()
{
	_rpcMethods.reset(new std::map<std::string, std::shared_ptr<RPCMethod>>);
	if(GD::settings.rpcServerThreadPriority() > 0)
	{
		_threadPriority = GD::settings.rpcServerThreadPriority();
		_threadPolicy = SCHED_FIFO;
	}
}

RPCServer::~RPCServer()
{
	stop();
}

void RPCServer::start(bool ssl)
{
	try
	{
		_useSSL = ssl;
		if(ssl)
		{
			SSL_load_error_strings();
			SSLeay_add_ssl_algorithms();
			_sslCTX = SSL_CTX_new(SSLv23_server_method());
			if(!_sslCTX)
			{
				HelperFunctions::printError("Could not start RPC Server with SSL support." + std::string(ERR_reason_error_string(ERR_get_error())));
				return;
			}

			DH* dh = DH_new();
			if(!dh)
			{
				HelperFunctions::printError("Could not start RPC Server with SSL support. Initialization of Diffie Hellman failed: " + std::string(ERR_reason_error_string(ERR_get_error())));
				SSL_CTX_free(_sslCTX);
				return;
			}
			if(!DH_generate_parameters_ex(dh, 2, DH_GENERATOR_2, 0))
			{
				HelperFunctions::printError("Could not start RPC Server with SSL support. Could not generate Diffie Hellman parameters: " + std::string(ERR_reason_error_string(ERR_get_error())));
				SSL_CTX_free(_sslCTX);
				return;
			}
			int32_t codes = 0;
			if(!DH_check(dh, &codes))
			{
				HelperFunctions::printError("Could not start RPC Server with SSL support. Diffie Hellman check failed: " + std::string(ERR_reason_error_string(ERR_get_error())));
				SSL_CTX_free(_sslCTX);
				return;
			}
			if(!DH_generate_key(dh))
			{
				HelperFunctions::printError("Could not start RPC Server with SSL support. Could not generate Diffie Hellman key: " + std::string(ERR_reason_error_string(ERR_get_error())));
				SSL_CTX_free(_sslCTX);
				return;
			}
			SSL_CTX_set_options(_sslCTX, SSL_OP_NO_SSLv2);
			SSL_CTX_set_tmp_dh(_sslCTX, dh);
			if(SSL_CTX_use_certificate_file(_sslCTX, GD::settings.certPath().c_str(), SSL_FILETYPE_PEM) < 1)
			{
				SSL_CTX_free(_sslCTX);
				HelperFunctions::printError("Could not load certificate file: " + std::string(ERR_reason_error_string(ERR_get_error())));
				return;
			}
			if(SSL_CTX_use_PrivateKey_file(_sslCTX, GD::settings.keyPath().c_str(), SSL_FILETYPE_PEM) < 1)
			{
				SSL_CTX_free(_sslCTX);
				HelperFunctions::printError("Could not load key from certificate file: " + std::string(ERR_reason_error_string(ERR_get_error())));
				return;
			}
			if(!SSL_CTX_check_private_key(_sslCTX))
			{
				SSL_CTX_free(_sslCTX);
				HelperFunctions::printError("Private key does not match the public key");
				return;
			}
		}
		_mainThread = std::thread(&RPCServer::mainThread, this);
		HelperFunctions::setThreadPriority(_mainThread.native_handle(), _threadPriority, _threadPolicy);
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
	if(_mainThread.joinable()) _mainThread.join();
	if(_sslCTX) SSL_CTX_free(_sslCTX);
}

uint32_t RPCServer::connectionCount()
{
	try
	{
		_stateMutex.lock();
		uint32_t connectionCount = _clients.size();
		_stateMutex.unlock();
		return connectionCount;
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
    return 0;
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
				_stateMutex.lock();
				if(_clients.size() >= _maxConnections)
				{
					_stateMutex.unlock();
					HelperFunctions::printError("Error: Client connection rejected, because there are too many clients connected to me.");
					shutdown(clientFileDescriptor, 0);
					close(clientFileDescriptor);
					continue;
				}
				if(_clients.find(clientFileDescriptor) != _clients.end())
				{
					HelperFunctions::printError("Tried to add client with the same file descriptor twice.");
					continue;
				}
				std::shared_ptr<Client> client(new Client());
				client->fileDescriptor = clientFileDescriptor;
				_clients[clientFileDescriptor] = client;
				_stateMutex.unlock();

				if(_useSSL)
				{
					getSSLFileDescriptor(client);
					if(!client->ssl)
					{
						removeClientFileDescriptor(client->fileDescriptor);
						shutdown(clientFileDescriptor, 0);
						close(clientFileDescriptor);
						continue;
					}
				}

				client->readThread = std::thread(&RPCServer::readClient, this, client);
				HelperFunctions::setThreadPriority(client->readThread.native_handle(), _threadPriority, _threadPolicy);
				client->readThread.detach();
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

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> data, bool keepAlive)
{
	try
	{
		if(!data || data->empty()) return;
		if(_useSSL && !client->ssl)
		{
			HelperFunctions::printError("Error: Could not send data to client. Clients SSL pointer is null.");
			return;
		}
		if(data->size() > 104857600)
		{
			HelperFunctions::printError("Error: Data size was larger than 100MB.");
			return;
		}
		int32_t ret = _useSSL ? SSL_write(client->ssl, &data->at(0), data->size()) : send(client->fileDescriptor, &data->at(0), data->size(), MSG_NOSIGNAL);
		if(!keepAlive)
		{
			removeClientFileDescriptor(client->fileDescriptor);
			shutdown(client->fileDescriptor, 1);
			close(client->fileDescriptor);
		}
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

void RPCServer::analyzeRPC(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		//HelperFunctions::printBinary(packet);
		std::string methodName;
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters;
		if(packetType == PacketType::Enum::binaryRequest) parameters = _rpcDecoder.decodeRequest(packet, methodName);
		else if(packetType == PacketType::Enum::xmlRequest) parameters = _xmlRpcDecoder.decodeRequest(packet, methodName);
		if(!parameters)
		{
			HelperFunctions::printWarning("Warning: Could not decode RPC packet.");
			return;
		}
		PacketType::Enum responseType = (packetType == PacketType::Enum::binaryRequest) ? PacketType::Enum::binaryResponse : PacketType::Enum::xmlResponse;
		if(!parameters->empty() && parameters->at(0)->errorStruct)
		{
			sendRPCResponseToClient(client, parameters->at(0), responseType, keepAlive);
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
		if(_rpcMethods->find(methodName) != _rpcMethods->end()) callMethod(client, methodName, parameters, responseType, keepAlive);
		else
		{
			HelperFunctions::printError("Warning: RPC method not found: " + methodName);
			sendRPCResponseToClient(client, RPCVariable::createError(-32601, ": Requested method not found."), responseType, keepAlive);
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

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<RPCVariable> variable, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		std::shared_ptr<std::vector<char>> data;
		if(responseType == PacketType::Enum::xmlResponse)
		{
			std::string xml = _xmlRpcEncoder.encodeResponse(variable);
			std::string header = getHttpResponseHeader(xml.size());
			xml.push_back('\r');
			xml.push_back('\n');
			if(GD::debugLevel >= 5)
			{
				HelperFunctions::printDebug("Response packet:");
				std::cout << header << xml << std::endl;
			}
			data.reset(new std::vector<char>());
			data->insert(data->begin(), header.begin(), header.end());
			data->insert(data->end(), xml.begin(), xml.end());
			sendRPCResponseToClient(client, data, keepAlive);
		}
		else if(responseType == PacketType::Enum::binaryResponse)
		{
			data = _rpcEncoder.encodeResponse(variable);
			if(GD::debugLevel >= 5)
			{
				HelperFunctions::printDebug("Response binary:");
				HelperFunctions::printBinary(data);
			}
			sendRPCResponseToClient(client, data, keepAlive);
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

void RPCServer::callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		std::shared_ptr<RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Response: ");
			ret->print();
		}
		sendRPCResponseToClient(client, ret, responseType, keepAlive);
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
	header.append("HTTP/1.1 200 OK\r\n");
	header.append("Connection: close\r\n");
	header.append("Content-Type: text/xml\r\n");
	header.append("Content-Length: ").append(std::to_string(contentLength + 21)).append("\r\n\r\n");
	header.append("<?xml version=\"1.0\"?>");
	return header;
}

void RPCServer::analyzeRPCResponse(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		std::shared_ptr<RPCVariable> response;
		if(packetType == PacketType::Enum::binaryResponse) response = _rpcDecoder.decodeResponse(packet);
		else if(packetType == PacketType::Enum::xmlResponse) response = _xmlRpcDecoder.decodeResponse(packet);
		if(!response) return;
		if(GD::debugLevel >= 3)
		{
			HelperFunctions::printWarning("Warning: RPC server received RPC response. This shouldn't happen. Response data: ");
			response->print();
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

void RPCServer::packetReceived(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest) analyzeRPC(client, packet, packetType, keepAlive);
		else if(packetType == PacketType::Enum::binaryResponse || packetType == PacketType::Enum::xmlResponse) analyzeRPCResponse(client, packet, packetType, keepAlive);
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
		if(_clients.find(clientFileDescriptor) != _clients.end()) _clients.erase(clientFileDescriptor);
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

void RPCServer::readClient(std::shared_ptr<Client> client)
{
	try
	{
		if(!client) return;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t dataSize = 0;
		PacketType::Enum packetType;
		HTTP http;

		HelperFunctions::printDebug("Listening for incoming packets from client number " + std::to_string(client->fileDescriptor) + ".");
		while(!_stopServer)
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			FD_SET(client->fileDescriptor, &readFileDescriptor);
			bytesRead = select(client->fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue; //timeout
			if(bytesRead != 1)
			{
				removeClientFileDescriptor(client->fileDescriptor);
				close(client->fileDescriptor);
				HelperFunctions::printDebug("Connection to client number " + std::to_string(client->fileDescriptor) + " closed.");
				return;
			}

			bytesRead = _useSSL ? SSL_read(client->ssl, buffer, bufferMax) : read(client->fileDescriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				if(bytesRead < 0 && _useSSL) HelperFunctions::printError("Error reading SSL packet: " + HelperFunctions::getSSLError(SSL_get_error(client->ssl, bytesRead)));
				removeClientFileDescriptor(client->fileDescriptor);
				close(client->fileDescriptor);
				HelperFunctions::printDebug("Connection to client number " + std::to_string(client->fileDescriptor) + " closed.");
				return;
			}
			if(!strncmp(&buffer[0], "Bin", 3))
			{
				http.reset();
				packetType = (buffer[3] == 0) ? PacketType::Enum::binaryRequest : PacketType::Enum::binaryResponse;
				if(bytesRead < 8) continue;
				HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				HelperFunctions::printDebug("Receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
				if(dataSize == 0) continue;
				if(dataSize > 104857600)
				{
					HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
					continue;
				}
				packet.reset(new std::vector<char>());
				packet->insert(packet->begin(), buffer + 8, buffer + bytesRead);
				if(dataSize > bytesRead - 8) packetLength = bytesRead - 8;
				else
				{
					packetLength = 0;
					std::thread t(&RPCServer::packetReceived, this, client, packet, packetType, true);
					HelperFunctions::setThreadPriority(t.native_handle(), _threadPriority, _threadPolicy);
					t.detach();
				}
			}
			else if(!strncmp(&buffer[0], "POST", 4) || !strncmp(&buffer[0], "HTTP/1.", 7))
			{
				if(GD::debugLevel >= 5)
				{
					std::vector<uint8_t> rawPacket;
					rawPacket.insert(rawPacket.begin(), buffer, buffer + bytesRead);
					HelperFunctions::printDebug("Debug: Packet received: " + HelperFunctions::getHexString(rawPacket));
				}
				packetType = (!strncmp(&buffer[0], "POST", 4)) ? PacketType::Enum::xmlRequest : PacketType::Enum::xmlResponse;
				//We are using string functions to process the buffer. So just to make sure,
				//they don't do something in the memory after buffer, we add '\0'
				buffer[bytesRead] = '\0';

				try
				{
					http.process(buffer, bytesRead);
				}
				catch(HTTPException& ex)
				{
					HelperFunctions::printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
				}

				if(http.getHeader()->contentLength > 104857600)
				{
					HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
					break;
				}
			}
			else if(packetLength > 0)
			{
				if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::binaryRequest)
				{
					if(packetLength + bytesRead > dataSize)
					{
						HelperFunctions::printError("Error: Packet length is wrong.");
						packetLength = 0;
						continue;
					}
					packet->insert(packet->begin() + packetLength, buffer, buffer + bytesRead);
					packetLength += bytesRead;
					if(packetLength == dataSize)
					{
						packet->push_back('\0');
						std::thread t(&RPCServer::packetReceived, this, client, packet, packetType, true);
						HelperFunctions::setThreadPriority(t.native_handle(), _threadPriority, _threadPolicy);
						t.detach();
						packetLength = 0;
					}
				}
				else
				{
					try
					{
						http.process(buffer, bytesRead);
					}
					catch(HTTPException& ex)
					{
						HelperFunctions::printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
					}

					if(http.getContentSize() > 104857600)
					{
						http.reset();
						HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
					}
				}
			}
			else HelperFunctions::printWarning("Warning: Uninterpretable packet received: " + std::string(buffer, buffer + bytesRead));
			if(http.isFinished())
			{
				std::thread t(&RPCServer::packetReceived, this, client, http.getContent(), packetType, http.getHeader()->connection == HTTP::Connection::Enum::keepAlive);
				HelperFunctions::setThreadPriority(t.native_handle(), _threadPriority, _threadPolicy);
				t.detach();
				packetLength = 0;
				http.reset();
			}
		}
		//This point is only reached, when stopServer is true
		removeClientFileDescriptor(client->fileDescriptor);
		shutdown(client->fileDescriptor, 0);
		close(client->fileDescriptor);
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

void RPCServer::getSSLFileDescriptor(std::shared_ptr<Client> client)
{
	try
	{
		client->ssl = SSL_new(_sslCTX);
		SSL_set_fd(client->ssl, client->fileDescriptor);
		int32_t result = SSL_accept(client->ssl);
		if(result < 1)
		{
			if(client->ssl && result != 0) HelperFunctions::printError("Error during TLS/SSL handshake: " + HelperFunctions::getSSLError(SSL_get_error(client->ssl, result)));
			else if(result == 0) HelperFunctions::printError("The TLS/SSL handshake was unsuccessful. Client number: " + std::to_string(client->fileDescriptor));
			else HelperFunctions::printError("Fatal error during TLS/SSL handshake. Client number: " + std::to_string(client->fileDescriptor));
			if(client->ssl) SSL_free(client->ssl);
			client->ssl = nullptr;
		}
		else HelperFunctions::printInfo("Info: New SSL connection to RPC server. Cipher: " + std::string(SSL_get_cipher(client->ssl)) + " (" + std::to_string(SSL_get_cipher_bits(client->ssl, 0)) + " bits)");
		return;
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
    if(client->ssl) SSL_free(client->ssl);
    client->ssl = nullptr;
}

void RPCServer::getFileDescriptor()
{
	try
	{
		addrinfo hostInfo;
		addrinfo *serverInfo = nullptr;

		int32_t fileDescriptor = -1;
		int32_t yes = 1;

		memset(&hostInfo, 0, sizeof(hostInfo));

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;
		hostInfo.ai_flags = AI_PASSIVE;
		char buffer[100];
		std::string port = _useSSL ? std::to_string(GD::settings.rpcSSLPort()) : std::to_string(GD::settings.rpcPort());

		if(getaddrinfo(GD::settings.rpcInterface().c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
		{
			HelperFunctions::printCritical("Error: Could not get address information: " + std::string(strerror(errno)));
			return;
		}

		bool bound = false;
		int32_t error = 0;
		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next)
		{
			fileDescriptor = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if(fileDescriptor == -1) continue;
			if(!(fcntl(fileDescriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(fileDescriptor, F_SETFL, fcntl(fileDescriptor, F_GETFL) | O_NONBLOCK) < 0) throw Exception("Error: Could not set socket options.");
			}
			if(setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw Exception("Error: Could not set socket options.");
			if(bind(fileDescriptor, info->ai_addr, info->ai_addrlen) == -1)
			{
				error = errno;
				continue;
			}
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
			HelperFunctions::printInfo("Info: RPC Server started listening on address " + address + " and port " + port);
			bound = true;
			break;
		}
		freeaddrinfo(serverInfo);
		if(!bound)
		{
			if(fileDescriptor > -1)	close(fileDescriptor);
			HelperFunctions::printCritical("Error: Server could not start listening: " + std::string(strerror(error)));
			return;
		}
		if(fileDescriptor == -1 || listen(fileDescriptor, _backlog) == -1 || !bound)
		{
			HelperFunctions::printCritical("Error: Server could not start listening: " + std::string(strerror(errno)));
			return;
		}
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
