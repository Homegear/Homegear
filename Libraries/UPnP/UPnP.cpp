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

#include "UPnP.h"
#include "../GD/GD.h"
#include "../../Version.h"
#include <vector>
#include <ifaddrs.h>
#include <string.h>

UPnP::UPnP()
{
	try
	{
		_serverSocketDescriptor.reset(new BaseLib::FileDescriptor());
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

UPnP::~UPnP()
{
	try
	{
		stop();
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

void UPnP::start()
{
	try
	{
		_out.init(GD::bl.get());
		_out.setPrefix("UPnP Server: ");
		if(_packets.empty())
		{
			GD::out.printWarning("Warning: Not starting server, because no suitable RPC server for serving the XML description is available (Necessary settings: No SSL, no auth, webserver enabled).");
			return;
		}
		stop();
		_stopServer = false;
		_listenThread = std::thread(&UPnP::listen, this);
		sendNotify();
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

void UPnP::stop()
{
	try
	{
		_stopServer = true;
		if(_listenThread.joinable()) _listenThread.join();
		sendByebye();
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

void UPnP::getUDN()
{
	try
	{
		if(!GD::db.getHomegearVariableString(DatabaseController::HomegearVariables::upnpusn, _udn) || _udn.size() != 36)
		{
			_udn = BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(-2147483648, 2147483647), 8) + "-";
			_udn.append(BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(0, 65535), 4) + "-");
			_udn.append(BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(0, 65535), 4) + "-");
			_udn.append(BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(0, 65535), 4) + "-");
			_udn.append(BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(-2147483648, 2147483647), 8));
			_udn.append(BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(0, 65535), 4));
			GD::db.setHomegearVariableString(DatabaseController::HomegearVariables::upnpusn, _udn);
			GD::out.printInfo("Info: Created new UPnP UDN: " + _udn);
		}
		_st = "uuid:" + _udn;
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

void UPnP::getAddress()
{
	try
	{
		std::string address;
		if(GD::bl->settings.uPnPIpAddress().empty())
		{
			char buffer[101];
			buffer[100] = 0;
			bool addressFound = false;
			ifaddrs* interfaces = nullptr;
			if(getifaddrs(&interfaces) != 0)
			{
				GD::out.printCritical("Error: Could not get address information.");
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
				GD::out.printError("Error: No IP address could be found to bind the server to. Please specify the IP address manually in main.conf.");
				return;
			}
		}
		else _address = GD::bl->settings.uPnPIpAddress();
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

void UPnP::listen()
{
	try
	{
		getSocketDescriptor();
		_out.printInfo("Info: Started listening.");

		_lastAdvertisement = BaseLib::HelperFunctions::getTimeSeconds();
		char buffer[1024];
		int32_t bytesReceived = 0;
		struct sockaddr_in si_other;
		socklen_t slen = sizeof(si_other);
		fd_set readFileDescriptor;
		timeval timeout;
		int32_t nfds = 0;
		BaseLib::HTTP http;
		while(!_stopServer)
		{
			try
			{
				if(!_serverSocketDescriptor || _serverSocketDescriptor->descriptor == -1)
				{
					if(_stopServer) break;
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					getSocketDescriptor();
					continue;
				}

				timeout.tv_sec = 1;
				timeout.tv_usec = 0;
				FD_ZERO(&readFileDescriptor);
				GD::bl->fileDescriptorManager.lock();
				nfds = _serverSocketDescriptor->descriptor + 1;
				if(nfds <= 0)
				{
					GD::bl->fileDescriptorManager.unlock();
					_out.printError("Error: Socket closed (1).");
					GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
				}
				FD_SET(_serverSocketDescriptor->descriptor, &readFileDescriptor);
				GD::bl->fileDescriptorManager.unlock();
				bytesReceived = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
				if(bytesReceived == 0)
				{
					if(BaseLib::HelperFunctions::getTimeSeconds() - _lastAdvertisement >= 900)
					{
						sendNotify();
						_lastAdvertisement = BaseLib::HelperFunctions::getTimeSeconds();
					}
					continue;
				}
				if(bytesReceived != 1)
				{
					_out.printError("Error: Socket closed (2).");
					GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
				}

				bytesReceived = recvfrom(_serverSocketDescriptor->descriptor, buffer, 1024, 0, (struct sockaddr *)&si_other, &slen);
				if(bytesReceived <= 0) continue;
				http.reset();
				http.process(buffer, bytesReceived, false);
				if(http.isFinished()) processPacket(http);
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
	GD::bl->fileDescriptorManager.shutdown(_serverSocketDescriptor);
}

void UPnP::processPacket(BaseLib::HTTP& http)
{
	try
	{
		BaseLib::HTTP::Header* header = http.getHeader();
		if(!header || header->method != "M-SEARCH") return;

		std::string temp(&http.getRawHeader()->at(0), http.getRawHeader()->size());
		if(!header->host.empty() && header->fields.find("st") != header->fields.end() && (header->fields.at("st") == "urn:schemas-upnp-org:device:basic:1" || header->fields.at("st") == "ssdp:all" || header->fields.at("st") == "upnp:rootdevice" || header->fields.at("st") == _st))
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Discovery packet received from " + header->host);
			std::pair<std::string, std::string> address = BaseLib::HelperFunctions::split(header->host, ':');
			int32_t port = BaseLib::Math::getNumber(address.second, false);
			if(!address.first.empty() && port > 0)
			{
				int32_t mx = 5000;
				if(header->fields.find("mx") != header->fields.end()) mx = BaseLib::Math::getNumber(header->fields.at("mx"), false) * 1000;
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				//Wait for 0 to mx seconds for load balancing
				if(mx > 0)
				{
					GD::out.printDebug("Debug: Sleeping " + std::to_string(mx) + "ms before sending response.");
					std::this_thread::sleep_for(std::chrono::milliseconds(BaseLib::HelperFunctions::getRandomNumber(0, mx)));
				}
				sendOK(address.first, port, header->fields.at("st") == "upnp:rootdevice");
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

void UPnP::getDescription(int32_t port, std::vector<char>& output)
{
	if(_packets.find(port) != _packets.end()) output = _packets.at(port).description;
}

void UPnP::registerServer(int32_t port)
{
	try
	{
		if(_address.empty()) getAddress();
		if(_address.empty())
		{
			_out.printError("Error: Could not get IP address.");
			return;
		}
		if(_udn.empty()) getUDN();
		if(_udn.empty())
		{
			_out.printError("Error: Could not get UDN.");
			return;
		}
		Packets* packet = &_packets[port];
		std::string notifyPacketBase = "NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nCACHE-CONTROL: max-age=1800\r\nSERVER: Homegear " + std::string(VERSION) + "\r\nLOCATION: " + "http://" + _address + ":" + std::to_string(port) + "/description.xml\r\n";
		std::string alivePacketRoot = notifyPacketBase + "NT: upnp:rootdevice\r\nUSN: " + _st + "::upnp:rootdevice\r\nNTS: ssdp:alive\r\n\r\n";
		std::string alivePacketRootUUID = notifyPacketBase + "NT: " + _st + "\r\nUSN: " + _st + "\r\nNTS: ssdp:alive\r\n\r\n";
		std::string alivePacket = notifyPacketBase + "NT: urn:schemas-upnp-org:device:basic:1\r\nUSN: " + _st + "\r\nNTS: ssdp:alive\r\n\r\n";
		packet->notifyRoot = std::vector<char>(&alivePacketRoot.at(0), &alivePacketRoot.at(0) + alivePacketRoot.size());
		packet->notifyRootUUID = std::vector<char>(&alivePacketRootUUID.at(0), &alivePacketRootUUID.at(0) + alivePacketRootUUID.size());
		packet->notify = std::vector<char>(&alivePacket.at(0), &alivePacket.at(0) + alivePacket.size());

		std::string byebyePacketRoot = notifyPacketBase + "NT: upnp:rootdevice\r\nUSN: " + _st + "::upnp:rootdevice\r\nNTS: ssdp:byebye\r\n\r\n";
		std::string byebyePacketRootUUID = notifyPacketBase + "NT: " + _st + "\r\nUSN: " + _st + "\r\nNTS: ssdp:byebye\r\n\r\n";
		std::string byebyePacket = notifyPacketBase + "NT: urn:schemas-upnp-org:device:basic:1\r\nUSN: " + _st + "\r\nNTS: ssdp:byebye\r\n\r\n";
		packet->byebyeRoot = std::vector<char>(&byebyePacketRoot.at(0), &byebyePacketRoot.at(0) + byebyePacketRoot.size());
		packet->byebyeRootUUID = std::vector<char>(&byebyePacketRootUUID.at(0), &byebyePacketRootUUID.at(0) + byebyePacketRootUUID.size());
		packet->byebye = std::vector<char>(&byebyePacket.at(0), &byebyePacket.at(0) + byebyePacket.size());

		std::string okPacketBase = std::string("HTTP/1.1 200 OK\r\nCache-Control: max-age=1800\r\nExt: \r\nLocation: ") + "http://" + _address + ":" + std::to_string(port) + "/description.xml\r\nServer: Homegear " + std::string(VERSION) + "\r\n";
		std::string okPacketRoot = okPacketBase + "ST: upnp:rootdevice\r\nUSN: " + _st + "::upnp:rootdevice\r\n\r\n";
		std::string okPacketRootUUID = okPacketBase + "ST: " + _st + "\r\nUSN: " + _st + "\r\n\r\n";
		std::string okPacket = okPacketBase + "ST: urn:schemas-upnp-org:device:basic:1\r\nUSN: " + _st + "\r\n\r\n";
		packet->okRoot = std::vector<char>(&okPacketRoot.at(0), &okPacketRoot.at(0) + okPacketRoot.size());
		packet->okRootUUID = std::vector<char>(&okPacketRootUUID.at(0), &okPacketRootUUID.at(0) + okPacketRootUUID.size());
		packet->ok = std::vector<char>(&okPacket.at(0), &okPacket.at(0) + okPacket.size());

		std::string description = "<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>";
		description.append(std::string("<URLBase>") + "http://" + _address + ":" + std::to_string(port) + "</URLBase>");
		description.append("<device><deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType><friendlyName>Homegear</friendlyName><manufacturer>Homegear UG (haftungsbeschränkt)</manufacturer><manufacturerURL>http://homegear.eu</manufacturerURL>");
		description.append("<modelDescription>Homegear</modelDescription><modelName>Homegear</modelName><modelNumber>Homegear " + std::string(VERSION) + "</modelNumber><serialNumber>" + _udn + "</serialNumber><modelURL>http://homegear.eu</modelURL>");
		description.append("<UDN>uuid:" + _udn + "</UDN><presentationURL>" + "http://" + _address + ":" + std::to_string(port) + "</presentationURL></device></root>");
		packet->description = std::vector<char>(&description.at(0), &description.at(0) + description.size());
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

void UPnP::sendOK(std::string destinationIpAddress, int32_t destinationPort, bool rootDeviceOnly)
{
	try
	{
		if(!_serverSocketDescriptor || _serverSocketDescriptor->descriptor == -1 || _packets.empty()) return;
		struct sockaddr_in addessInfo;
		addessInfo.sin_family = AF_INET;
		addessInfo.sin_addr.s_addr = inet_addr(destinationIpAddress.c_str());
		addessInfo.sin_port = htons(destinationPort);

		for(std::map<int32_t, Packets>::iterator i = _packets.begin(); i != _packets.end(); ++i)
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Sending discovery response packets to " + destinationIpAddress + " on port " + std::to_string(destinationPort));
			sendto(_serverSocketDescriptor->descriptor, &i->second.okRoot.at(0), i->second.okRoot.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
			if(!rootDeviceOnly)
			{
				sendto(_serverSocketDescriptor->descriptor, &i->second.okRootUUID.at(0), i->second.okRootUUID.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
				sendto(_serverSocketDescriptor->descriptor, &i->second.ok.at(0), i->second.ok.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
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

void UPnP::sendNotify()
{
	try
	{
		if(!_serverSocketDescriptor || _serverSocketDescriptor->descriptor == -1 || _packets.empty()) return;
		struct sockaddr_in addessInfo;
		addessInfo.sin_family = AF_INET;
		addessInfo.sin_addr.s_addr = inet_addr("239.255.255.250");
		addessInfo.sin_port = htons(1900);

		for(std::map<int32_t, Packets>::iterator i = _packets.begin(); i != _packets.end(); ++i)
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Sending notify packets.");
			sendto(_serverSocketDescriptor->descriptor, &i->second.notifyRoot.at(0), i->second.notifyRoot.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
			sendto(_serverSocketDescriptor->descriptor, &i->second.notifyRootUUID.at(0), i->second.notifyRootUUID.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
			sendto(_serverSocketDescriptor->descriptor, &i->second.notify.at(0), i->second.notify.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
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

void UPnP::sendByebye()
{
	try
	{
		if(!_serverSocketDescriptor || _serverSocketDescriptor->descriptor == -1 || _packets.empty()) return;
		struct sockaddr_in addessInfo;
		addessInfo.sin_family = AF_INET;
		addessInfo.sin_addr.s_addr = inet_addr("239.255.255.250");
		addessInfo.sin_port = htons(1900);

		for(std::map<int32_t, Packets>::iterator i = _packets.begin(); i != _packets.end(); ++i)
		{
			if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Sending byebye packets.");
			sendto(_serverSocketDescriptor->descriptor, &i->second.byebyeRoot.at(0), i->second.byebyeRoot.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
			sendto(_serverSocketDescriptor->descriptor, &i->second.byebyeRootUUID.at(0), i->second.byebyeRootUUID.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
			sendto(_serverSocketDescriptor->descriptor, &i->second.byebye.at(0), i->second.byebye.size(), 0, (struct sockaddr*)&addessInfo, sizeof(addessInfo));
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

void UPnP::getSocketDescriptor()
{
	try
	{
		if(_address.empty()) return;
		_serverSocketDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_INET, SOCK_DGRAM, 0));
		if(_serverSocketDescriptor->descriptor == -1)
		{
			GD::out.printError("Error: Could not create socket.");
			return;
		}

		int32_t reuse = 1;
		setsockopt(_serverSocketDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

		GD::out.printInfo("Info: UPnP server: Binding to address: " + _address);

		char loopch = 0;
		setsockopt(_serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch));

		struct in_addr localInterface;
		localInterface.s_addr = inet_addr(_address.c_str());
		setsockopt(_serverSocketDescriptor->descriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));

		struct sockaddr_in localSock;
		memset((char *) &localSock, 0, sizeof(localSock));
		localSock.sin_family = AF_INET;
		localSock.sin_port = htons(1900);
		localSock.sin_addr.s_addr = inet_addr("239.255.255.250");

		if(bind(_serverSocketDescriptor->descriptor, (struct sockaddr*)&localSock, sizeof(localSock)) == -1)
		{
			GD::out.printError("Error: Binding to address " + _address + " failed: " + std::string(strerror(errno)));
			return;
		}

		struct ip_mreq group;
		group.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
		group.imr_interface.s_addr = inet_addr(_address.c_str());
		setsockopt(_serverSocketDescriptor->descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
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
