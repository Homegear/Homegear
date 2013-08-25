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
		std::string result = sendRequest(server, port, _xmlRpcEncoder.encodeRequest(methodName, parameters));
		if(result.empty())
		{
			HelperFunctions::printError("Error: Response for XML RPC method " + methodName + " sent to server " + server + " on port " + port + " is empty.");
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
		struct addrinfo hostInfo;
		struct addrinfo *serverInfo;

		memset(&hostInfo, 0, sizeof hostInfo);

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;

		std::string ipAddress = server;
		if(ipAddress.size() < 8) throw Exception("Error: Server's address too short: " + ipAddress);
		if(ipAddress.substr(0, 7) == "http://") ipAddress = ipAddress.substr(7);

		if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0) throw Exception("Error: Could not get address information.");
		if(GD::settings.tunnelClients().find(ipAddress) != GD::settings.tunnelClients().end())
		{
			if(serverInfo->ai_family == AF_INET6) ipAddress = "::1";
			else ipAddress = "127.0.0.1";
			if(getaddrinfo(ipAddress.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0) throw Exception("Error: Could not get address information.");
		}


		int32_t fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if(fileDescriptor == -1)
		{
			HelperFunctions::printError("Could not create socket for XML RPC server " + server + " on port " + port);
			freeaddrinfo(serverInfo);
			return "";
		}

		if(connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
		{
			HelperFunctions::printError("Error: Could not connect to XML RPC server " + server + " on port " + port);
			freeaddrinfo(serverInfo);
			close(fileDescriptor);
			return "";
		}

		freeaddrinfo(serverInfo);

		std::string msg("POST /RPC2 HTTP/1.1\nUser_Agent: Homegear\nHost: " + server + ":" + port + "\nContent-Type: text/xml\nContent-Length: " + std::to_string(data.length()) + "\n\n" + data + "\n");
		HelperFunctions::printDebug("Sending packet: " + msg);
		int32_t sentBytes = send(fileDescriptor, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		if(sentBytes == 0)
		{
			HelperFunctions::printError("Error sending data to XML RPC server " + server + " on port " + port);
			close(fileDescriptor);
			return "";
		}

		ssize_t receivedBytes;

		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(fileDescriptor, &readFileDescriptor);
		char buffer[1024];
		bool receiving = true;
		bool firstPart = true;
		int32_t lineBufferMax = 128;
		char lineBuffer[lineBufferMax];
		uint32_t dataSize = 0;
		std::string response;
		while(receiving)
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 10;
			timeout.tv_usec = 0;
			receivedBytes = select(fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(receivedBytes)
			{
				case 0:
					HelperFunctions::printError("Error: Reading from XML RPC server " + server + " on port " + port + " timed out.");
					break;
				case -1:
					HelperFunctions::printError("Error reading from XML RPC server " + server + " on port " + port + ".");
					break;
				case 1:
					break;
				default:
					HelperFunctions::printError("Error reading from XML RPC server " + server + " on port " + port + ".");
			}
			if(receivedBytes != 1)
			{
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				return "";
			}
			receivedBytes = recv(fileDescriptor, buffer, 1024, 0);
			if(receivedBytes < 0)
			{
				HelperFunctions::printError("Error reading data from XML RPC server " + server + " on port " + port + ".");
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				return "";
			}
			if(firstPart)
			{
				if(receivedBytes < 50)
				{
					HelperFunctions::printError("Error: Response packet from XML RPC server too small " + server + " on port " + port + ".");
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					return "";
				}
				std::stringstream receivedData;
				receivedData.write(buffer, receivedBytes);
				bool contentType = false;
				receivedData.getline(lineBuffer, lineBufferMax);
				std::string line(lineBuffer);
				HelperFunctions::toLower(line);
				if(line.size() < 15 || line.substr(0, 6) != "http/1" || line.substr(9, 6) != "200 ok")
				{
					HelperFunctions::printError("Error receiving response from XML RPC server " + server + " on port " + port + ". Response code: " + line);
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					return "";
				}
				while(receivedData.good() && !receivedData.eof())
				{
					receivedData.getline(lineBuffer, lineBufferMax);
					line = std::string(lineBuffer);
					HelperFunctions::toLower(line);
					if(line.size() > 14 && line.substr(0, 13) == "content-type:")
					{
						contentType = true;
						if(line.substr(14, 8) != "text/xml")
						{
							HelperFunctions::printError("Error receiving response from XML RPC server " + server + " on port " + port + ". Content type is not text/xml but " + line.substr(14, 8) + ".");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							return "";
						}
					}
					else if(line.size() > 16 && line.substr(0, 15) == "content-length:")
					{
						try	{ dataSize = std::stoll(line.substr(16)); } catch(...) {}
						if(dataSize == 0)
						{
							HelperFunctions::printError("Error receiving response from XML RPC server " + server + " on port " + port + ". Content length is 0.");
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							return "";
						}
						HelperFunctions::printDebug("Debug: Receiving packet with length " + std::to_string(dataSize) + " from remote server " + server + " on port " + port + ".");
					}
					else if(contentType && line.size() > 5 && line.substr(0, 5) == "<?xml")
					{
						uint32_t pos = (int32_t)receivedData.tellg() - line.length() - 1; //Don't count the new line after the header
						int32_t restLength = receivedBytes - pos;
						if(restLength < 1)
						{
							HelperFunctions::printError("Error: No data in XML RPC packet.");
							break;
						}
						if((unsigned)restLength >= dataSize)
						{
							response.append(buffer + pos, dataSize);
							break;
						}
						else response.append(buffer + pos, restLength);
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
				if(response.size() + receivedBytes > dataSize)
				{
					HelperFunctions::printError("Error: Packet length is wrong.");
					shutdown(fileDescriptor, 0);
					close(fileDescriptor);
					return "";
				}
				response.append(buffer, receivedBytes);
				if(response.size() == dataSize) break;
			}
		}
		shutdown(fileDescriptor, 0);
		close(fileDescriptor);
		HelperFunctions::printDebug("Debug: Received packet from server " + server + " on port " + port + ":\n" + response);
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
    return "";
}
} /* namespace RPC */
