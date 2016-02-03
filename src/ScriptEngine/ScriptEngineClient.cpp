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

#include "ScriptEngineClient.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

ScriptEngineClient::ScriptEngineClient()
{
	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine: ");
}

ScriptEngineClient::~ScriptEngineClient()
{
	try
	{

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

void ScriptEngineClient::start()
{
	try
	{
		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.log").c_str(), "a", stdout))
		{
			_out.printError("Error: Could not redirect output to log file.");
		}
		if(!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.err").c_str(), "a", stderr))
		{
			_out.printError("Error: Could not redirect errors to log file.");
		}

		_socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
		for(int32_t i = 0; i < 2; i++)
		{
			_fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM, 0));
			if(!_fileDescriptor || _fileDescriptor->descriptor == -1)
			{
				_out.printError("Could not create socket.");
				return;
			}

			if(GD::bl->debugLevel >= 4 && i == 0) std::cout << "Info: Trying to connect..." << std::endl;
			sockaddr_un remoteAddress;
			remoteAddress.sun_family = AF_LOCAL;
			//104 is the size on BSD systems - slightly smaller than in Linux
			if(_socketPath.length() > 104)
			{
				//Check for buffer overflow
				_out.printCritical("Critical: Socket path is too long.");
				return;
			}
			strncpy(remoteAddress.sun_path, _socketPath.c_str(), 104);
			remoteAddress.sun_path[103] = 0; //Just to make sure it is null terminated.
			if(connect(_fileDescriptor->descriptor, (struct sockaddr*)&remoteAddress, strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family)) == -1)
			{
				GD::bl->fileDescriptorManager.shutdown(_fileDescriptor);
				if(i == 0)
				{
					_out.printDebug("Debug: Socket closed. Trying again...");
					//When socket was not properly closed, we sometimes need to reconnect
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					continue;
				}
				else
				{
					_out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
					return;
				}
			}
			else break;
		}
		if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Connected.");

		rl_bind_key('\t', rl_abort); //no autocompletion

		std::string level = "";
		std::string lastCommand;
		std::string currentCommand;
		char* sendBuffer;
		char receiveBuffer[1025];
		int32_t bytes = 0;
		/*while(!command.empty() || (sendBuffer = readline((level + "> ").c_str())) != NULL)
		{
			if(command.empty())
			{
				if(_closed) break;
				bytes = strlen(sendBuffer);
				if(sendBuffer[0] == '\n' || sendBuffer[0] == 0) continue;
				if(strncmp(sendBuffer, "quit", 4) == 0 || strncmp(sendBuffer, "exit", 4) == 0 || strncmp(sendBuffer, "moin", 4) == 0)
				{
					_closed = true;
					//If we close the socket, the socket file gets deleted. We don't want that
					//GD::bl->fileDescriptorManager.close(_fileDescriptor);
					free(sendBuffer);
					return 4;
				}
				_sendMutex.lock();
				if(_closed)
				{
					_sendMutex.unlock();
					break;
				}
				if(send(_fileDescriptor->descriptor, sendBuffer, bytes, MSG_NOSIGNAL) == -1)
				{
					_sendMutex.unlock();
					_out.printError("Error sending to socket.");
					//If we close the socket, the socket file gets deleted. We don't want that
					//GD::bl->fileDescriptorManager.close(_fileDescriptor);
					free(sendBuffer);
					return 5;
				}
				currentCommand = std::string(sendBuffer);
				if(currentCommand != lastCommand)
				{
					lastCommand = currentCommand;
					add_history(sendBuffer); //Sets sendBuffer to 0
				}
				else
				{
					free(sendBuffer);
					sendBuffer = 0;
				}
			}
			else
			{
				_sendMutex.lock();
				if(_closed)
				{
					_sendMutex.unlock();
					break;
				}
				if(send(_fileDescriptor->descriptor, command.c_str(), command.size(), MSG_NOSIGNAL) == -1)
				{
					_sendMutex.unlock();
					_out.printError("Error sending to socket.");
					return 6;
				}
			}

			while(true)
			{
				bytes = recv(_fileDescriptor->descriptor, receiveBuffer, 1024, 0);
				if(bytes > 0)
				{
					receiveBuffer[bytes] = 0;
					std::string response(receiveBuffer);
					if(response.size() > 15)
					{
						if(response.compare(7, 6, "family") == 0)
						{
							if((signed)response.find("unselected") != (signed)std::string::npos)
							{
								level = "";
							}
							else if((signed)response.find("selected") != (signed)std::string::npos)
							{
								level = "(Family)";
							}
						}
						else if(response.compare(0, 4, "Peer") == 0)
						{
							if((signed)response.find("unselected") != (signed)std::string::npos)
							{
								level = "(Family)";
							}
							else if((signed)response.find("selected") != (signed)std::string::npos)
							{
								level = "(Peer)";
							}
						}
						/*else if(response.compare(0, 6, "Device") == 0)
						{
							if((signed)response.find("unselected") != (signed)std::string::npos)
							{
								level = "(Family)";
							}
							else if((signed)response.find("selected") != (signed)std::string::npos)
							{
								level = "(Device)";
							}
						}*/
					/*}

					if(bytes < 1024 || (bytes == 1024 && receiveBuffer[bytes - 1] == 0))
					{
						if(!command.empty())
						{
							_sendMutex.unlock();

							// {{{ Get last line and check if it contains the exit code.
							if(response.size() > 2)
							{
								const char* pos = response.c_str() + response.size() - 2; // -2, because last line ends with new line

								while(pos > response.c_str())
								{
									if(*pos == '\n')
									{
										pos++;
										break;
									}
									pos--;
								}

								int32_t count = (response.c_str() + response.size()) - pos - 1;
								std::string lastLine(pos, count);
								if(lastLine.compare(0, 11, "Exit code: ") == 0 && lastLine.size() > 11)
								{
									count = pos - response.c_str();
									response = response.substr(0, count);
									std::cout << response;
									std::string exitCode = lastLine.substr(11);
									return BaseLib::Math::getNumber(exitCode);
								}
								else std::cout << response;
							}
							// }}}

							return 0;
						}
						else std::cout << response;
						break;
					}
					else std::cout << response;
				}
				else
				{
					_sendMutex.unlock();
					if(bytes < 0) std::cerr << "Error receiving data from socket." << std::endl;
					else std::cout << "Connection closed." << std::endl;
					//If we close the socket, the socket file gets deleted. We don't want that
					//GD::bl->fileDescriptorManager.close(_fileDescriptor);
					return 8;
				}
			}
			_sendMutex.unlock();
		}*/
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
    return;
}
