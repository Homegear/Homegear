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

#include "CLIServer.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

namespace CLI {

Server::Server()
{
}

Server::~Server()
{
	stop();
	int32_t i = 0;
	while(_clientData.size() > 0 && i < 300)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i++;
	}
}

void Server::start()
{
	try
	{
		stop();
		_stopServer = false;
		_mainThread = std::thread(&Server::mainThread, this);
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
		if(_mainThread.joinable()) _mainThread.join();
		unlink(GD::socketPath.c_str());
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
				getFileDescriptor();
				if(_serverFileDescriptor->descriptor == -1)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					continue;
				}
				clientFileDescriptor = getClientFileDescriptor();
				if(!clientFileDescriptor || clientFileDescriptor->descriptor < 0) continue;
				if(clientFileDescriptor->descriptor > _maxConnections)
				{
					GD::out.printError("Error: Client connection rejected, because there are too many clients connected to me.");
					GD::bl->fileDescriptorManager.shutdown(clientFileDescriptor);
					continue;
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
			try
			{
				std::shared_ptr<ClientData> clientData = std::shared_ptr<ClientData>(new ClientData(clientFileDescriptor));
				clientData->id = _currentClientID++;
				_stateMutex.lock();
				_clientData[clientData->id] = clientData;
				clientData->readThread = std::shared_ptr<std::thread>(new std::thread(&Server::readClient, this, clientData));
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
			_stateMutex.unlock();
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
		FD_ZERO(&readFileDescriptor);
		GD::bl->fileDescriptorManager.lock();
		int32_t nfds = _serverFileDescriptor->descriptor + 1;
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		GD::bl->fileDescriptorManager.unlock();
		if(nfds <= 0)
		{
			GD::out.printError("Error: CLI server socket descriptor is invalid.");
			return descriptor;
		}
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout)) return descriptor;

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

void Server::removeClientData(uint32_t clientID)
{
	try
	{
		_stateMutex.lock();
		if(_clientData.find(clientID) != _clientData.end())
		{
			std::shared_ptr<ClientData> clientData = _clientData.at(clientID);
			clientData->readThread->detach();
			_clientData.erase(clientID);
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
    _stateMutex.unlock();
}

void Server::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		struct stat sb;
		if(stat(GD::runDir.c_str(), &sb) == -1)
		{
			if(errno == ENOENT) GD::out.printError("Directory " + GD::runDir + " does not exist. Please create it before starting Homegear.");
			else throw BaseLib::Exception("Error reading information of directory " + GD::runDir + ": " + strerror(errno));
			return;
		}
		if(!S_ISDIR(sb.st_mode))
		{
			GD::out.printError("Directory " + GD::runDir + " does not exist. Please create it before starting Homegear.");
			return;
		}
		if(deleteOldSocket)
		{
			if(unlink(GD::socketPath.c_str()) == -1 && errno != ENOENT) throw(BaseLib::Exception("Couldn't delete existing socket: " + GD::socketPath + ". Error: " + strerror(errno)));
		}
		else if(stat(GD::socketPath.c_str(), &sb) == 0) return;

		_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if(_serverFileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't create socket: " + GD::socketPath + ". Error: " + strerror(errno)));
		int32_t reuseAddress = 1;
		if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseAddress, sizeof(int32_t)) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			throw(BaseLib::Exception("Couldn't set socket options: " + GD::socketPath + ". Error: " + strerror(errno)));
		}
		sockaddr_un serverAddress;
		serverAddress.sun_family = AF_UNIX;
		if(GD::socketPath.length() > 107)
		{
			//Check for buffer overflow
			GD::out.printCritical("Critical: Socket path is too long.");
			return;
		}
		strncpy(serverAddress.sun_path, GD::socketPath.c_str(), 107);
		serverAddress.sun_path[107] = 0; //Just to make sure the string is null terminated.
		bool bound = (bind(_serverFileDescriptor->descriptor, (sockaddr*)&serverAddress, strlen(serverAddress.sun_path) + sizeof(serverAddress.sun_family)) != -1);
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			throw BaseLib::Exception("Error: CLI server could not start listening. Error: " + std::string(strerror(errno)));
		}
		if(chmod(GD::socketPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP) == -1)
		{
			GD::out.printError("Error: chmod failed on unix socket \"" + GD::socketPath + "\".");
		}
		return;
    }
    catch(const std::exception& ex)
    {
    	GD::out.printError("Couldn't create socket file " + GD::socketPath + ": " + ex.what());
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
			FD_ZERO(&readFileDescriptor);
			GD::bl->fileDescriptorManager.lock();
			int32_t nfds = clientData->fileDescriptor->descriptor + 1;
			FD_SET(clientData->fileDescriptor->descriptor, &readFileDescriptor);
			GD::bl->fileDescriptorManager.unlock();
			if(nfds <= 0)
			{
				removeClientData(clientData->id);
				GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
				return;
			}
			bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				removeClientData(clientData->id);
				GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
				return;
			}

			bytesRead = read(clientData->fileDescriptor->descriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				removeClientData(clientData->id);
				GD::out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
				//If we close the socket, the socket file gets deleted. We don't want that
				//GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
				return;
			}

			std::string command;
			command.insert(command.end(), buffer, buffer + bytesRead);
			handleCommand(command, clientData);
		}
		//This point is only reached, when stopServer is true
		removeClientData(clientData->id);
		GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
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
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "users list (ul)\t\tLists all users." << std::endl;
			stringStream << "users create (uc)\tCreate a new user." << std::endl;
			stringStream << "users update (uu)\tChange the password of an existing user." << std::endl;
			stringStream << "users delete (ud)\tDelete an existing user." << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 10, "users list") == 0 || command.compare(0, 2, "ul") == 0)
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

			std::shared_ptr<BaseLib::Database::DataTable> rows = GD::db.getUsers();
			if(rows->size() == 0) return "No users exist.\n";

			stringStream << std::left << std::setfill(' ') << std::setw(6) << "ID" << std::setw(30) << "Name" << std::endl;
			for(BaseLib::Database::DataTable::const_iterator i = rows->begin(); i != rows->end(); ++i)
			{
				if(i->second.size() < 2 || !i->second.at(0) || !i->second.at(1)) continue;
				stringStream << std::setw(6) << i->second.at(0)->intValue << std::setw(30) << i->second.at(1)->textValue << std::endl;
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

			if(GD::db.userNameExists(userName)) return "A user with that name already exists.\n";

			std::vector<uint8_t> salt;
			std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL(password, salt);

			if(GD::db.createUser(userName, passwordHash, salt)) stringStream << "User successfully created." << std::endl;
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

			uint64_t userID = GD::db.getUserID(userName);
			if(userID == 0) return "The user doesn't exist.\n";

			std::vector<uint8_t> salt;
			std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL(password, salt);

			if(GD::db.updateUser(userID, passwordHash, salt)) stringStream << "User successfully updated." << std::endl;
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

			uint64_t userID = GD::db.getUserID(userName);
			if(userID == 0) return "The user doesn't exist.\n";

			if(GD::db.deleteUser(userID)) stringStream << "User successfully deleted." << std::endl;
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

std::string Server::handleGlobalCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if((command == "help" || command == "h") && !GD::familyController.familySelected())
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "debuglevel (dl)\t\tChanges the debug level" << std::endl;
			stringStream << "runscript (rs)\t\tExecutes a script with the internal PHP engine" << std::endl;
			stringStream << "users [COMMAND]\t\tExecute user commands. Type \"users help\" for more information." << std::endl;
			stringStream << "families [COMMAND]\tExecute device family commands. Type \"families help\" for more information." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 10, "debuglevel") == 0 || (command.compare(0, 2, "dl") == 0 && !GD::familyController.familySelected()))
		{
			int32_t debugLevel;

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
				else if(index == 1)
				{
					if(element == "help") break;
					debugLevel = BaseLib::HelperFunctions::getNumber(element);
					if(debugLevel < 0 || debugLevel > 10) return "Invalid debug level. Please provide a debug level between 0 and 10.\n";
				}
				index++;
			}
			if(index == 1)
			{
				stringStream << "Description: This command changes the current debug level temporarily until Homegear is restarted." << std::endl;
				stringStream << "Usage: debuglevel DEBUGLEVEL" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEBUGLEVEL:\tThe debug level between 0 and 10." << std::endl;
				return stringStream.str();
			}

			GD::bl->debugLevel = debugLevel;
			stringStream << "Debug level set to " << debugLevel << "." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 9, "runscript") == 0 || command.compare(0, 2, "rs") == 0)
		{
			std::string path;

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
					path = GD::bl->settings.scriptPath() + element;
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

			int32_t exitCode = GD::scriptEngine.execute(path, arguments.str());
			stringStream << "Script executed. Exit code: " << std::dec << exitCode << std::endl;
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

void Server::handleCommand(std::string& command, std::shared_ptr<ClientData> clientData)
{
	try
	{
		if(!command.empty() && command.at(0) == 0) return;
		std::string response = handleGlobalCommand(command);
		if(response.empty())
		{
			if(command.compare(0, 5, "users") == 0 || (BaseLib::HelperFunctions::isShortCLICommand(command) && command.at(0) == 'u' && !GD::familyController.familySelected())) response = handleUserCommand(command);
			else response = GD::familyController.handleCLICommand(command);
		}
		response.push_back(0);
		if(send(clientData->fileDescriptor->descriptor, response.c_str(), response.size(), MSG_NOSIGNAL) == -1)
		{
			GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
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

} /* namespace CLI */
