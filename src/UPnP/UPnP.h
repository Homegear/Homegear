/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef UPNP_H_
#define UPNP_H_

#include "homegear-base/BaseLib.h"

class UPnP : public BaseLib::Rpc::IWebserverEventSink
{
public:
	UPnP();
	virtual ~UPnP();

	void start();
	void stop();
	void getDescription(int32_t port, std::vector<char>& output);
private:
	struct Packets
	{
		std::vector<char> notifyRoot;
		std::vector<char> notifyRootUUID;
		std::vector<char> notify;
		std::vector<char> okRoot;
		std::vector<char> okRootUUID;
		std::vector<char> ok;
		std::vector<char> byebyeRoot;
		std::vector<char> byebyeRootUUID;
		std::vector<char> byebye;
		std::vector<char> description;
	};

	BaseLib::Output _out;
	bool _stopServer = true;
	std::shared_ptr<BaseLib::FileDescriptor> _serverSocketDescriptor;
	std::thread _listenThread;
	std::string _address;
	std::string _udn;
	std::string _st;
	std::map<int32_t, Packets> _packets;
	int32_t _lastAdvertisement;

	// {{{ Webserver events
		BaseLib::PEventHandler _webserverEventHandler;

		bool onGet(BaseLib::Rpc::PServerInfo& serverInfo, BaseLib::HTTP& httpRequest, std::shared_ptr<BaseLib::SocketOperations>& socket, std::string& path);
	// }}}

	void getAddress();
	void getUDN();
	void getSocketDescriptor();
	void listen();
	void processPacket(BaseLib::HTTP& http);
	void sendOK(std::string destinationIpAddress, int32_t destinationPort, bool rootDeviceOnly);
	void sendNotify();
	void sendByebye();
	void registerServers();
};

#endif
