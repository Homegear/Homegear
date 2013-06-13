#include "RPCClient.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace RPC
{

void RPCClient::sendRequest(std::string server, std::string port, std::string data)
{
	try
	{
		struct addrinfo hostInfo;
		struct addrinfo *serverInfo;

		memset(&hostInfo, 0, sizeof hostInfo);

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;

		if(getaddrinfo(server.c_str(), port.c_str(), &hostInfo, &serverInfo) != 0) throw new Exception("Error: Could not get address information.");

		int32_t fileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if(fileDescriptor == -1)
		{
			if(GD::debugLevel >= 2) std::cout << "Could not create socket for XML RPC server " << server << " on port " << port << std::endl;
			freeaddrinfo(serverInfo);
			return;
		}

		if(connect(fileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not connect to XML RPC server " << server << " on port " << port << std::endl;
			freeaddrinfo(serverInfo);
			close(fileDescriptor);
			return;
		}
		freeaddrinfo(serverInfo);

		std::string msg("POST /RPC2 HTTP/1.1\nUser_Agent: Homegear\nHost: " + server + ":" + port + "\nContent-Type: text/xml\nContent-Length: " + std::to_string(data.length()) + "\n\n" + data + "\n");
		int32_t sentBytes = send(fileDescriptor, msg.c_str(), msg.size(), 0);
		if(sentBytes == 0)
		{
			if(GD::debugLevel >= 2) std::cout << "Error while sending data to XML RPC server " << server << " on port " << port << std::endl;
			close(fileDescriptor);
			return;
		}

		ssize_t receivedBytes;

		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(fileDescriptor, &readFileDescriptor);
		char buffer[1024];
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
		std::stringstream data;
		if(receivedBytes == 1)
		{
			receivedBytes = recv(fileDescriptor, buffer, 1024, 0);
			data.write(buffer, receivedBytes);
		}
		close(fileDescriptor);
		if(receivedBytes == -1)
		{
			if(GD::debugLevel >= 2) std::cout << "Error receiving data from XML RPC server " << server << " on port " << port << std::endl;
			close(fileDescriptor);
			return;
		}
		std::cout << data.str() << std::endl;
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

} /* namespace RPC */
