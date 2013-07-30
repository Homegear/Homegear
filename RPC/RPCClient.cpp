#include "RPCClient.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace RPC
{
void RPCClient::invokeBroadcast(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	if(methodName.empty())
	{
		if(GD::debugLevel >= 2) std::cout << "Error: Could not invoke XML RPC method for server " << server << ". methodName is empty." << std::endl;
		return;
	}
	std::string result = sendRequest(server, port, _xmlRpcEncoder.encodeRequest(methodName, parameters));
	if(result.empty())
	{
		if(GD::debugLevel >= 2) std::cout << "Error: Response for XML RPC method " << methodName << " sent to server " << server << " on port " << port << " is empty." << std::endl;
		return;
	}
	std::shared_ptr<RPCVariable> returnValue = _xmlRpcDecoder.decodeResponse(result);
	if(returnValue->errorStruct && GD::debugLevel >= 2)
	{
		if(returnValue->structValue->size() == 2)
		{
			std::cout << "Error reading response from XML RPC server " << server << " on port " << port << ". Fault code: " << returnValue->structValue->at(0)->integerValue << ". Fault string: " << returnValue->structValue->at(1)->stringValue << std::endl;
		}
		else
		{
			std::cout << "Error reading response from XML RPC server " << server << " on port " << port << ". Can't print fault struct. It is not well formed." << std::endl;
		}
	}
}

std::shared_ptr<RPCVariable> RPCClient::invoke(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	if(methodName.empty()) return RPCVariable::createError(-32601, "Method name is empty");
	std::string result = sendRequest(server, port, _xmlRpcEncoder.encodeRequest(methodName, parameters));
	if(result.empty()) return RPCVariable::createError(-32700, "No response data.");
	return _xmlRpcDecoder.decodeResponse(result);
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

		int32_t fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if(fileDescriptor == -1)
		{
			if(GD::debugLevel >= 2) std::cout << "Could not create socket for XML RPC server " << server << " on port " << port << std::endl;
			freeaddrinfo(serverInfo);
			return "";
		}

		if(connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not connect to XML RPC server " << server << " on port " << port << std::endl;
			freeaddrinfo(serverInfo);
			close(fileDescriptor);
			return "";
		}

		freeaddrinfo(serverInfo);

		std::string msg("POST /RPC2 HTTP/1.1\nUser_Agent: Homegear\nHost: " + server + ":" + port + "\nContent-Type: text/xml\nContent-Length: " + std::to_string(data.length()) + "\n\n" + data + "\n");
		if(GD::debugLevel >= 5) std::cout << "Sending packet: " << msg << std::endl;
		int32_t sentBytes = send(fileDescriptor, msg.c_str(), msg.size(), 0);
		if(sentBytes == 0)
		{
			if(GD::debugLevel >= 2) std::cout << "Error while sending data to XML RPC server " << server << " on port " << port << std::endl;
			close(fileDescriptor);
			return "";
		}

		ssize_t receivedBytes;

		struct timeval timeout;
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;
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
			receivedBytes = select(fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(receivedBytes)
			{
				case 0:
					if(GD::debugLevel >= 2) std::cout << "Error: Reading from XML RPC server " << server << " on port " << port << " timed out." << std::endl;
					break;
				case -1:
					if(GD::debugLevel >= 2) std::cout << "Error reading from XML RPC server " << server << " on port " << port << "." << std::endl;
					break;
				case 1:
					break;
				default:
					if(GD::debugLevel >= 2) std::cout << "Error reading from XML RPC server " << server << " on port " << port << "." << std::endl;
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
				if(GD::debugLevel >= 2) std::cout << "Error reading data from XML RPC server " << server << " on port " << port << "." << std::endl;
				shutdown(fileDescriptor, 0);
				close(fileDescriptor);
				return "";
			}
			if(firstPart)
			{
				if(receivedBytes < 50)
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Response packet from XML RPC server too small " << server << " on port " << port << "." << std::endl;
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
					if(GD::debugLevel >= 2) std::cout << "Error receiving response from XML RPC server " << server << " on port " << port << ". Response code: " << line << std::endl;
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
							if(GD::debugLevel >= 2) std::cout << "Error receiving response from XML RPC server " << server << " on port " << port << ". Content type is not text/xml but " << line.substr(14, 8) << "." << std::endl;
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
							if(GD::debugLevel >= 2) std::cout << "Error receiving response from XML RPC server " << server << " on port " << port << ". Content length is 0." << std::endl;
							shutdown(fileDescriptor, 0);
							close(fileDescriptor);
							return "";
						}
						if(GD::debugLevel >= 5) std::cout << "Receiving packet with length " << dataSize << " from remote server " << server << " on port " << port << "." << std::endl;
					}
					else if(contentType && line.size() > 5 && line.substr(0, 5) == "<?xml")
					{
						uint32_t pos = (int32_t)receivedData.tellg() - line.length() - 1; //Don't count the new line after the header
						int32_t restLength = receivedBytes - pos;
						if(restLength < 1)
						{
							if(GD::debugLevel >= 2) std::cout << "Error: No data in XML RPC packet." << std::endl;
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
					if(GD::debugLevel >= 2) std::cout << "Error: Packet length is wrong." << std::endl;
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
		if(GD::debugLevel >= 5) std::cout << "Debug: Received packet from server " << server << " on port " << port << ": " << std::endl << response << std::endl;
		return response;
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
    return "";
}
} /* namespace RPC */
