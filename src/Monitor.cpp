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

#include "Monitor.h"
#include "GD/GD.h"

Monitor::Monitor()
{
	signal(SIGPIPE, SIG_IGN);
}

Monitor::~Monitor()
{
	dispose();
}

void Monitor::init()
{
	/*if(pipe(_pipeFromChild) == -1)
	{
		GD::out.printError("Error creating pipe to child.");
	}*/
	if(pipe(_pipeToChild) == -1)
	{
		GD::out.printError("Error creating pipe to child.");
	}
}

void Monitor::prepareParent()
{
	//close(_pipeToChild[1]);
	//close(_pipeFromChild[0]);
	/*if(!(fcntl(_pipeToChild[0], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeToChild[0], F_SETFL, fcntl(_pipeToChild[0], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not set pipe to child process to non blocking.");
		}
	}*/
	/*if(!(fcntl(_pipeFromChild[1], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeFromChild[1], F_SETFL, fcntl(_pipeFromChild[1], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not set pipe from child process to non blocking.");
		}
	}*/
}

void Monitor::prepareChild()
{
	//close(_pipeToChild[0]);
	//close(_pipeFromChild[1]);
	if(!(fcntl(_pipeToChild[1], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeToChild[1], F_SETFL, fcntl(_pipeToChild[1], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not start monitor thread, because the pipe would be blocking.");
			return;
		}
	}
	/*if(!(fcntl(_pipeFromChild[0], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeFromChild[0], F_SETFL, fcntl(_pipeFromChild[0], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not start monitor thread, because the pipe would be blocking.");
			return;
		}
	}*/
	stop();
	_stopMonitorThread = false;
	_monitorThread = std::thread(&Monitor::monitor, this);
}

void Monitor::stop()
{
	try
	{
		_stopMonitorThread = true;
		if(_monitorThread.joinable()) _monitorThread.join();
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void Monitor::dispose()
{
	if(_disposing) return;
	_disposing = true;
	stop();
}

void Monitor::checkHealth()
{
	try
	{
		uint32_t totalBytesWritten = 0;
		std::string command("Hallo Welt");
		while(totalBytesWritten < command.size())
		{
			ssize_t bytesWritten = write(_pipeToChild[0], command.c_str() + totalBytesWritten, command.size() - totalBytesWritten);
			if(bytesWritten == -1)
			{
				GD::out.printError(std::string("Error writing to child process pipe: ") + strerror(errno));
				return;
			}
			totalBytesWritten += bytesWritten;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void Monitor::monitor()
{
	try
	{
		std::vector<char> buffer(2048);
		while(!_stopMonitorThread)
		{
			fd_set readFileDescriptor;
			int32_t nfds = _pipeToChild[1] + 1;
			FD_SET(_pipeToChild[1], &readFileDescriptor);
			timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			int32_t bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue; //Timeout
			if(bytesRead != 1)
			{
				GD::out.printError("Error reading from parent's process pipe. Stopping monitoring.");
				return;
			}
			bytesRead = read(_pipeToChild[1], &buffer.at(0), buffer.size());
			if(bytesRead <= 0 || (unsigned)bytesRead > buffer.size())
			{
				if(bytesRead == -1) GD::out.printError(std::string("Error reading from parent's process pipe: ") + strerror(errno));
				else GD::out.printError("Error reading from parent's process pipe.");
				continue;
			}
			std::cerr << std::string(&buffer.at(0), bytesRead) << std::endl;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
