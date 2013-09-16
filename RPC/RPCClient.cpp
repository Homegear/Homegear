/* Copyright 2013 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

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
		SSL_load_error_strings();
		_sslCTX = SSL_CTX_new(SSLv23_client_method());
		SSL_CTX_set_options(_sslCTX, SSL_OP_NO_SSLv2);
		if(!_sslCTX)
		{
			HelperFunctions::printError("Could not initialize SSL support for RPC client." + std::string(ERR_reason_error_string(ERR_get_error())));
			_sslCTX = nullptr;
			return;
		}
		if(!SSL_CTX_load_verify_locations(_sslCTX, NULL, "/etc/ssl/certs"))
		{
			HelperFunctions::printError("Could not load root certificates from /etc/ssl/certs." + std::string(ERR_reason_error_string(ERR_get_error())));
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

void RPCClient::invokeBroadcast(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty())
		{
			HelperFunctions::printError("Error: Could not invoke XML RPC method for server " + server->address.first + ". methodName is empty.");
			return;
		}
		HelperFunctions::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server->address.first + " and port " + server->address.second + ".");
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Parameters:");
			for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> requestData;
		if(server->binary) requestData = _rpcEncoder.encodeRequest(methodName, parameters);
		else requestData = _xmlRpcEncoder.encodeRequest(methodName, parameters);
		std::shared_ptr<std::vector<char>> result = sendRequest(server, requestData, timedout);
		if(timedout) return;
		if(!result || result->empty())
		{
			HelperFunctions::printWarning("Warning: Response is empty. XML RPC method: " + methodName + " Server: " + server->address.first + " Port: " + server->address.second);
			return;
		}
		std::shared_ptr<RPCVariable> returnValue;
		if(server->binary) returnValue = _rpcDecoder.decodeResponse(result);
		else returnValue = _xmlRpcDecoder.decodeResponse(result);
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Response was:");
			returnValue->print();
		}
		if(returnValue->errorStruct)
		{
			if(returnValue->structValue->size() == 2)
			{
				HelperFunctions::printError("Error reading response from XML RPC server " + server->address.first + " on port " + server->address.second + ". Fault code: " + std::to_string(returnValue->structValue->at(0)->integerValue) + ". Fault string: " + returnValue->structValue->at(1)->stringValue);
			}
			else
			{
				HelperFunctions::printError("Error reading response from XML RPC server " + server->address.first + " on port " + server->address.second + ". Can't print fault struct. It is not well formed.");
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

std::shared_ptr<RPCVariable> RPCClient::invoke(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty()) return RPCVariable::createError(-32601, "Method name is empty");
		HelperFunctions::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server->address.first + " and port " + server->address.second + ".");
		if(GD::debugLevel >= 5)
		{
			HelperFunctions::printDebug("Parameters:");
			for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> requestData;
		if(server->binary) requestData = _rpcEncoder.encodeRequest(methodName, parameters);
		else requestData = _xmlRpcEncoder.encodeRequest(methodName, parameters);
		std::shared_ptr<std::vector<char>> result = sendRequest(server, requestData, timedout);
		if(timedout) return RPCVariable::createError(-32300, "Request timed out.");
		if(!result || result->empty()) return RPCVariable::createError(-32700, "No response data.");
		std::shared_ptr<RPCVariable> returnValue;
		if(server->binary) returnValue = _rpcDecoder.decodeResponse(result);
		else returnValue = _xmlRpcDecoder.decodeResponse(result);
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

int32_t RPCClient::getConnection(std::string& hostname, const std::string& port, std::string& ipAddress)
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

			if(getaddrinfo(hostname.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
			{
				freeaddrinfo(serverInfo);
				HelperFunctions::printError("Error: Could not get address information: " + std::string(strerror(errno)));
				return -1;
			}

			if(GD::settings.tunnelClients().find(hostname) != GD::settings.tunnelClients().end())
			{
				if(serverInfo->ai_family == AF_INET6) hostname = "::1";
				else hostname = "127.0.0.1";
				if(getaddrinfo(hostname.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
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
			ipAddress = std::string(&ipStringBuffer[0]);

			fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
			if(fileDescriptor == -1)
			{
				HelperFunctions::printError("Error: Could not create socket for XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno));
				freeaddrinfo(serverInfo);
				return -1;
			}
			int32_t optValue = 1;
			if(setsockopt(fileDescriptor, SOL_SOCKET, SO_KEEPALIVE, (void*)&optValue, sizeof(int32_t)) == -1)
			{
				HelperFunctions::printError("Error: Could not set socket options for XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno));
				freeaddrinfo(serverInfo);
				return -1;
			}

			if(!(fcntl(fileDescriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(fileDescriptor, F_SETFL, fcntl(fileDescriptor, F_GETFL) | O_NONBLOCK) < 0)
				{
					HelperFunctions::printError("Error: Could not set socket options for XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno));
					freeaddrinfo(serverInfo);
					return -1;
				}
			}

			int32_t connectResult;
			if((connectResult = connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1 && errno != EINPROGRESS)
			{
				if(i < 5)
				{
					HelperFunctions::printError("Warning: Could not connect to XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno) + ". Retrying in 3 seconds...");
					freeaddrinfo(serverInfo);
					close(fileDescriptor);
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
					continue;
				}
				else
				{
					HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno) + ". Removing server. Server has to send init again.");
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
					if(i < 5)
					{
						HelperFunctions::printWarning("Warning: Could not connect to XML RPC server " + ipAddress + " on port " + port + ". Poll failed with error code: " + std::to_string(pollResult) + ". Retrying in 3 seconds...");
						close(fileDescriptor);
						std::this_thread::sleep_for(std::chrono::milliseconds(3000));
						continue;
					}
					else
					{
						HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipAddress + " on port " + port + ". Poll failed with error code: " + std::to_string(pollResult) + ". Giving up and removing server. Server has to send init again.");
						close(fileDescriptor);
						return -2;
					}
				}
				else if(pollResult > 0)
				{
					socklen_t resultLength = sizeof(connectResult);
					if(getsockopt(fileDescriptor, SOL_SOCKET, SO_ERROR, &connectResult, &resultLength) < 0)
					{
						HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipAddress + " on port " + port + ": " + strerror(errno) + ".");
						close(fileDescriptor);
						return -1;
					}
					break;
				}
				else if(pollResult == 0)
				{
					if(i < 5)
					{
						HelperFunctions::printError("Warning: Connecting to XML RPC server " + ipAddress + " on port " + port + " timed out. Retrying...");
						close(fileDescriptor);
						continue;
					}
					else
					{
						HelperFunctions::printError("Error: Connecting to XML RPC server " + ipAddress + " on port " + port + " timed out. Giving up and removing server. Server has to send init again.");
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

SSL* RPCClient::getSSL(int32_t fileDescriptor, bool verifyCertificate)
{
	try
	{
		SSL* ssl;
		if(!_sslCTX)
		{
			HelperFunctions::printError("RPC Client: Could not connect to server using SSL. SSL is not initialized. Look for previous error messages.");
			return nullptr;
		}
		ssl = SSL_new(_sslCTX);
		SSL_set_fd(ssl, fileDescriptor);
		int32_t result = SSL_connect(ssl);
		if(!ssl || result < 1)
		{
			HelperFunctions::printError("RPC Client: Error during TLS/SSL handshake: " + HelperFunctions::getSSLError(SSL_get_error(ssl, result)));
			SSL_free(ssl);
			return nullptr;
		}

		X509* serverCert = SSL_get_peer_certificate(ssl);
		if(!serverCert)
		{
			HelperFunctions::printError("RPC Client: Could not get server certificate.");
			SSL_free(ssl);
			return nullptr;
		}

		//When no certificate was received, verify returns X509_V_OK!
		if((result = SSL_get_verify_result(ssl)) != X509_V_OK && (result != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT || verifyCertificate))
		{
			HelperFunctions::printError("RPC Client: Error during TLS/SSL handshake: " + HelperFunctions::getSSLCertVerificationError(result));
			SSL_free(ssl);
			return nullptr;
		}
		return ssl;
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
    return nullptr;
}

void RPCClient::getFileDescriptor(std::shared_ptr<RemoteRPCServer>& server, bool& timedout)
{
	try
	{
		if(server->fileDescriptor > -1)
		{
			shutdown(server->fileDescriptor, 0);
			close(server->fileDescriptor);
		}

		server->fileDescriptor = getConnection(server->hostname, server->address.second, server->ipAddress);
		if(server->fileDescriptor < 0)
		{
			if(server->fileDescriptor == -2)
			{
				timedout = true;
				GD::rpcClient.removeServer(server->address);
			}
			return;
		}

		if(server->useSSL)
		{
			bool verifyCertificate = (!server->settings || server->settings->verifyCertificate) ? true : false;
			server->ssl = getSSL(server->fileDescriptor, verifyCertificate);
			if(!server->ssl)
			{
				shutdown(server->fileDescriptor, 0);
				close(server->fileDescriptor);
				server->fileDescriptor = -1;
			}
		}
		server->socket = SocketOperations(server->fileDescriptor, server->ssl);
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

void RPCClient::closeConnection(std::shared_ptr<RemoteRPCServer>& server)
{
	if(!server) return;
	if(server->ssl) SSL_free(server->ssl);
	server->ssl = nullptr;
	if(server->fileDescriptor > -1) close(server->fileDescriptor);
	server->fileDescriptor = -1;
}

std::shared_ptr<std::vector<char>> RPCClient::sendRequest(std::shared_ptr<RemoteRPCServer> server, std::shared_ptr<std::vector<char>>& data, bool& timedout)
{
	try
	{
		if(!server || !data)
		{
			HelperFunctions::printError("RPC Client: Could not send packet. Pointer to server or data is nullptr.");
			return std::shared_ptr<std::vector<char>>();
		}
		signal(SIGPIPE, SIG_IGN);

		if(server->hostname.empty()) server->hostname = getIPAddress(server->address.first);
		if(server->hostname.empty()) return std::shared_ptr<std::vector<char>>();
		//Get settings pointer every time this method is executed, because
		//the settings might change.
		server->settings = GD::clientSettings.get(server->hostname);
		if(!server->useSSL && server->settings && server->settings->forceSSL)
		{
			HelperFunctions::printError("RPC Client: Tried to send unencrypted packet to " + server->hostname + " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again.");
			GD::rpcClient.removeServer(server->address);
			return std::shared_ptr<std::vector<char>>();
		}

		_sendCounter++;
		if(_sendCounter > 100)
		{
			HelperFunctions::printCritical("Could not execute XML RPC method on server " + server->address.first + " and port " + server->address.second + ", because there are more than 100 requests queued. Your server is either not reachable currently or your connection is too slow.");
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		if(server->fileDescriptor < 0) getFileDescriptor(server, timedout);
		else if(!server->socket.connected())
		{
			HelperFunctions::printDebug("RPC Client: Existing connection (" + std::to_string(server->fileDescriptor) + ") has been closed. Reconnecting...");
			closeConnection(server);
			getFileDescriptor(server, timedout);
		}
		else HelperFunctions::printDebug("RPC Client: Using existing connection: " + std::to_string(server->fileDescriptor));
		if(server->fileDescriptor < 0)
		{
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		if(server->settings->authType != ClientSettings::Settings::AuthType::none && !server->auth.initialized())
		{
			if(server->settings->userName.empty() || server->settings->password.empty())
			{
				HelperFunctions::printError("Error: No user name or password specified in config file for XML RPC server " + server->ipAddress + " on port " + server->address.second + ". Closing connection.");
				closeConnection(server);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			server->auth = Auth(server->socket, server->settings->userName, server->settings->password);
		}

		if(!server->binary)
		{
			std::string header = "POST /RPC2 HTTP/1.1\r\nUser_Agent: Homegear " + std::string(VERSION) + "\r\nHost: " + server->hostname + ":" + server->address.second + "\r\nContent-Type: text/xml\r\nContent-Length: " + std::to_string(data->size()) + "\r\nConnection: " + (server->keepAlive ? "Keep-Alive" : "close") + "\r\nTE: chunked\r\n";
			if(server->settings->authType == ClientSettings::Settings::AuthType::basic)
			{
				HelperFunctions::printDebug("Using Basic Access Authentication.");
				header += server->auth.basicClient() + "\r\n";
			}
			header += "\r\n";
			data->insert(data->begin(), header.begin(), header.end());
			data->push_back('\r');
			data->push_back('\n');
		}

		if(GD::debugLevel >= 5) HelperFunctions::printDebug("Sending packet: " + HelperFunctions::getHexString(*data));
		try
		{
			server->socket.proofwrite(data);
		}
		catch(SocketDataLimitException& ex)
		{
			HelperFunctions::printWarning("Warning: " + ex.what());
			closeConnection(server);
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}
		catch(SocketOperationException& ex)
		{
			HelperFunctions::printError("Error: Could not send data to XML RPC server " + server->ipAddress + " on port " + server->address.second + ": " + ex.what() + ". Giving up.");
			server->ssl = nullptr; //After SSL_write failed, the pointer does not seem to be valid anymore. Calling SSL_free causes segfault.
			closeConnection(server);
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		ssize_t receivedBytes;

		fd_set socketSet;
		FD_ZERO(&socketSet);
		FD_SET(server->fileDescriptor, &socketSet);
		int32_t bufferMax = 1024;
		char buffer[bufferMax];
		HTTP http;
		uint32_t packetLength = 0;
		uint32_t dataSize = 0;
		std::shared_ptr<std::vector<char>> packet;
		if(server->binary) packet.reset(new std::vector<char>());

		while(!http.isFinished()) //This is equal to while(true) for binary packets
		{
			try
			{
				receivedBytes = server->socket.proofread(buffer, bufferMax);
				//Some clients send only one byte in the first packet
				if(packetLength == 0 && receivedBytes == 1) receivedBytes += server->socket.proofread(&buffer[1], bufferMax - 1);
			}
			catch(SocketTimeOutException& ex)
			{
				HelperFunctions::printWarning("Warning: Reading from XML RPC server timed out. Server: " + server->ipAddress + " Port: " + server->address.second + " Data: " + HelperFunctions::getHexString(*data));
				timedout = true;
				if(!server->keepAlive) closeConnection(server);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			catch(SocketClosedException& ex)
			{
				HelperFunctions::printWarning("Warning: " + ex.what());
				if(!server->keepAlive) closeConnection(server);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			catch(SocketOperationException& ex)
			{
				HelperFunctions::printError(ex.what());
				if(!server->keepAlive) closeConnection(server);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			//We are using string functions to process the buffer. So just to make sure,
			//they don't do something in the memory after buffer, we add '\0'
			buffer[receivedBytes] = '\0';
			if(!strncmp(buffer, "401", 3) || !strncmp(&buffer[9], "401", 3)) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
			{
				HelperFunctions::printError("Error: Authentication failed. Server " + server->ipAddress + ", port " + server->address.second + ". Check user name and password in rpcclients.conf.");
				if(!server->keepAlive) closeConnection(server);
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}

			if(server->binary)
			{
				if(dataSize == 0)
				{
					if(!(buffer[3] & 1) && buffer[3] != 0xFF)
					{
						HelperFunctions::printError("Error: RPC client received binary request as response from server " + server->ipAddress + " on port " + server->address.second);
						if(!server->keepAlive) closeConnection(server);
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					if(receivedBytes < 8)
					{
						HelperFunctions::printError("Error: RPC client received binary packet smaller than 8 bytes from server " + server->ipAddress + " on port " + server->address.second);
						if(!server->keepAlive) closeConnection(server);
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
					HelperFunctions::printDebug("RPC client receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
					if(dataSize == 0)
					{
						HelperFunctions::printError("Error: RPC client received binary packet without data from server " + server->ipAddress + " on port " + server->address.second);
						if(!server->keepAlive) closeConnection(server);
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					if(dataSize > 104857600)
					{
						HelperFunctions::printError("Error: RPC client received packet with data larger than 100 MiB received.");
						if(!server->keepAlive) closeConnection(server);
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					packetLength = receivedBytes - 8;
					packet->insert(packet->begin(), buffer, buffer + receivedBytes);
				}
				else
				{
					if(packetLength + receivedBytes > dataSize)
					{
						HelperFunctions::printError("Error: RPC client received response packet larger than the expected data size.");
						if(!server->keepAlive) closeConnection(server);
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					packet->insert(packet->end(), buffer, buffer + receivedBytes);
					packetLength += receivedBytes;
					if(packetLength == dataSize)
					{
						packet->push_back('\0');
						break;
					}
				}
			}
			else
			{
				try
				{
					http.process(buffer, receivedBytes);
				}
				catch(HTTPException& ex)
				{
					HelperFunctions::printError("XML RPC Client: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
					if(!server->keepAlive) closeConnection(server);
					_sendCounter--;
					return std::shared_ptr<std::vector<char>>();
				}
				if(http.getContentSize() > 104857600 || http.getHeader()->contentLength > 104857600)
				{
					HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
					if(!server->keepAlive) closeConnection(server);
					_sendCounter--;
					return std::shared_ptr<std::vector<char>>();
				}
			}
		}
		if(!server->keepAlive) closeConnection(server);
		HelperFunctions::printDebug("Debug: Received packet from server " + server->ipAddress + " on port " + server->address.second + ":\n" + HelperFunctions::getHexString(*http.getContent()));
		_sendCounter--;
		if(server->binary) return packet;
		else if(http.isFinished()) return http.getContent();
		else return std::shared_ptr<std::vector<char>>();;
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
    if(server && server->fileDescriptor > -1)
    {
    	shutdown(server->fileDescriptor, 0);
		close(server->fileDescriptor);
		server->fileDescriptor = -1;
    }
    if(server && server->ssl) SSL_free(server->ssl);
    _sendCounter--;
    return std::shared_ptr<std::vector<char>>();
}
} /* namespace RPC */
