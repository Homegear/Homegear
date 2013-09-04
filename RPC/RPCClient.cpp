#include "RPCClient.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace RPC
{
void RPCClient::invokeBroadcast(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
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
		std::string result = sendRequest(server, port, _xmlRpcEncoder.encodeRequest(methodName, parameters));
		if(result.empty())
		{
			HelperFunctions::printWarning("Warning: Response is empty. XML RPC method: " + methodName + " Server: " + server + " Port: " + port);
			return;
		}
		std::shared_ptr<RPCVariable> returnValue = _xmlRpcDecoder.decodeResponse(result);
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

std::shared_ptr<RPCVariable> RPCClient::invoke(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
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
		std::string result = sendRequest(server, port, _xmlRpcEncoder.encodeRequest(methodName, parameters));
		if(result.empty()) return RPCVariable::createError(-32700, "No response data.");
		return _xmlRpcDecoder.decodeResponse(result);
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

std::string RPCClient::sendRequest(std::string server, std::string port, std::string data)
{
	try
	{
		_sendCounter++;
		if(_sendCounter > 100)
		{
			HelperFunctions::printCritical("Could not execute XML RPC method on server " + server + " and port " + port + ", because there are more than 100 requests queued. Your server is either not reachable currently or your connection is too slow.");
			_sendCounter--;
			return "";
		}

		std::string ipAddress = server;
		std::string ipString;
		if(ipAddress.size() < 8)
		{
			HelperFunctions::printError("Error: Server's address too short: " + ipAddress);
			_sendCounter--;
			return "";
		}
		if(ipAddress.substr(0, 7) == "http://") ipAddress = ipAddress.substr(7);

		int32_t fileDescriptor = -1;

		//Retry for two minutes
		for(uint32_t i = 0; i < 12; ++i)
		{
			struct addrinfo *serverInfo;
			struct addrinfo hostInfo;
			memset(&hostInfo, 0, sizeof hostInfo);

			hostInfo.ai_family = AF_UNSPEC;
			hostInfo.ai_socktype = SOCK_STREAM;

			if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
			{
				freeaddrinfo(serverInfo);
				throw Exception("Error: Could not get address information.");
			}

			if(GD::settings.tunnelClients().find(ipAddress) != GD::settings.tunnelClients().end())
			{
				if(serverInfo->ai_family == AF_INET6) ipAddress = "::1";
				else ipAddress = "127.0.0.1";
				if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0)
				{
					freeaddrinfo(serverInfo);
					throw Exception("Error: Could not get address information.");
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
				_sendCounter--;
				return "";
			}

			if(connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
			{
				if(i < 11)
				{
					HelperFunctions::printError("Warning: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ". Retrying in 5 seconds...");
					freeaddrinfo(serverInfo);
					close(fileDescriptor);
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				}
				else
				{
					HelperFunctions::printError("Error: Could not connect to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno) + ". Removing server. Server has to send init again.");
					freeaddrinfo(serverInfo);
					close(fileDescriptor);
					_sendCounter--;
					GD::rpcClient.removeServer(std::pair<std::string, std::string>(server, port));
					return "";
				}
			}
			else
			{
				freeaddrinfo(serverInfo);
				break;
			}
		}

		std::string msg("POST /RPC2 HTTP/1.1\nUser_Agent: Homegear\nHost: " + ipString + ":" + port + "\nContent-Type: text/xml\nContent-Length: " + std::to_string(data.length()) + "\n\n" + data + "\n");
		HelperFunctions::printDebug("Sending packet: " + msg);
		int32_t sentBytes = send(fileDescriptor, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		if(sentBytes == 0)
		{
			HelperFunctions::printError("Error sending data to XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
			close(fileDescriptor);
			_sendCounter--;
			return "";
		}

		ssize_t receivedBytes;

		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(fileDescriptor, &readFileDescriptor);
		int32_t bufferMax = 1024;
		char buffer[bufferMax];
		bool firstPart = true;
		bool chunked = false;
		int32_t lineBufferMax = 128;
		char lineBuffer[lineBufferMax];
		uint32_t dataSize = 0;
		std::string response;
		while(true)
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 10;
			timeout.tv_usec = 0;
			int64_t startTime = HelperFunctions::getTime();
			receivedBytes = select(fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(receivedBytes)
			{
				case 0:
					HelperFunctions::printWarning("Warning: Reading from XML RPC server timed out after " + std::to_string(HelperFunctions::getTime() - startTime) + " ms. Server: " + ipString + " Port: " + port + " Data: " + data);
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					_sendCounter--;
					return "";
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
				return "";
			}
			receivedBytes = recv(fileDescriptor, buffer, 1024, 0);
			if(receivedBytes < 0)
			{
				HelperFunctions::printError("Error reading data from XML RPC server " + ipString + " on port " + port + ": " + strerror(errno));
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				_sendCounter--;
				return "";
			}
			if(firstPart)
			{
				if(GD::debugLevel >= 5)
				{
					std::vector<uint8_t> rawPacket;
					rawPacket.insert(rawPacket.begin(), buffer, buffer + receivedBytes);
					HelperFunctions::printDebug("Debug: XML RPC client received packet: " + HelperFunctions::getHexString(rawPacket));
				}
				if(receivedBytes < 50)
				{
					HelperFunctions::printError("Error: Response packet from XML RPC server too small " + ipString + " on port " + port + ".");
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					_sendCounter--;
					return "";
				}
				std::stringstream receivedData;
				receivedData.write(buffer, receivedBytes);
				bool headerFinished = false;
				bool contentType = false;
				receivedData.getline(lineBuffer, lineBufferMax);
				std::string line(lineBuffer);
				uint32_t position = line.size() + 1;
				HelperFunctions::toLower(line);
				if(line.size() < 15 || line.substr(0, 6) != "http/1" || line.substr(9, 6) != "200 ok")
				{
					HelperFunctions::printError("Error receiving response from XML RPC server " + ipString + " on port " + port + ". Response code: " + line);
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					_sendCounter--;
					return "";
				}
				while(receivedData.good() && !receivedData.eof())
				{
					receivedData.getline(lineBuffer, lineBufferMax);
					line = std::string(lineBuffer);
					position += line.size() + 1;
					HelperFunctions::toLower(line);
					if(line.size() > 14 && line.substr(0, 13) == "content-type:")
					{
						contentType = true;
						std::string typeString = line.substr(14);
						HelperFunctions::trim(typeString);
						if(typeString != "text/xml")
						{
							std::string contentTypeString = line.substr(14);
							HelperFunctions::trim(contentTypeString);
							HelperFunctions::printError("Error receiving response from XML RPC server " + ipString + " on port " + port + ". Content type is not text/xml but " + contentTypeString + ".");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							_sendCounter--;
							return "";
						}
					}
					else if(line.size() > 16 && line.substr(0, 15) == "content-length:")
					{
						std::string sizeString = line.substr(16);
						HelperFunctions::trim(sizeString);
						try	{ dataSize = std::stoll(sizeString); } catch(...) {}
						if(dataSize == 0)
						{
							HelperFunctions::printError("Error receiving response from XML RPC server " + ipString + " on port " + port + ". Content length is 0.");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							_sendCounter--;
							return "";
						}
						HelperFunctions::printDebug("Debug: Receiving packet with length " + std::to_string(dataSize) + " from remote server " + ipString + " on port " + port + ".");
					}
					else if(line.size() > 19 && line.substr(0, 18) == "transfer-encoding:")
					{
						std::string transferEncoding = line.substr(19);
						HelperFunctions::trim(transferEncoding);
						if(transferEncoding == "chunked") chunked = true;
					}
					else if(line.size() <= 1)
					{
						if(!contentType)
						{
							std::string contentTypeString = line.substr(14);
							HelperFunctions::trim(contentTypeString);
							HelperFunctions::printError("Error receiving response from XML RPC server " + ipString + " on port " + port + ". No content type specified.");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							_sendCounter--;
							return "";
						}
						headerFinished = true;
					}
					else if(chunked && headerFinished && dataSize == 0)
					{
						std::string sizeString = line;
						HelperFunctions::trim(sizeString);
						try	{ dataSize = HelperFunctions::getNumber(sizeString, true); } catch(...) {}
						if(dataSize == 0)
						{
							HelperFunctions::printError("Error receiving response from XML RPC server " + ipString + " on port " + port + ". Content length is 0.");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							_sendCounter--;
							return "";
						}
					}
					else if(headerFinished)
					{
						if((!contentType && !chunked) || line.size() <= 5 || line.substr(0, 5) != "<?xml")
						{
							HelperFunctions::printWarning("Warning: XML RPC client received packet in unknown format.");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							_sendCounter--;
							return "";
						}
						if(dataSize > 104857600)
						{
							HelperFunctions::printError("Error: Packet with data larger than 100 MiB received.");
							break;
						}
						int32_t pos = position - (line.length() + 1);
						if(pos < 0)
						{
							HelperFunctions::printError("Error in XML RPC client: Could not calculate position in packet.");
							break;
						}
						int32_t restLength = receivedBytes - pos;
						if(restLength < 1)
						{
							HelperFunctions::printError("Error: No data in XML RPC packet.");
							break;
						}
						if((unsigned)restLength >= dataSize)
						{
							response.append(buffer + pos, dataSize);
							if(!chunked || (unsigned)restLength - dataSize == 7 && !strncmp(&buffer[pos + dataSize], "\r\n0\r\n\r\n", 7))
							{
								shutdown(fileDescriptor, 0);
								close(fileDescriptor);
								HelperFunctions::printDebug("Debug: Received packet from server " + ipString + " on port " + port + ":\n" + response);
								_sendCounter--;
								return response;
							}
						}
						else response.append(buffer + pos, restLength);
						break;
					}
				}
				firstPart = false;
			}
			else
			{
				if(response.size() + receivedBytes == dataSize + 1)
				{
					//For XML RPC packets the last byte is "\n", which is not counted in "Content-Length"
					receivedBytes -= 1;
				}
				if(chunked)
				{
					if(response.size() + receivedBytes <= dataSize)
					{
						response.append(buffer, receivedBytes);
						if(response.size() == dataSize) break;
					}
					else if((response.size() + receivedBytes) - dataSize == 7 && !strncmp(&buffer[receivedBytes - 7], "\r\n0\r\n\r\n", 7))
					{
						response.append(buffer, receivedBytes - 7);
						break;
					}
					else if(receivedBytes <= 7) //Shouldn't happen
					{
						std::string lastBytes(buffer, receivedBytes);
						HelperFunctions::trim(lastBytes);
						if(lastBytes.empty() || HelperFunctions::isNumber(lastBytes)) break;
						else
						{
							lastBytes = std::string(buffer, receivedBytes);
							HelperFunctions::rtrim(lastBytes); //Preserve left whitespaces
							response.append(lastBytes);
							break;
						}
					}
					else if(response.size() + receivedBytes > dataSize)
					{
						response.append(buffer, dataSize - response.size());
						break;
					}
				}
				else if(response.size() + receivedBytes > dataSize)
				{
					HelperFunctions::printError("Error: Packet size (>=" + std::to_string(response.size() + receivedBytes) + ") is larger than Content-Length (" + std::to_string(dataSize) + ").");
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					_sendCounter--;
					return "";
				}
				else
				{
					response.append(buffer, receivedBytes);
					if(response.size() == dataSize) break;
				}
			}
		}
		shutdown(fileDescriptor, 0);
		close(fileDescriptor);
		HelperFunctions::printDebug("Debug: Received packet from server " + ipString + " on port " + port + ":\n" + response);
		_sendCounter--;
		return response;
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
    return "";
}
} /* namespace RPC */
