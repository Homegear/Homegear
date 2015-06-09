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

#include "SSDP.h"
#include "../BaseLib.h"
#include <ifaddrs.h>

namespace BaseLib
{

SSDPInfo::SSDPInfo()
{

}

SSDPInfo::~SSDPInfo()
{

}

SSDP::SSDP(Obj* baseLib)
{
	_bl = baseLib;
	getAddress();
}

SSDP::~SSDP()
{
}

void SSDP::getAddress()
{
	try
	{
		std::string address;
		if(_bl->settings.uPnPIpAddress().empty())
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
					_address = address;
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
		else _address = _bl->settings.uPnPIpAddress();
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::shared_ptr<FileDescriptor> SSDP::getSocketDescriptor()
{
	std::shared_ptr<FileDescriptor> serverSocketDescriptor;
	try
	{
		if(_address.empty()) getAddress();
		if(_address.empty()) return serverSocketDescriptor;
		serverSocketDescriptor = _bl->fileDescriptorManager.add(socket(AF_INET, SOCK_DGRAM, 0));
		if(serverSocketDescriptor->descriptor == -1)
		{
			_bl->out.printError("Error: Could not create socket.");
			return serverSocketDescriptor;
		}

		int32_t reuse = 1;
		setsockopt(serverSocketDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

		_bl->out.printInfo("Info: UPnP server: Binding to address: " + _address);

		char loopch = 0;
		setsockopt(serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch));

		struct in_addr localInterface;
		localInterface.s_addr = inet_addr(_address.c_str());
		setsockopt(serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));

		struct sockaddr_in localSock;
		memset((char *) &localSock, 0, sizeof(localSock));
		localSock.sin_family = AF_INET;
		localSock.sin_port = htons(1900);
		localSock.sin_addr.s_addr = inet_addr(_address.c_str());

		if(bind(serverSocketDescriptor->descriptor, (struct sockaddr*)&localSock, sizeof(localSock)) == -1)
		{
			_bl->out.printError("Error: Binding to address " + _address + " failed: " + std::string(strerror(errno)));
			_bl->fileDescriptorManager.close(serverSocketDescriptor);
			return serverSocketDescriptor;
		}

		struct ip_mreq group;
		group.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
		group.imr_interface.s_addr = inet_addr(_address.c_str());
		setsockopt(serverSocketDescriptor->descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return serverSocketDescriptor;
}

void SSDP::sendSearchBroadcast(std::shared_ptr<FileDescriptor>& serverSocketDescriptor, const std::string& stHeader, uint32_t timeout)
{
	if(!serverSocketDescriptor || serverSocketDescriptor->descriptor == -1) return;
	struct sockaddr_in addessInfo;
	addessInfo.sin_family = AF_INET;
	addessInfo.sin_addr.s_addr = inet_addr("239.255.255.250");
	addessInfo.sin_port = htons(1900);

	if(timeout < 1000) timeout = 1000;

	std::string broadcastPacket("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: " + std::to_string(timeout / 1000) + "\r\nST: " + stHeader + "\r\nContent-Length: 0\r\n\r\n");

	sendto(serverSocketDescriptor->descriptor, &broadcastPacket.at(0), broadcastPacket.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
}

void SSDP::searchDevices(const std::string& stHeader, uint32_t timeout, std::vector<std::string>& ips)
{
	try
	{
		if(stHeader.empty())
		{
			_bl->out.printError("Error: Cannot search for SSDP devices. ST header is empty.");
			return;
		}

		std::shared_ptr<FileDescriptor> serverSocketDescriptor = getSocketDescriptor();
		if(!serverSocketDescriptor || serverSocketDescriptor->descriptor == -1) return;
		_bl->out.printInfo("Info: Searching for UPnP devices .");

		sendSearchBroadcast(serverSocketDescriptor, stHeader, timeout);

		uint64_t startTime = _bl->hf.getTime();
		char buffer[1024];
		int32_t bytesReceived = 0;
		struct sockaddr_in si_other;
		socklen_t slen = sizeof(si_other);
		fd_set readFileDescriptor;
		timeval socketTimeout;
		int32_t nfds = 0;
		HTTP http;
		while(_bl->hf.getTime() - startTime <= (timeout + 500))
		{
			try
			{
				if(!serverSocketDescriptor || serverSocketDescriptor->descriptor == -1) break;

				socketTimeout.tv_sec = timeout / 1000;
				socketTimeout.tv_usec = 500000;
				FD_ZERO(&readFileDescriptor);
				_bl->fileDescriptorManager.lock();
				nfds = serverSocketDescriptor->descriptor + 1;
				if(nfds <= 0)
				{
					_bl->fileDescriptorManager.unlock();
					_bl->out.printError("Error: Socket closed (1).");
					_bl->fileDescriptorManager.shutdown(serverSocketDescriptor);
				}
				FD_SET(serverSocketDescriptor->descriptor, &readFileDescriptor);
				_bl->fileDescriptorManager.unlock();
				bytesReceived = select(nfds, &readFileDescriptor, NULL, NULL, &socketTimeout);
				if(bytesReceived == 0) continue;
				if(bytesReceived != 1)
				{
					_bl->out.printError("Error: Socket closed (2).");
					_bl->fileDescriptorManager.shutdown(serverSocketDescriptor);
				}

				bytesReceived = recvfrom(serverSocketDescriptor->descriptor, buffer, 1024, 0, (struct sockaddr *)&si_other, &slen);
				if(bytesReceived <= 0) continue;
				if(_bl->debugLevel >= 5)_bl->out.printDebug("Debug: SSDP response received:\n" + std::string(buffer, bytesReceived));
				http.reset();
				http.process(buffer, bytesReceived, false);
				if(http.headerIsFinished()) processPacket(http, stHeader, ips);
			}
			catch(const std::exception& ex)
			{
				_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(Exception& ex)
			{
				_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void SSDP::processPacket(HTTP& http, const std::string& stHeader, std::vector<std::string>& ips)
{
	try
	{
		HTTP::Header* header = http.getHeader();
		if(!header || header->responseCode != 200 || header->fields.at("st") != stHeader) return;

		std::string ip = HelperFunctions::toLower(header->fields.at("location"));
		if(ip.size() < 7) return;
		std::string::size_type pos = ip.find(':', 7);
		if(pos == std::string::npos) return;
		ips.push_back(ip.substr(7, pos - 7));
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

}
