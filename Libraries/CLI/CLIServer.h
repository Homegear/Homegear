/* Copyright 2013-2014 Sathya Laufer
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

#ifndef CLISERVER_H_
#define CLISERVER_H_

#include "../../Modules/Base/BaseLib.h"
#include "../User/User.h"
#include "../Systems/FamilyController.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

namespace CLI {

class ClientData
{
public:
	ClientData() { fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor); }
	ClientData(std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor) { fileDescriptor = clientFileDescriptor; }
	virtual ~ClientData() {}

	int32_t id = 0;
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
};

class Server {
public:
	Server();
	virtual ~Server();

	void start();
	void stop();
private:
	bool _stopServer = false;
	std::thread _mainThread;
	int32_t _backlog = 10;
	std::shared_ptr<BaseLib::FileDescriptor> _serverFileDescriptor;
	int32_t _maxConnections = 100;
	std::mutex _stateMutex;
	std::vector<std::shared_ptr<ClientData>> _fileDescriptors;
	std::vector<std::thread> _readThreads;
	int32_t _currentClientID = 0;

	void handleCommand(std::string& command, std::shared_ptr<ClientData> clientData);
	std::string handleUserCommand(std::string& command);
	std::string handleGlobalCommand(std::string& command);
	void getFileDescriptor(bool deleteOldSocket = false);
	std::shared_ptr<BaseLib::FileDescriptor> getClientFileDescriptor();
	void removeClientData(int32_t clientFileDescriptor);
	void mainThread();
	void readClient(std::shared_ptr<ClientData> clientData);
};

} /* namespace CLI */
#endif /* CLISERVER_H_ */
