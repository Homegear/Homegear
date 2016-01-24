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

#include "ScriptEngineServer.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"

int32_t ScriptEngineServer::_currentClientID = 0;

ScriptEngineServer::ScriptEngineServer() : IQueue(GD::bl.get(), 1000)
{
	_out.init(GD::bl.get());
	_out.setPrefix("Script Engine Server: ");
}

ScriptEngineServer::~ScriptEngineServer()
{
	stop();
}

void ScriptEngineServer::collectGarbage()
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
			_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		_stateMutex.unlock();
		for(std::vector<std::shared_ptr<ClientData>>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			_out.printDebug("Debug: Joining read thread of script engine client " + std::to_string((*i)->id));
			GD::bl->threadManager.join((*i)->readThread);
			_stateMutex.lock();
			try
			{
				if(i->use_count() <= 2) _clients.erase((*i)->id);
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_stateMutex.unlock();
			_out.printDebug("Debug: CLI client " + std::to_string((*i)->id) + " removed.");
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
    _garbageCollectionMutex.unlock();
}

void ScriptEngineServer::start()
{
	try
	{
		stop();
		_socketPath = GD::runDir + "homegearSE.sock";
		GD::bl->threadManager.start(_mainThread, true, &ScriptEngineServer::mainThread, this);
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

void ScriptEngineServer::stop()
{
	try
	{
		_stopServer = true;
		GD::bl->threadManager.join(_mainThread);
		_out.printDebug("Debug: Waiting for script engine server's client threads to finish.");
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
		unlink(GD::socketPath.c_str());
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

void ScriptEngineServer::closeClientConnection(std::shared_ptr<ClientData> client)
{
	try
	{
		if(!client) return;
		GD::bl->fileDescriptorManager.close(client->fileDescriptor);
		client->closed = true;
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

void ScriptEngineServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{

}

void ScriptEngineServer::mainThread()
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
				if(_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections())
				{
					collectGarbage();
					if(_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections())
					{
						_out.printError("Error: There are too many clients connected to me. Waiting for connections to close. You can increase the number of allowed connections in main.conf.");
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
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				continue;
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				continue;
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			try
			{
				std::shared_ptr<ClientData> clientData = std::shared_ptr<ClientData>(new ClientData(clientFileDescriptor));
				std::lock_guard<std::mutex> stateGuard(_stateMutex);
				clientData->id = _currentClientID++;
				_clients[clientData->id] = clientData;
				GD::bl->threadManager.start(clientData->readThread, true, &ScriptEngineServer::readClient, this, clientData);
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
		GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
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

std::shared_ptr<BaseLib::FileDescriptor> ScriptEngineServer::getClientFileDescriptor()
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
		if(nfds <= 0)
		{
			GD::bl->fileDescriptorManager.unlock();
			_out.printError("Error: Socket descriptor is invalid.");
			return descriptor;
		}
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		GD::bl->fileDescriptorManager.unlock();
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout))
		{
			if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.scriptEngineServerMaxConnections() * 100 / 112) collectGarbage();
			return descriptor;
		}

		sockaddr_un clientAddress;
		socklen_t addressSize = sizeof(addressSize);
		descriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientAddress, &addressSize));
		if(descriptor->descriptor == -1) return descriptor;
		_out.printInfo("Info: Connection accepted. Client number: " + std::to_string(descriptor->id));

		return descriptor;
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
    return descriptor;
}

void ScriptEngineServer::getFileDescriptor(bool deleteOldSocket)
{
	try
	{
		struct stat sb;
		if(stat(GD::runDir.c_str(), &sb) == -1)
		{
			if(errno == ENOENT) _out.printCritical("Critical: Directory " + GD::runDir + " does not exist. Please create it before starting Homegear otherwise the script engine won't work.");
			else _out.printCritical("Critical: Error reading information of directory " + GD::runDir + ". The script engine won't work: " + strerror(errno));
			_stopServer = true;
			return;
		}
		if(!S_ISDIR(sb.st_mode))
		{
			_out.printCritical("Critical: Directory " + GD::runDir + " does not exist. Please create it before starting Homegear otherwise the script engine interface won't work.");
			_stopServer = true;
			return;
		}
		if(deleteOldSocket)
		{
			if(unlink(_socketPath.c_str()) == -1 && errno != ENOENT)
			{
				_out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath + ". Please delete it manually. The script engine won't work. Error: " + strerror(errno));
				return;
			}
		}
		else if(stat(_socketPath.c_str(), &sb) == 0) return;

		_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if(_serverFileDescriptor->descriptor == -1)
		{
			_out.printCritical("Critical: Couldn't create socket: " + _socketPath + ". The script engine won't work. Error: " + strerror(errno));
			return;
		}
		int32_t reuseAddress = 1;
		if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseAddress, sizeof(int32_t)) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Couldn't set socket options: " + _socketPath + ". The script engine won't work correctly. Error: " + strerror(errno));
			return;
		}
		sockaddr_un serverAddress;
		serverAddress.sun_family = AF_LOCAL;
		//104 is the size on BSD systems - slightly smaller than in Linux
		if(_socketPath.length() > 104)
		{
			//Check for buffer overflow
			_out.printCritical("Critical: Socket path is too long.");
			return;
		}
		strncpy(serverAddress.sun_path, _socketPath.c_str(), 104);
		serverAddress.sun_path[103] = 0; //Just to make sure the string is null terminated.
		bool bound = (bind(_serverFileDescriptor->descriptor, (sockaddr*)&serverAddress, strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Critical: Script engine server could not start listening. Error: " + std::string(strerror(errno)));
		}
		if(chmod(_socketPath.c_str(), S_IRWXU | S_IRWXG) == -1)
		{
			_out.printError("Error: chmod failed on unix socket \"" + _socketPath + "\".");
		}
		return;
    }
    catch(const std::exception& ex)
    {
    	_out.printError("Critical: Couldn't create socket file " + _socketPath + ": " + ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
}

void ScriptEngineServer::readClient(std::shared_ptr<ClientData> clientData)
{
	try
	{
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		int32_t bytesRead;
		_out.printDebug("Debug: Listening for incoming commands from client number " + std::to_string(clientData->fileDescriptor->id) + ".");
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
			if(nfds <= 0)
			{
				GD::bl->fileDescriptorManager.unlock();
				_out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				closeClientConnection(clientData);
				return;
			}
			FD_SET(clientData->fileDescriptor->descriptor, &readFileDescriptor);
			GD::bl->fileDescriptorManager.unlock();
			bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				_out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				closeClientConnection(clientData);
				return;
			}

			bytesRead = read(clientData->fileDescriptor->descriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				_out.printDebug("Connection to client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
				closeClientConnection(clientData);
				//If we close the socket, the socket file gets deleted. We don't want that
				//GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
				return;
			}

			std::string command;
			command.insert(command.end(), buffer, buffer + bytesRead);
			//handleCommand(command, clientData);
		}
		closeClientConnection(clientData);
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
