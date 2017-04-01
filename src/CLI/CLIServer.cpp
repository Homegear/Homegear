/* Copyright 2013-2017 Sathya Laufer
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

#include "CLIServer.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace CLI {

int32_t Server::_currentClientID = 0;

Server::Server()
{
	_stopServer = false;
}

Server::~Server()
{
	stop();
}

void Server::collectGarbage()
{
	_garbageCollectionMutex.lock();
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<std::shared_ptr<ClientData>> clientsToRemove;
		_stateMutex.lock();
		try
		{
			for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				if(i->second->closed) clientsToRemove.push_back(i->second);
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		_stateMutex.unlock();
		for(std::vector<std::shared_ptr<ClientData>>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			GD::out.printDebug("Debug: Joining read thread of CLI client " + std::to_string((*i)->id));
			GD::bl->threadManager.join((*i)->readThread);
			_stateMutex.lock();
			try
			{
				_clients.erase((*i)->id);
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_stateMutex.unlock();
			GD::out.printDebug("Debug: CLI client " + std::to_string((*i)->id) + " removed.");
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _garbageCollectionMutex.unlock();
}

void Server::start()
{
	try
	{
		_socketPath = GD::bl->settings.socketPath() + "homegear.sock";
		stop();
		_stopServer = false;
		GD::bl->threadManager.start(_mainThread, true, &Server::mainThread, this);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Server::stop()
{
	try
	{
		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		GD::out.printDebug("Debug: Waiting for CLI client threads to finish.");
		{
			std::lock_guard<std::mutex> stateGuard(_stateMutex);
			for(std::map<int32_t, std::shared_ptr<ClientData>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
			{
				closeClientConnection(i->second);
			}
		}
		while(_clients.size() > 0)
		{
			collectGarbage();
			if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		unlink(_socketPath.c_str());
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void Server::closeClientConnection(std::shared_ptr<ClientData> client)
{
	try
	{
		if(!client) return;
		GD::bl->fileDescriptorManager.close(client->fileDescriptor);
		client->closed = true;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Server::mainThread()
{
	try
	{
		getFileDescriptor(true); //Deletes an existing socket file
		std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor;
		while(!_stopServer)
		{
			try
			{
				//Don't lock _stateMutex => no synchronisation needed
				if(_clients.size() > GD::bl->settings.cliServerMaxConnections())
				{
					collectGarbage();
					if(_clients.size() > GD::bl->settings.cliServerMaxConnections())
					{
						GD::out.printError("Error in CLI server: There are too many clients connected to me. Waiting for connections to close. You can increase the number of allowed connections in main.conf.");
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						continue;
					}
				}
				getFileDescriptor();
				if(!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					continue;
				}
				clientFileDescriptor = getClientFileDescriptor();
				if(!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				continue;
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				continue;
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			try
			{
				std::shared_ptr<ClientData> clientData = std::shared_ptr<ClientData>(new ClientData(clientFileDescriptor));
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				clientData->id = _currentClientID++;
				_clients[clientData->id] = clientData;
				GD::bl->threadManager.start(clientData->readThread, false, &Server::readClient, this, clientData);
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::FileDescriptor> Server::getClientFileDescriptor()
{
	std::shared_ptr<BaseLib::FileDescriptor> descriptor;
	try
	{
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		int32_t nfds = 0;
		FD_ZERO(&readFileDescriptor);
		{
			auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
			fileDescriptorGuard.lock();
			nfds = _serverFileDescriptor->descriptor + 1;
			if(nfds <= 0)
			{
				fileDescriptorGuard.unlock();
				GD::out.printError("Error: CLI server socket descriptor is invalid.");
				return descriptor;
			}
			FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		}
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout))
		{
			if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.cliServerMaxConnections() * 100 / 112) collectGarbage();
			return descriptor;
		}

		sockaddr_un clientAddress;
		socklen_t addressSize = sizeof(addressSize);
		descriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientAddress, &addressSize));
		if(descriptor->descriptor == -1) return descriptor;
		GD::out.printInfo("Info: CLI connection accepted. Client number: " + std::to_string(descriptor->id));

		return descriptor;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return descriptor;
}

void Server::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		struct stat sb;
		if(stat(GD::bl->settings.socketPath().c_str(), &sb) == -1)
		{
			if(errno == ENOENT) GD::out.printError("Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the command line interface won't work.");
			else throw BaseLib::Exception("Error reading information of directory " + GD::bl->settings.socketPath() + ": " + strerror(errno));
			_stopServer = true;
			return;
		}
		if(!S_ISDIR(sb.st_mode))
		{
			GD::out.printError("Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the command line interface won't work.");
			_stopServer = true;
			return;
		}
		if(deleteOldSocket)
		{
			if(unlink(_socketPath.c_str()) == -1 && errno != ENOENT) throw(BaseLib::Exception("Couldn't delete existing socket: " + _socketPath + ". Error: " + strerror(errno)));
		}
		else if(stat(_socketPath.c_str(), &sb) == 0) return;

		_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if(_serverFileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't create socket: " + _socketPath + ". Error: " + strerror(errno)));
		int32_t reuseAddress = 1;
		if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseAddress, sizeof(int32_t)) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			throw(BaseLib::Exception("Couldn't set socket options: " + _socketPath + ". Error: " + strerror(errno)));
		}
		sockaddr_un serverAddress;
		serverAddress.sun_family = AF_LOCAL;
		//104 is the size on BSD systems - slightly smaller than in Linux
		if(_socketPath.length() > 104)
		{
			//Check for buffer overflow
			GD::out.printCritical("Critical: Socket path is too long.");
			return;
		}
		strncpy(serverAddress.sun_path, _socketPath.c_str(), 104);
		serverAddress.sun_path[103] = 0; //Just to make sure the string is null terminated.
		bool bound = (bind(_serverFileDescriptor->descriptor, (sockaddr*)&serverAddress, strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			throw BaseLib::Exception("Error: CLI server could not start listening. Error: " + std::string(strerror(errno)));
		}
		if(chmod(_socketPath.c_str(), S_IRWXU | S_IRWXG) == -1)
		{
			GD::out.printError("Error: chmod failed on unix socket \"" + _socketPath + "\".");
		}
		return;
    }
    catch(const std::exception& ex)
    {
    	GD::out.printError("Couldn't create socket file " + _socketPath + ": " + ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
}

void Server::readClient(std::shared_ptr<ClientData> clientData)
{
	try
	{
		std::string unselect = "unselect";
		GD::familyController->handleCliCommand(unselect);
		GD::familyController->handleCliCommand(unselect);
		GD::familyController->handleCliCommand(unselect);
		GD::familyController->handleCliCommand(unselect);

		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		int32_t bytesRead;
		GD::out.printDebug("Listening for incoming commands from client number " + std::to_string(clientData->fileDescriptor->id) + ".");
		while(!_stopServer)
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			fd_set readFileDescriptor;
			int32_t nfds = 0;
			FD_ZERO(&readFileDescriptor);
			{
				auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
				fileDescriptorGuard.lock();
				nfds = clientData->fileDescriptor->descriptor + 1;
				if(nfds <= 0)
				{
					fileDescriptorGuard.unlock();
					GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
					closeClientConnection(clientData);
					return;
				}
				FD_SET(clientData->fileDescriptor->descriptor, &readFileDescriptor);
			}
			bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				closeClientConnection(clientData);
				return;
			}

			bytesRead = read(clientData->fileDescriptor->descriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				closeClientConnection(clientData);
				//If we close the socket, the socket file gets deleted. We don't want that
				//GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
				return;
			}

			std::string command;
			command.insert(command.end(), buffer, buffer + bytesRead);
			handleCommand(command, clientData);
		}
		closeClientConnection(clientData);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string Server::handleUserCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command.compare(0, 10, "users help") == 0 || command.compare(0, 2, "uh") == 0)
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "users list (ul)\t\tLists all users." << std::endl;
			stringStream << "users create (uc)\tCreate a new user." << std::endl;
			stringStream << "users update (uu)\tChange the password of an existing user." << std::endl;
			stringStream << "users delete (ud)\tDelete an existing user." << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 10, "users list") == 0 || command.compare(0, 2, "ul") == 0 || command.compare(0, 2, "ls") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'l') ? 0 : 1;
			int32_t index = 0;
			bool printHelp = false;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset && element == "help") printHelp = true;
				else
				{
					printHelp = true;
					break;
				}
				index++;
			}
			if(printHelp)
			{
				stringStream << "Description: This command lists all known users." << std::endl;
				stringStream << "Usage: users list" << std::endl << std::endl;
				return stringStream.str();
			}

			std::map<uint64_t, std::string> users;
			User::getAll(users);
			if(users.size() == 0) return "No users exist.\n";

			stringStream << std::left << std::setfill(' ') << std::setw(6) << "ID" << std::setw(30) << "Name" << std::endl;
			for(std::map<uint64_t, std::string>::iterator i = users.begin(); i != users.end(); ++i)
			{
				if(i->second.size() < 2 || !i->second.at(0) || !i->second.at(1)) continue;
				stringStream << std::setw(6) << i->first << std::setw(30) << i->second << std::endl;
			}

			return stringStream.str();
		}
		else if(command.compare(0, 12, "users create") == 0 || command.compare(0, 2, "uc") == 0)
		{
			std::string userName;
			std::string password;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'c') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else
					{
						userName = BaseLib::HelperFunctions::toLower(BaseLib::HelperFunctions::trim(element));
						if(userName.empty() || !BaseLib::HelperFunctions::isAlphaNumeric(userName))
						{
							stringStream << "The user name contains invalid characters. Only alphanumeric characters, \"_\" and \"-\" are allowed." << std::endl;
							return stringStream.str();
						}
					}
				}
				else if(index == 2 + offset)
				{
					password = BaseLib::HelperFunctions::trim(element);

					if(password.front() == '"' && password.back() == '"')
					{
						password = password.substr(1, password.size() - 2);
						BaseLib::HelperFunctions::stringReplace(password, "\\\"", "\"");
						BaseLib::HelperFunctions::stringReplace(password, "\\\\", "\\");
					}
					if(password.size() < 8)
					{
						stringStream << "The password is too short. Please choose a password with at least 8 characters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}
			if(index < 3 + offset)
			{
				stringStream << "Description: This command creates a new user." << std::endl;
				stringStream << "Usage: users create USERNAME \"PASSWORD\"" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  USERNAME:\tThe user name of the new user to create. It may contain alphanumeric characters, \"_\"" << std::endl;
				stringStream << "           \tand \"-\". Example: foo" << std::endl;
				stringStream << "  PASSWORD:\tThe password for the new user. All characters are allowed. If the password contains spaces," << std::endl;
				stringStream << "           \twrap it in double quotes." << std::endl;
				stringStream << "           \tExample: MyPassword" << std::endl;
				stringStream << "           \tExample with spaces and escape characters: \"My_\\\\\\\"Password\" (Translates to: My_\\\"Password)" << std::endl;
				return stringStream.str();
			}

			if(User::exists(userName)) return "A user with that name already exists.\n";

			if(User::create(userName, password)) stringStream << "User successfully created." << std::endl;
			else stringStream << "Error creating user. See log for more details." << std::endl;

			return stringStream.str();
		}
		else if(command.compare(0, 12, "users update") == 0 || command.compare(0, 2, "uu") == 0)
		{
			std::string userName;
			std::string password;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'u') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else
					{
						userName = BaseLib::HelperFunctions::trim(element);
						if(userName.empty() || !BaseLib::HelperFunctions::isAlphaNumeric(userName))
						{
							stringStream << "The user name contains invalid characters. Only alphanumeric characters, \"_\" and \"-\" are allowed." << std::endl;
							return stringStream.str();
						}
					}
				}
				else if(index == 2 + offset)
				{
					password = BaseLib::HelperFunctions::trim(element);

					if(password.front() == '"' && password.back() == '"')
					{
						password = password.substr(1, password.size() - 2);
						BaseLib::HelperFunctions::stringReplace(password, "\\\"", "\"");
						BaseLib::HelperFunctions::stringReplace(password, "\\\\", "\\");
					}
					if(password.size() < 8)
					{
						stringStream << "The password is too short. Please choose a password with at least 8 characters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}
			if(index < 3 + offset)
			{
				stringStream << "Description: This command sets a new password for an existing user." << std::endl;
				stringStream << "Usage: users update USERNAME \"PASSWORD\"" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  USERNAME:\tThe user name of an existing user. Example: foo" << std::endl;
				stringStream << "  PASSWORD:\tThe new password for the user. All characters are allowed. If the password contains spaces," << std::endl;
				stringStream << "           \twrap it in double quotes." << std::endl;
				stringStream << "           \tExample: MyPassword" << std::endl;
				stringStream << "           \tExample with spaces and escape characters: \"My_\\\\\\\"Password\" (Translates to: My_\\\"Password)" << std::endl;
				return stringStream.str();
			}

			if(!User::exists(userName)) return "The user doesn't exist.\n";

			if(User::update(userName, password)) stringStream << "User successfully updated." << std::endl;
			else stringStream << "Error updating user. See log for more details." << std::endl;

			return stringStream.str();
		}
		else if(command.compare(0, 12, "users delete") == 0 || command.compare(0, 2, "ud") == 0)
		{
			std::string userName;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'd') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else
					{
						userName = BaseLib::HelperFunctions::trim(element);
						if(userName.empty() || !BaseLib::HelperFunctions::isAlphaNumeric(userName))
						{
							stringStream << "The user name contains invalid characters. Only alphanumeric characters, \"_\" and \"-\" are allowed." << std::endl;
							return stringStream.str();
						}
					}
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command deletes an existing user." << std::endl;
				stringStream << "Usage: users delete USERNAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  USERNAME:\tThe user name of the user to delete. Example: foo" << std::endl;
				return stringStream.str();
			}

			if(!User::exists(userName)) return "The user doesn't exist.\n";

			if(User::remove(userName)) stringStream << "User successfully deleted." << std::endl;
			else stringStream << "Error deleting user. See log for more details." << std::endl;

			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

std::string Server::handleModuleCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command.compare(0, 12, "modules help") == 0 || command.compare(0, 2, "mh") == 0)
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "modules list (mls)\tLists all family modules" << std::endl;
			stringStream << "modules load (mld)\tLoads a family module" << std::endl;
			stringStream << "modules unload (mul)\tUnloads a family module" << std::endl;
			stringStream << "modules reload (mrl)\tReloads a family module" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 12, "modules list") == 0 || command.compare(0, 3, "mls") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			bool printHelp = false;
			int32_t offset = (command.at(1) == 'l') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset && element == "help") printHelp = true;
				else
				{
					printHelp = true;
					break;
				}
				index ++;
			}
			if(printHelp)
			{
				stringStream << "Description: This command lists all loaded family modules and shows the corresponding family ids." << std::endl;
				stringStream << "Usage: modules list" << std::endl << std::endl;
				return stringStream.str();
			}

			std::vector<std::shared_ptr<FamilyController::ModuleInfo>> modules = GD::familyController->getModuleInfo();
			if(modules.size() == 0) return "No modules loaded.\n";

			stringStream << std::left << std::setfill(' ') << std::setw(6) << "ID" << std::setw(30) << "Family Name" << std::setw(30) << "Filename" << std::setw(14) << "Compiled For" << std::setw(7) << "Loaded" << std::endl;
			for(std::vector<std::shared_ptr<FamilyController::ModuleInfo>>::iterator i = modules.begin(); i != modules.end(); ++i)
			{
				stringStream << std::setw(6) << (*i)->familyId << std::setw(30) << (*i)->familyName << std::setw(30) << (*i)->filename << std::setw(14) << (*i)->baselibVersion << std::setw(7) << ((*i)->loaded ? "true" : "false") << std::endl;
			}

			return stringStream.str();
		}
		else if(command.compare(0, 12, "modules load") == 0 || command.compare(0, 3, "mld") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'l') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help" || element.empty()) break;
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command loads a family module from \"" + GD::bl->settings.modulePath() + "\"." << std::endl;
				stringStream << "Usage: modules load FILENAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\"" << std::endl;
				return stringStream.str();
			}

			int32_t result = GD::familyController->loadModule(element);
			switch(result)
			{
			case 0:
				stringStream << "Module successfully loaded." << std::endl;
				break;
			case 1:
				stringStream << "Module already loaded." << std::endl;
				break;
			case -1:
				stringStream << "System error. See log for more details." << std::endl;
				break;
			case -2:
				stringStream << "Module does not exist." << std::endl;
				break;
			case -4:
				stringStream << "Family initialization failed. See log for more details." << std::endl;
				break;
			default:
				stringStream << "Unknown result code: " << result << std::endl;
			}

			stringStream << "Exit code: " << std::dec << result << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "modules unload") == 0 || command.compare(0, 3, "mul") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'u') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help" || element.empty()) break;
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command unloads a family module." << std::endl;
				stringStream << "Usage: modules unload FILENAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\"" << std::endl;
				return stringStream.str();
			}

			int32_t result = GD::familyController->unloadModule(element);
			switch(result)
			{
			case 0:
				stringStream << "Module successfully unloaded." << std::endl;
				break;
			case 1:
				stringStream << "Module not loaded." << std::endl;
				break;
			case -1:
				stringStream << "System error. See log for more details." << std::endl;
				break;
			case -2:
				stringStream << "Module does not exist." << std::endl;
				break;
			default:
				stringStream << "Unknown result code: " << result << std::endl;
			}

			stringStream << "Exit code: " << std::dec << result << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "modules reload") == 0 || command.compare(0, 3, "mrl") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'r') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help" || element.empty()) break;
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command reloads a family module." << std::endl;
				stringStream << "Usage: modules reload FILENAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\"" << std::endl;
				return stringStream.str();
			}

			int32_t result = GD::familyController->reloadModule(element);
			switch(result)
			{
			case 0:
				stringStream << "Module successfully reloaded." << std::endl;
				break;
			case 1:
				stringStream << "Module already loaded." << std::endl;
				break;
			case -1:
				stringStream << "System error. See log for more details." << std::endl;
				break;
			case -2:
				stringStream << "Module does not exist." << std::endl;
				break;
			case -4:
				stringStream << "Family initialization failed. See log for more details." << std::endl;
				break;
			default:
				stringStream << "Unknown result code: " << result << std::endl;
			}

			stringStream << "Exit code: " << std::dec << result << std::endl;
			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}


std::string Server::handleGlobalCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		std::vector<std::string> arguments;
		bool showHelp = false;
		if(BaseLib::HelperFunctions::checkCliCommand(command, "help", "h", "", 0, arguments, showHelp) && !GD::familyController->familySelected())
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "debuglevel (dl)      Changes the debug level" << std::endl;
#ifndef NO_SCRIPTENGINE
			stringStream << "runscript (rs)       Executes a script with the internal PHP engine" << std::endl;
			stringStream << "runcommand (rc)      Executes a PHP command" << std::endl;
			stringStream << "scriptcount (sc)     Returns the number of currently running scripts" << std::endl;
			stringStream << "scriptsrunning (sr)  Returns the ID and filename of all running scripts" << std::endl;
#endif
			stringStream << "rpcservers (rpc)     Lists all active RPC servers" << std::endl;
			stringStream << "rpcclients (rcl)     Lists all active RPC clients" << std::endl;
			stringStream << "threads              Prints current thread count" << std::endl;
			stringStream << "lifetick (lt)        Checks the lifeticks of all components." << std::endl;
			stringStream << "users [COMMAND]      Execute user commands. Type \"users help\" for more information." << std::endl;
			stringStream << "families [COMMAND]   Execute device family commands. Type \"families help\" for more information." << std::endl;
			stringStream << "modules [COMMAND]    Execute module commands. Type \"modules help\" for more information." << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "disconnect", "dcr", "", 0, arguments, showHelp))
		{
			GD::rpcClient->disconnectRega();
			stringStream << "RegaHss socket closed." << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "debuglevel", "dl", "", 1, arguments, showHelp) && !GD::familyController->familySelected())
		{
			int32_t debugLevel = 3;

			if(showHelp)
			{
				stringStream << "Description: This command changes the current debug level temporarily until Homegear is restarted." << std::endl;
				stringStream << "Usage: debuglevel DEBUGLEVEL" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEBUGLEVEL:\tThe debug level between 0 and 10." << std::endl;
				return stringStream.str();
			}

			debugLevel = BaseLib::Math::getNumber(arguments.at(0), false);
			if(debugLevel < 0 || debugLevel > 10) return "Invalid debug level. Please provide a debug level between 0 and 10.\n";

			GD::bl->debugLevel = debugLevel;
			stringStream << "Debug level set to " << debugLevel << "." << std::endl;
			return stringStream.str();
		}
#ifndef NO_SCRIPTENGINE
		else if(command.compare(0, 9, "runscript") == 0 || command.compare(0, 2, "rs") == 0)
		{
			std::string fullPath;
			std::string relativePath;

			std::stringstream stream(command);
			std::string element;
			std::stringstream arguments;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help" || element.empty()) break;
					relativePath = '/' + element;
					fullPath = GD::bl->settings.scriptPath() + element;
				}
				else
				{
					arguments << element << " ";
				}
				index++;
			}
			if(index < 2)
			{
				stringStream << "Description: This command executes a script in the Homegear script folder using the internal PHP engine." << std::endl;
				stringStream << "Usage: runscript PATH [ARGUMENTS]" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PATH:\t\tPath relative to the Homegear scripts folder." << std::endl;
				stringStream << "  ARGUMENTS:\tParameters passed to the script." << std::endl;
				return stringStream.str();
			}

			std::string argumentsString = arguments.str();
			BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, argumentsString));
			scriptInfo->returnOutput = true;
			GD::scriptEngineServer->executeScript(scriptInfo, true);
			if(!scriptInfo->output.empty()) stringStream << scriptInfo->output;
			stringStream << "Exit code: " << std::dec << scriptInfo->exitCode << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 10, "runcommand") == 0 || command.compare(0, 3, "rc ") == 0 || command.compare(0, 1, "$") == 0)
		{
			int32_t commandSize = 11;
			if(command.compare(0, 2, "rc") == 0) commandSize = 3;
			else if(command.compare(0, 1, "$") == 0) commandSize = 0;
			if(((int32_t)command.size()) - commandSize < 4 || command.compare(commandSize, 4, "help") == 0)
			{
				stringStream << "Description: Executes a PHP command. The Homegear object ($hg) is defined implicitly." << std::endl;
				stringStream << "Usage: runcommand COMMAND" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  COMMAND:\t\tThe command to execute. E. g.: $hg->setValue(12, 2, \"STATE\", true);" << std::endl;
				return stringStream.str();
			}

			std::string script;
			script.reserve(command.size() - commandSize + 50);
			script.append("<?php\nuse Homegear\\Homegear as Homegear;\nuse Homegear\\HomegearGpio as HomegearGpio;\nuse Homegear\\HomegearSerial as HomegearSerial;\nuse Homegear\\HomegearI2c as HomegearI2c;\n$hg = new Homegear();\n");
			if(commandSize > 0) command = command.substr(commandSize);
			BaseLib::HelperFunctions::trim(command);
			if(command.size() < 4) return "Invalid code.";
			if((command.front() == '\"' && command.back() == '\"') || (command.front() == '\'' && command.back() == '\'')) command = command.substr(1, command.size() - 2);
			command.push_back(';');

			script.append(command);

			std::string fullPath = GD::bl->settings.scriptPath() + "inline.php";
			std::string relativePath = "/inline.php";
			std::string arguments;
			BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, script, arguments));
			scriptInfo->returnOutput = true;
			GD::scriptEngineServer->executeScript(scriptInfo, true);
			if(!scriptInfo->output.empty()) stringStream << scriptInfo->output;
			stringStream << "Exit code: " << std::dec << scriptInfo->exitCode << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "scriptcount", "sc", "", 0, arguments, showHelp))
		{
			if(showHelp)
			{
				stringStream << "Description: This command returns the total number of currently running scripts." << std::endl;
				stringStream << "Usage: scriptcount" << std::endl << std::endl;
				return stringStream.str();
			}

			stringStream << std::dec << GD::scriptEngineServer->scriptCount() << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "scriptsrunning", "sr", "", 0, arguments, showHelp))
		{
			if(showHelp)
			{
				stringStream << "Description: This command returns the script IDs and filenames of all running scripts." << std::endl;
				stringStream << "Usage: scriptsrunning" << std::endl << std::endl;
				return stringStream.str();
			}

			std::vector<std::tuple<int32_t, uint64_t, int32_t, std::string>> runningScripts = GD::scriptEngineServer->getRunningScripts();
			if(runningScripts.empty()) return "No scripts are being executed.\n";

			stringStream << std::left << std::setfill(' ') << std::setw(10) << "PID" << std::setw(10) << "Peer ID" << std::setw(10) << "Script ID" << std::setw(80) << "Filename" << std::endl;
			for(auto& script : runningScripts)
			{
				stringStream << std::setw(10) << std::get<0>(script) << std::setw(10) << (std::get<1>(script) > 0 ? std::to_string(std::get<1>(script)) : "") << std::setw(10) << std::get<2>(script) << std::setw(80) << std::get<3>(script) << std::endl;
			}

			return stringStream.str();
		}
#endif
		else if(command.compare(0, 10, "rpcclients") == 0 || command.compare(0, 3, "rcl") == 0)
		{
			std::stringstream stream(command);
			std::string element;

			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0)
				{
					index++;
					continue;
				}
				else
				{
					index++;
					break;
				}
			}
			if(index > 1)
			{
				stringStream << "Description: This command lists all connected RPC clients." << std::endl;
				stringStream << "Usage: rpcclients" << std::endl << std::endl;
				return stringStream.str();
			}

			int32_t idWidth = 10;
			int32_t addressWidth = 15;
			int32_t urlWidth = 30;
			int32_t interfaceIdWidth = 30;
			int32_t xmlWidth = 10;
			int32_t binaryWidth = 10;
			int32_t jsonWidth = 10;
			int32_t websocketWidth = 10;

			//Safe to use without mutex
			for(std::map<int32_t, Rpc::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
			{
				if(!i->second.isRunning()) continue;
				const BaseLib::Rpc::PServerInfo settings = i->second.getInfo();
				const std::vector<BaseLib::PRpcClientInfo> clients = i->second.getClientInfo();

				stringStream << "Server " << settings->name << " (Port: " << std::to_string(settings->port) << "):" << std::endl;

				std::string idCaption("Client ID");
				std::string addressCaption("Address");
				std::string urlCaption("Init URL");
				std::string interfaceIdCaption("Init ID");
				idCaption.resize(idWidth, ' ');
				addressCaption.resize(addressWidth, ' ');
				urlCaption.resize(urlWidth, ' ');
				interfaceIdCaption.resize(interfaceIdWidth, ' ');
				stringStream << std::setfill(' ')
				    << "    "
					<< idCaption << "  "
					<< addressCaption << "  "
					<< urlCaption << "  "
					<< interfaceIdCaption << "  "
					<< std::setw(xmlWidth) << "XML-RPC" << "  "
					<< std::setw(binaryWidth) << "Binary RPC" << "  "
					<< std::setw(jsonWidth) << "JSON-RPC" << "  "
					<< std::setw(websocketWidth) << "Websocket" << "  "
					<< std::endl;

				for(std::vector<BaseLib::PRpcClientInfo>::const_iterator j = clients.begin(); j != clients.end(); ++j)
				{
					std::string id = std::to_string((*j)->id);
					id.resize(idWidth, ' ');

					std::string address = (*j)->address;
					if(address.size() > (unsigned)addressWidth)
					{
						address.resize(addressWidth - 3);
						address += "...";
					}
					else address.resize(addressWidth, ' ');

					std::string url = (*j)->initUrl;
					if(url.size() > (unsigned)urlWidth)
					{
						url.resize(urlWidth - 3);
						url += "...";
					}
					else url.resize(urlWidth, ' ');

					std::string interfaceId = (*j)->initInterfaceId;
					if(interfaceId.size() > (unsigned)interfaceIdWidth)
					{
						interfaceId.resize(interfaceIdWidth - 3);
						interfaceId += "...";
					}
					else interfaceId.resize(interfaceIdWidth, ' ');

					stringStream
						<< "    "
						<< id << "  "
						<< address << "  "
						<< url << "  "
						<< interfaceId << "  "
						<< std::setw(xmlWidth) << ((*j)->rpcType == BaseLib::RpcType::xml ? "true" : "false") << "  "
						<< std::setw(binaryWidth) << ((*j)->rpcType == BaseLib::RpcType::binary ? "true" : "false") << "  "
						<< std::setw(jsonWidth) << ((*j)->rpcType == BaseLib::RpcType::json ? "true" : "false") << "  "
						<< std::setw(websocketWidth) << ((*j)->rpcType == BaseLib::RpcType::websocket ? "true" : "false") << "  "
						<< std::endl;
				}

				stringStream << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "rpcservers") == 0 || command.compare(0, 3, "rpc") == 0)
		{
			std::stringstream stream(command);
			std::string element;

			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index == 0)
				{
					index++;
					continue;
				}
				else
				{
					index++;
					break;
				}
			}
			if(index > 1)
			{
				stringStream << "Description: This command lists all active RPC servers." << std::endl;
				stringStream << "Usage: rpcservers" << std::endl << std::endl;
				return stringStream.str();
			}

			int32_t nameWidth = 20;
			int32_t interfaceWidth = 20;
			int32_t portWidth = 6;
			int32_t sslWidth = 5;
			int32_t authWidth = 5;
			std::string nameCaption("Name");
			std::string interfaceCaption("Interface");
			nameCaption.resize(nameWidth, ' ');
			interfaceCaption.resize(interfaceWidth, ' ');
			stringStream << std::setfill(' ')
				<< nameCaption << "  "
				<< interfaceCaption << "  "
				<< std::setw(portWidth) << "Port" << "  "
				<< std::setw(sslWidth) << "SSL" << "  "
				<< std::setw(authWidth) << "Auth" << "  "
				<< std::endl;

			//Safe to use without mutex
			for(std::map<int32_t, Rpc::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
			{
				if(!i->second.isRunning()) continue;
				const BaseLib::Rpc::PServerInfo settings = i->second.getInfo();
				std::string name = settings->name;
				if(name.size() > (unsigned)nameWidth)
				{
					name.resize(nameWidth - 3);
					name += "...";
				}
				else name.resize(nameWidth, ' ');
				std::string interface = settings->interface;
				if(interface.size() > (unsigned)interfaceWidth)
				{
					interface.resize(interfaceWidth - 3);
					interface += "...";
				}
				else interface.resize(interfaceWidth, ' ');
				stringStream
					<< name << "  "
					<< interface << "  "
					<< std::setw(portWidth) << settings->port << "  "
					<< std::setw(sslWidth) << (settings->ssl ? "true" : "false") << "  "
					<< std::setw(authWidth) << (settings->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic ? "basic" : "none") << "  "
					<< std::endl;

			}
			return stringStream.str();
		}
		else if(command.compare(0, 7, "threads") == 0)
		{
			stringStream << GD::bl->threadManager.getCurrentThreadCount() << " of " << GD::bl->threadManager.getMaxThreadCount() << std::endl << "Maximum thread count since start: " << GD::bl->threadManager.getMaxRegisteredThreadCount() << std::endl;
			return stringStream.str();
		}
		else if(BaseLib::HelperFunctions::checkCliCommand(command, "lifetick", "lt", "", 2, arguments, showHelp))
		{
			int32_t exitCode = 0;
			try
			{

				if(!GD::rpcClient->lifetick())
				{
					stringStream << "RPC Client: Failed" << std::endl;
					exitCode = 1;
				}
				else stringStream << "RPC Client: OK" << std::endl;
				for(std::map<int32_t, Rpc::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
				{
					if(!i->second.lifetick())
					{
						stringStream << "RPC Server (Port " << i->second.getInfo()->port << "): Failed" << std::endl;
						exitCode = 2;
					}
					else stringStream << "RPC Server (Port " << i->second.getInfo()->port << "): OK" << std::endl;
				}
				if(!GD::familyController->lifetick())
				{
					stringStream << "Device families: Failed" << std::endl;
					exitCode = 3;
				}
				else stringStream << "Device families: OK" << std::endl;
			}
			catch(const std::exception& ex)
			{
				exitCode = 127;
				GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				exitCode = 127;
				GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				exitCode = 127;
				GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			stringStream << "Exit code: " << exitCode << std::endl;
			return stringStream.str();
		}
		return "";
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

std::string Server::handleCommand(std::string& command)
{
	try
	{
		if(!command.empty() && command.at(0) == 0) return "";
		std::string response = handleGlobalCommand(command);
		if(response.empty())
		{
			//User commands can be executed when family is selected
			if(command.compare(0, 5, "users") == 0 || (BaseLib::HelperFunctions::isShortCliCommand(command) && command.at(0) == 'u' && !GD::familyController->familySelected())) response = handleUserCommand(command);
			//Do not execute module commands when family is selected
			else if((command.compare(0, 7, "modules") == 0 || (BaseLib::HelperFunctions::isShortCliCommand(command) && command.at(0) == 'm')) && !GD::familyController->familySelected()) response = handleModuleCommand(command);
			else response = GD::familyController->handleCliCommand(command);
		}
		return response;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

void Server::handleCommand(std::string& command, std::shared_ptr<ClientData> clientData)
{
	try
	{
		if(!command.empty() && command.at(0) == 0) return;
		std::string response = handleCommand(command);
		response.push_back(0);
		send(clientData, response);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Server::send(std::shared_ptr<ClientData> client, std::string& data)
{
	try
	{
		std::lock_guard<std::mutex> clientLock(client->sendMutex);
		if(client->fileDescriptor->descriptor == -1) return;
		int32_t totallySentBytes = 0;
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sizeToSend = data.size() - totallySentBytes;
			if(totallySentBytes + sizeToSend == (signed)data.size() && sizeToSend == 1024) sizeToSend -= 1; //Avoid packet size of exactly 1024, so CLI client does not hang. I know, this is a bad solution for the problem. At some point the CLI packets need to RPC encoded to avoid this problem.
			int32_t sentBytes = ::send(client->fileDescriptor->descriptor, data.c_str() + totallySentBytes, sizeToSend, MSG_NOSIGNAL);
			if(sentBytes == -1)
			{
				GD::out.printError("CLI Server: Error: Could not send data to client: " + std::to_string(client->fileDescriptor->descriptor));
				break;
			}
			totallySentBytes += sentBytes;
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
