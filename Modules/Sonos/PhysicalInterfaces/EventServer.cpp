/* Copyright 2013-2015 Sathya Laufer
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

#include "EventServer.h"
#include "../GD.h"
#include "../../Base/Encoding/RapidXml/rapidxml.hpp"
#include <ifaddrs.h>

namespace Sonos
{
EventServer::EventServer(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : ISonosInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "Event server \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	if(!settings)
	{
		_out.printCritical("Critical: Error initializing. Settings pointer is empty.");
		return;
	}
	_listenPort = BaseLib::Math::getNumber(settings->port);
	if(_listenPort <= 0  || _listenPort > 65535) _listenPort = 7373;
}

EventServer::~EventServer()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void EventServer::getAddress()
{
	try
	{
		std::string address;
		if(_settings->host.empty())
		{
			char buffer[101];
			buffer[100] = 0;
			bool addressFound = false;
			ifaddrs* interfaces = nullptr;
			if(getifaddrs(&interfaces) != 0)
			{
				_bl->out.printCritical("Error: Could not get address information: " + std::string(strerror(errno)));
				return;
			}
			for(ifaddrs* info = interfaces; info != 0; info = info->ifa_next)
			{
				if (info->ifa_addr == NULL) continue;
				switch (info->ifa_addr->sa_family)
				{
					case AF_INET:
						inet_ntop (info->ifa_addr->sa_family, &((struct sockaddr_in *)info->ifa_addr)->sin_addr, buffer, 100);
						address = std::string(buffer);
						if(address.compare(0, 3, "10.") == 0 || address.compare(0, 4, "172.") == 0 || address.compare(0, 8, "192.168.") == 0) addressFound = true;
						break;
					case AF_INET6:
						//Ignored currently
						inet_ntop (info->ifa_addr->sa_family, &((struct sockaddr_in6 *)info->ifa_addr)->sin6_addr, buffer, 100);
						break;
				}
				if(addressFound)
				{
					_listenAddress = address;
					break;
				}
			}
			freeifaddrs(interfaces);
			if(!addressFound)
			{
				_bl->out.printError("Error: No IP address could be found to bind the server to. Please specify the IP address manually in main.conf.");
				return;
			}
		}
		else _listenAddress = _settings->host;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventServer::startListening()
{
	try
	{
		stopListening();
		getAddress();
		if(_listenAddress.empty())
		{
			GD::out.printError("Error: Could not get listen automatically. Please specify it in physicalinterfaces.conf");
			return;
		}
		_stopServer = false;
		_listenThread = std::thread(&EventServer::mainThread, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void EventServer::stopListening()
{
	try
	{
		if(_stopServer) return;
		_stopServer = true;
		if(_listenThread.joinable()) _listenThread.join();

		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void EventServer::mainThread()
{
    try
    {
    	getSocketDescriptor();

        while(!_stopServer)
        {
			try
			{
				if(!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1)
				{
					if(_stopServer) break;
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					getSocketDescriptor();
					continue;
				}
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = getClientSocketDescriptor();
				if(!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;

				std::shared_ptr<BaseLib::SocketOperations> socket(new BaseLib::SocketOperations(GD::bl, clientFileDescriptor));
				readClient(socket);
				GD::bl->fileDescriptorManager.shutdown(clientFileDescriptor);
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
}

void EventServer::readClient(std::shared_ptr<BaseLib::SocketOperations> socket)
{
	try
	{
		if(!socket) return;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		//Make sure the buffer is null terminated.
		buffer[bufferMax] = '\0';
		std::vector<char> packet;
		int32_t bytesRead;
		BaseLib::HTTP http;

		while(!_stopServer)
		{
			try
			{
				bytesRead = socket->proofread(buffer, bufferMax);
				buffer[bufferMax] = 0; //Even though it shouldn't matter, make sure there is a null termination.
				//Some clients send only one byte in the first packet
				if(!http.headerProcessingStarted() && bytesRead == 1) bytesRead += socket->proofread(&buffer[1], bufferMax - 1);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				_out.printWarning("Warning: Connection timed out.");
				break;
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_out.printInfo("Info: " + ex.what());
				break;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_out.printError(ex.what());
				break;
			}

			if(GD::bl->debugLevel >= 5)
			{
				std::vector<uint8_t> rawPacket(buffer, buffer + bytesRead);
				_out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
			}
			buffer[bytesRead] = '\0';
			if(!http.headerProcessingStarted() && (!strncmp(&buffer[0], "NOTIFY", 6) || !strncmp(&buffer[0], "HTTP/1.", 7))) http.reset();
			else if(!http.headerProcessingStarted())
			{
				_out.printError("Error: Uninterpretable packet received. Closing connection. Packet was: " + std::string(buffer, bytesRead));
				break;
			}

			try
			{
				http.process(buffer, bytesRead);
			}
			catch(BaseLib::HTTPException& ex)
			{
				_out.printError("Error: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
				http.reset();
			}

			if(http.getContentSize() > 10485760)
			{
				_out.printError("Error: Received HTTP packet larger than 10MiB.");
				http.reset();
			}

			if(http.isFinished())
			{
				BaseLib::HTTP::Header* header = http.getHeader();
				std::string serialNumber;
				if(header->fields.find("sid") != header->fields.end())
				{
					serialNumber = header->fields.at("sid");
					if(serialNumber.size() > 24) serialNumber = serialNumber.substr(12, 12); else serialNumber.clear();
				}
				if(http.getContentSize() > 0 && !serialNumber.empty())
				{
					xml_document<> doc;
					doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(&http.getContent()->at(0));
					for(xml_node<>* node = doc.first_node(); node; node = node->next_sibling())
					{
						std::string name(node->name());
						if(name == "e:propertyset")
						{
							for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
							{
								std::string subNodeName(subNode->name());
								if(subNodeName == "e:property")
								{
									for(xml_node<>* propertyNode = subNode->first_node(); propertyNode; propertyNode = propertyNode->next_sibling())
									{
										std::string propertyName(propertyNode->name());
										if(propertyName == "LastChange")
										{
											std::string value(propertyNode->value());
											std::string xml;
											BaseLib::Html::unescapeHtmlEntities(value, xml);
											std::shared_ptr<SonosPacket> packet(new SonosPacket(xml, serialNumber, BaseLib::HelperFunctions::getTime()));
											raisePacketReceived(packet);
										}
										else _out.printWarning("Unknown element in \"e:property\": " + name);
									}
								}
								else _out.printWarning("Unknown element in \"e:propertyset\": " + name);
							}
						}
						else _out.printWarning("Unknown root element: " + name);
					}
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
				}
				else _out.printWarning("Warning: Packet without content or serial number received.");
				break;
			}
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void EventServer::getSocketDescriptor()
{
	try
	{
		addrinfo hostInfo;
		addrinfo *serverInfo = nullptr;

		int32_t yes = 1;

		memset(&hostInfo, 0, sizeof(hostInfo));

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;
		hostInfo.ai_flags = AI_PASSIVE;
		char buffer[100];
		std::string port = std::to_string(_listenPort);
		int32_t result;
		if((result = getaddrinfo(_listenAddress.c_str(), port.c_str(), &hostInfo, &serverInfo)) != 0)
		{
			_out.printCritical("Error: Could not get address information: " + std::string(gai_strerror(result)));
			return;
		}

		bool bound = false;
		int32_t error = 0;
		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next)
		{
			_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(info->ai_family, info->ai_socktype, info->ai_protocol));
			if(_serverFileDescriptor->descriptor == -1) continue;
			if(!(fcntl(_serverFileDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(_serverFileDescriptor->descriptor, F_SETFL, fcntl(_serverFileDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0) throw BaseLib::Exception("Error: Could not set socket options.");
			}
			if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw BaseLib::Exception("Error: Could not set socket options.");
			if(bind(_serverFileDescriptor->descriptor, info->ai_addr, info->ai_addrlen) == -1)
			{
				error = errno;
				continue;
			}
			switch (info->ai_family)
			{
				case AF_INET:
					inet_ntop (info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, buffer, 100);
					break;
				case AF_INET6:
					inet_ntop (info->ai_family, &((struct sockaddr_in6 *) info->ai_addr)->sin6_addr, buffer, 100);
					break;
			}
			_out.printInfo("Info: Started listening on address " + _listenAddress + " and port " + port);
			bound = true;
			break;
		}
		freeaddrinfo(serverInfo);
		if(!bound)
		{
			GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
			_out.printCritical("Error: Could not start listening on port " + port + ": " + std::string(strerror(error)));
			return;
		}
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backLog) == -1)
		{
			GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
			_out.printCritical("Error: Could not start listening on port " + port + ": " + std::string(strerror(errno)));
			return;
		}
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::FileDescriptor> EventServer::getClientSocketDescriptor()
{
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	try
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		GD::bl->fileDescriptorManager.lock();
		int32_t nfds = _serverFileDescriptor->descriptor + 1;
		if(nfds <= 0)
		{
			GD::bl->fileDescriptorManager.unlock();
			GD::out.printError("Error: Server file descriptor is invalid.");
			return fileDescriptor;
		}
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		GD::bl->fileDescriptorManager.unlock();
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout)) return fileDescriptor;

		struct sockaddr_storage clientInfo;
		socklen_t addressSize = sizeof(addressSize);
		fileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientInfo, &addressSize));
		if(!fileDescriptor) return fileDescriptor;

		getpeername(fileDescriptor->descriptor, (struct sockaddr*)&clientInfo, &addressSize);

		int32_t port = -1;
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

		_out.printInfo("Info: Connection from " + std::string(&ipString[0]) + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(fileDescriptor->id));
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return fileDescriptor;
}
}
