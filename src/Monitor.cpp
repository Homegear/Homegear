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

#include "Monitor.h"
#include "GD/GD.h"

Monitor::Monitor()
{
	signal(SIGPIPE, SIG_IGN);

	_pipeToChild[0] = 0;
	_pipeToChild[1] = 0;
	_pipeFromChild[0] = 0;
	_pipeFromChild[1] = 0;
}

Monitor::~Monitor()
{
	dispose();
}

bool Monitor::killedProcess()
{
	return _killedProcess;
}

void Monitor::init()
{
	if(!GD::bl->settings.enableMonitoring()) return;
	if(pipe(_pipeFromChild) == -1)
	{
		GD::out.printError("Error creating pipe from child.");
	}
	if(pipe(_pipeToChild) == -1)
	{
		GD::out.printError("Error creating pipe to child.");
	}
	if(!(fcntl(_pipeFromChild[0], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeFromChild[0], F_SETFL, fcntl(_pipeFromChild[0], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not set pipe from child process to non blocking.");
		}
	}
	if(!(fcntl(_pipeToChild[0], F_GETFL) & O_NONBLOCK))
	{
		if(fcntl(_pipeToChild[0], F_SETFL, fcntl(_pipeToChild[0], F_GETFL) | O_NONBLOCK) < 0)
		{
			GD::out.printError("Error: Could not start monitor thread, because the pipe would be blocking.");
			return;
		}
	}
}

void Monitor::prepareParent()
{
	if(!GD::bl->settings.enableMonitoring()) return;
	close(_pipeToChild[0]);
	close(_pipeFromChild[1]);
	_suspendMonitoring = false;
	_killedProcess = false;
}

void Monitor::prepareChild()
{
	if(!GD::bl->settings.enableMonitoring()) return;
	close(_pipeToChild[1]);
	close(_pipeFromChild[0]);
	stop();
	_stopMonitorThread = false;
	GD::bl->threadManager.start(_monitorThread, true, &Monitor::monitor, this);
}

void Monitor::stop()
{
	try
	{
		_stopMonitorThread = true;
		GD::bl->threadManager.join(_monitorThread);
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

void Monitor::killChild(pid_t mainProcessId)
{
	_suspendMonitoring = true;
	_killedProcess = true;
	GD::out.printCritical("Critical: Killing child process.");
	if(mainProcessId != 0) kill(mainProcessId, 9);
}

void Monitor::checkHealth(pid_t mainProcessId)
{
	try
	{
		if(!GD::bl->settings.enableMonitoring() || _suspendMonitoring) return;
		for(int32_t i = 0; i < 6; i++)
		{
			uint32_t totalBytesWritten = 0;
			std::string command("l");
			while(totalBytesWritten < command.size())
			{
				ssize_t bytesWritten = write(_pipeToChild[1], command.c_str() + totalBytesWritten, command.size() - totalBytesWritten);
				if(bytesWritten == -1)
				{
					if(_suspendMonitoring) return;
					GD::out.printError(std::string("Error writing to child process pipe: ") + strerror(errno));
					killChild(mainProcessId);
					return;
				}
				totalBytesWritten += bytesWritten;
			}

			for(int32_t i = 0; i < 100; i++)
			{
				char buffer = 0;
				int32_t bytesRead = read(_pipeFromChild[0], &buffer, 1);
				if(bytesRead <= 0 || (unsigned)bytesRead > 1)
				{
					if(bytesRead == -1)
					{
						if(errno != EAGAIN && errno != EWOULDBLOCK)
						{
							if(_suspendMonitoring) return;
							GD::out.printError(std::string("Error reading from child's process pipe: ") + strerror(errno));
							killChild(mainProcessId);
							return;
						}
					}
					else
					{
						if(_suspendMonitoring) return;
						GD::out.printInfo("Info: Pipe to child process closed.");
						std::this_thread::sleep_for(std::chrono::milliseconds(30000));
						//Normally this point is never reached (exit is called by main before)
						killChild(mainProcessId); //In case the pipe didn't close because of an ordered shutdown and the process hangs, kill it
						_suspendMonitoring = true;
						return;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}
				if(bytesRead != 1)
				{
					if(_suspendMonitoring) return;
					GD::out.printError("Error reading from child's process pipe: Too much data.");
					killChild(mainProcessId);
					return;
				}

				switch(buffer)
				{
				case 'a': //Everything ok
					if(GD::bl->debugLevel >= 6) GD::out.printDebug("Debug: checkHealth returned ok.");
					break;
				case 'n':
					killChild(mainProcessId);
					break;
				case 's':
					GD::out.printInfo("Info: Shutdown detected. Suspending monitoring.");
					_suspendMonitoring = true;
					break;
				}
				return;
			}
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
		char buffer = 0;
		int32_t bytesRead = 0;
		uint32_t totalBytesWritten = 0;
		while(!_stopMonitorThread)
		{
			bytesRead = read(_pipeToChild[0], &buffer, 1);
			if(bytesRead <= 0 || (unsigned)bytesRead > 1)
			{
				if(bytesRead == -1)
				{
					if(errno != EAGAIN && errno != EWOULDBLOCK) GD::out.printError(std::string("Error reading from parent's process pipe: ") + strerror(errno));
				}
				else GD::out.printError("Error reading from parent's process pipe.");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			if(bytesRead != 1)
			{
				GD::out.printError("Error reading from parent's process pipe: Too much data.");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			totalBytesWritten = 0;
			if(buffer == 'l')
			{
				std::string response;
				if(GD::bl->shuttingDown) response = "s";
				else
				{
					if(lifetick()) response = "a";
					else response = "n";
				}
				ssize_t bytesWritten = write(_pipeFromChild[1], response.c_str() + totalBytesWritten, response.size() - totalBytesWritten);
				if(bytesWritten == -1)
				{
					GD::out.printError(std::string("Error writing to child process pipe: ") + strerror(errno));
					return;
				}
				totalBytesWritten += bytesWritten;
			}
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

bool Monitor::lifetick()
{
	try
	{
		if(!GD::rpcClient->lifetick()) return false;
		for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
		{
			if(!i->second.lifetick()) return false;
		}
		if(!GD::familyController->lifetick()) return false;
		return true;
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
	return false;
}
