#include "RPCClient.h"
#include "../HelperFunctions.h"
#include "../GD.h"
#include "../Version.h"

namespace RPC
{
RPCClient::RPCClient()
{
	try
	{
		SSLeay_add_ssl_algorithms();
		_sslMethod = SSLv23_client_method();
		SSL_load_error_strings();
		_sslCTX = SSL_CTX_new(_sslMethod);
		SSL_CTX_set_options(_sslCTX, SSL_OP_NO_SSLv2);
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

RPCClient::~RPCClient()
{
	try
	{
		if(_sslCTX) SSL_CTX_free(_sslCTX);
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

void RPCClient::invokeBroadcast(std::string server, std::string port, bool useSSL, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty())
		{
			HelperFunctions::printError("Error: Could not invoke XML RPC method for server " + server + ". methodName is empty.");
			return;
		}
		HelperFunctions::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server + " and port " + port + ".");
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Parameters:");
			for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> result = sendRequest(server, port, useSSL, _xmlRpcEncoder.encodeRequest(methodName, parameters), timedout);
		if(timedout) return;
		if(!result || result->empty())
		{
			HelperFunctions::printWarning("Warning: Response is empty. XML RPC method: " + methodName + " Server: " + server + " Port: " + port);
			return;
		}
		std::shared_ptr<RPCVariable> returnValue = _xmlRpcDecoder.decodeResponse(result);
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Response was:");
			returnValue->print();
		}
		if(returnValue->errorStruct)
		{
			if(returnValue->structValue->size() == 2)
			{
				HelperFunctions::printError("Error reading response from XML RPC server " + server + " on port " + port + ". Fault code: " + std::to_string(returnValue->structValue->at(0)->integerValue) + ". Fault string: " + returnValue->structValue->at(1)->stringValue);
			}
			else
			{
				HelperFunctions::printError("Error reading response from XML RPC server " + server + " on port " + port + ". Can't print fault struct. It is not well formed.");
			}
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

std::shared_ptr<RPCVariable> RPCClient::invoke(std::string server, std::string port, bool useSSL, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty()) return RPCVariable::createError(-32601, "Method name is empty");
		HelperFunctions::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server + " and port " + port + ".");
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Parameters:");
			for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> result = sendRequest(server, port, useSSL, _xmlRpcEncoder.encodeRequest(methodName, parameters), timedout);
		if(timedout) return RPCVariable::createError(-32300, "Request timed out.");
		if(!result || result->empty()) return RPCVariable::createError(-32700, "No response data.");
		std::shared_ptr<RPCVariable> returnValue = _xmlRpcDecoder.decodeResponse(result);
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Response was:");
			returnValue->print();
		}
		return returnValue;
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
    return RPCVariable::createError(-32700, "No response data.");
}

std::string RPCClient::getIPAddress(std::string address)
{
	try
	{
		if(address.size() < 9)
		{
			HelperFunctions::printError("Error: Server's address too short: " + address);
			return "";
		}
		if(address.substr(0, 7) == "http://") address = address.substr(7);
		else if(address.substr(0, 8) == "https://") address = address.substr(8);
		if(address.empty())
		{
			HelperFunctions::printError("Error: Server's address is empty.");
			return "";
		}
		//Remove "[" and "]" of IPv6 address
		if(address.front() == '[' && address.back() == ']') address = address.substr(1, address.size() - 2);
		if(address.empty())
		{
			HelperFunctions::printError("Error: Server's address is empty.");
			return "";
		}
		return address;
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
    return "";
}

int32_t RPCClient::getConnection(std::string& ipAddress, std::string& port, std::string& ipString)
{
	try
	{
		int32_t fileDescriptor = -1;

		//Retry for two minutes
		for(uint32_t i = 0; i < 6; ++i)
		{
			struct addrinfo *serverInfo = nullptr;
			struct addrinfo hostInfo;
			memset(&hostInfo, 0, sizeof hostInfo);

			hostInfo.ai_family = AF_UNSPEC;
			hostInfo.ai_socktype = SOCK_STREAM;

			if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
			{
				freeaddrinfo(serverInfo);
				HelperFunctions::printError("Error: Could not get address information: " + std::string(strerror(errno)));
				return -1;
			}

			if(GD::settings.tunnelClients().find(ipAddress) != GD::settings.tunnelClients().end())
			{
				if(serverInfo->ai_family == AF_INET6) ipAddress = "::1";
				else ipAddress = "127.0.0.1";
				if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
				{
					freeaddrinfo(serverInfo);
					HelperFunctions::printError("Error: Could not get address information: " + std::string(strerror(errno)));
					return -1;
				}
			}

			char ipStringBuffer[INET6_ADDRSTRLEN];
			if (serverInfo->ai_family == AF_INET) {
				struct sockaddr_in *s = (struct sockaddr_in *)serverInfo->ai_addr;
				inet_ntop(AF_INET, &s->sin_addr, ipStringBuffer, sizeof(ipStringBuffer));
			} else { // AF_INET6
				struct sockaddr_in6 *s = (struct sockaddr_in6 *)serverInfo->ai_addr;
				inet_ntop(AF_INET6, &s->sin6_addr, ipStringBuffer, sizeof(ipStringBuffer));
			}
			ipString = std::string(&ipStringBuffer[0]);

			fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
			if(fileDescriptor == -1)
			{
				HelperFunctions::printError("Error: Could not create socket for XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
				freeaddrinfo(serverInfo);
				return -1;
			}

			if(!(fcntl(fileDescriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(fileDescriptor, F_SETFL, fcntl(fileDescriptor, F_GETFL) | O_NONBLOCK) < 0)
				{
					freeaddrinfo(serverInfo);
					return -1;
				}
			}

			int32_t connectResult;
			if((connectResult = connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1 && errno != EINPROGRESS)
			{
				if(i < 5)
				{
					HelperFunctions::printError("Warning: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ". Retrying in 3 seconds...");
					freeaddrinfo(serverInfo);
					close(fileDescriptor);
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
					continue;
				}
				else
				{
					HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ". Removing server. Server has to send init again.");
					freeaddrinfo(serverInfo);
					close(fileDescriptor);
					return -2;
				}
			}
			freeaddrinfo(serverInfo);

			if(fcntl(fileDescriptor, F_SETFL, fcntl(fileDescriptor, F_GETFL) ^ O_NONBLOCK) < 0)
			{
				close(fileDescriptor);
				return -1;
			}

			if(connectResult != 0) //We have to wait for the connection
			{
				timeval timeout;
				timeout.tv_sec = 5;
				timeout.tv_usec = 0;

				pollfd pollstruct
				{
					(int)fileDescriptor,
					(short)(POLLOUT | POLLERR),
					(short)0
				};

				int32_t pollResult = poll(&pollstruct, 1, 5000);
				if(pollResult < 0 || (pollstruct.revents & POLLERR))
				{
					HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ". Poll failed. Error code: " + std::to_string(pollResult));
					close(fileDescriptor);
					return -1;
				}
				else if(pollResult > 0)
				{
					socklen_t resultLength = sizeof(connectResult);
					if(getsockopt(fileDescriptor, SOL_SOCKET, SO_ERROR, &connectResult, &resultLength) < 0)
					{
						HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ".");
						close(fileDescriptor);
						return -1;
					}
					break;
				}
				else if(pollResult == 0)
				{
					if(i < 5)
					{
						HelperFunctions::printError("Warning: Connecting to XML RPC server " + ipString + " on port " + port + " timed out. Retrying...");
						close(fileDescriptor);
						continue;
					}
					else
					{
						HelperFunctions::printError("Error: Connecting to XML RPC server " + ipString + " on port " + port + " timed out. Giving up and removing server. Server has to send init again.");
						close(fileDescriptor);
						return -2;
					}
				}
			}
		}
		return fileDescriptor;
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

std::shared_ptr<std::vector<char>> RPCClient::sendRequest(std::string server, std::string port, bool useSSL, std::string data, bool& timedout)
{
	try
	{
		_sendCounter++;
		if(_sendCounter > 100)
		{
			HelperFunctions::printCritical("Could not execute XML RPC method on server " + server + " and port " + port + ", because there are more than 100 requests queued. Your server is either not reachable currently or your connection is too slow.");
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		std::string ipAddress = getIPAddress(server);
		if(ipAddress.empty())
		{
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		std::string ipString;

		int32_t fileDescriptor = getConnection(ipAddress, port, ipString);
		if(fileDescriptor < 0)
		{
			if(fileDescriptor == -2)
			{
				timedout = true;
				GD::rpcClient.removeServer(std::pair<std::string, std::string>(server, port));
			}
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		SSL_CTX* ctx = nullptr;
		SSL* ssl = nullptr;
		if(useSSL)
		{
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, fileDescriptor);
			int32_t result = SSL_connect(ssl);
			if(result < 1)
			{
				HelperFunctions::printError("Error during TLS/SSL handshake: " + HelperFunctions::getSSLError(SSL_get_error(ssl, result)));
				SSL_free(ssl);
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
		}

		std::string msg("POST /RPC2 HTTP/1.1\r\nUser_Agent: Homegear " + std::string(VERSION) + "\r\nHost: " + ipString + ":" + port + "\r\nContent-Type: text/xml\r\nContent-Length: " + std::to_string(data.length()) + "\r\nConnection: close\r\nTE: chunked\r\n\r\n" + data + "\r\n");
		HelperFunctions::printDebug("Sending packet: " + msg);
		int32_t sentBytes = send(fileDescriptor, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		if(sentBytes == 0)
		{
			HelperFunctions::printError("Error sending data to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
			close(fileDescriptor);
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		ssize_t receivedBytes;

		fd_set socketSet;
		FD_ZERO(&socketSet);
		FD_SET(fileDescriptor, &socketSet);
		int32_t bufferMax = 1024;
		char buffer[bufferMax];
		HTTP http;

		while(!http.isFinished())
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;
			int64_t startTime = HelperFunctions::getTime();
			receivedBytes = select(fileDescriptor + 1, &socketSet, NULL, NULL, &timeout);
			switch(receivedBytes)
			{
				case 0:
					HelperFunctions::printWarning("Warning: Reading from XML RPC server timed out after " + std::to_string(HelperFunctions::getTime() - startTime) + " ms. Server: " + ipString + " Port: " + port + " Data: " + data);
					timedout = true;
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					_sendCounter--;
					return std::shared_ptr<std::vector<char>>();
				case -1:
					HelperFunctions::printError("Error reading from XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
					break;
				case 1:
					break;
				default:
					HelperFunctions::printError("Error reading from XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
			}
			if(receivedBytes != 1)
			{
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			receivedBytes = recv(fileDescriptor, buffer, 1024, 0);
			if(receivedBytes < 0)
			{
				HelperFunctions::printError("Error reading data from XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}

			try
			{
				http.process(buffer, receivedBytes);
			}
			catch(HTTPException& ex)
			{
				HelperFunctions::printError("XML RPC Client: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			if(http.getContentSize() > 104857600 || http.getHeader()->contentLength > 104857600)
			{
				HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
		}
		shutdown(fileDescriptor, 0);
		close(fileDescriptor);
		HelperFunctions::printDebug("Debug: Received packet from server " + ipString + " on port " + port + ":\n" + HelperFunctions::getHexString(*http.getContent()));
		_sendCounter--;
		if(http.isFinished()) return http.getContent(); else return std::shared_ptr<std::vector<char>>();;
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
    _sendCounter--;
    return std::shared_ptr<std::vector<char>>();
}
} /* namespace RPC */
