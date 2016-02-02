/* Copyright 2013-2016 Sathya Laufer
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

#ifndef CLICLIENT_H_
#define CLICLIENT_H_

#include "homegear-base/BaseLib.h"

#include <thread>
#include <mutex>
#include <string>

namespace CLI {

class Client {
public:
	Client();
	virtual ~Client();

	/**
	 * Starts the CLI client.
	 *
	 * @param command A CLI command to execute. After this command is executed the function returns. If "command" is empty the function listens for input until "exit" or "quit" is entered.
	 * @return Returns the exit code. If a script is executed the script exit code is returned.
	 */
	int32_t start(std::string command = "");
private:
	std::string _socketPath;
	std::shared_ptr<BaseLib::FileDescriptor> _fileDescriptor;
	bool _stopPingThread = false;
	std::thread _pingThread;
	bool _closed = false;
	std::mutex _sendMutex;

	void ping();
};

}
#endif
