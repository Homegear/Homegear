#include "CLIServer.h"
#include "../GD.h"

namespace CLI {

Server::Server()
{
}

Server::~Server()
{
	stop();
}

void Server::start()
{
	_mainThread = std::thread(&Server::mainThread, this);
	_mainThread.detach();
}

void Server::stop()
{
	try
	{
		_stopServer = true;
		unlink(GD::socketPath.c_str());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
		_stateMutex.unlock();
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
		_stateMutex.unlock();
	}
	catch(...)
	{
		std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
		_stateMutex.unlock();
	}
}

void Server::mainThread()
{
	getFileDescriptor();
	if(_serverFileDescriptor == -1) return;
	while(!_stopServer)
	{
		try
		{
			int32_t clientFileDescriptor = getClientFileDescriptor();
			if(clientFileDescriptor < 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
			if(clientFileDescriptor > _maxConnections)
			{
				if(GD::debugLevel >= 2) std::cout << "Error: Client connection rejected, because there are too many clients connected to me." << std::endl;
				shutdown(clientFileDescriptor, 0);
				close(clientFileDescriptor);
				continue;
			}
			std::shared_ptr<ClientData> clientData = std::shared_ptr<ClientData>(new ClientData(clientFileDescriptor));
			_stateMutex.lock();
			_fileDescriptors.push_back(clientData);
			_stateMutex.unlock();

			_readThreads.push_back(std::thread(&Server::readClient, this, clientData));
			_readThreads.back().detach();
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
			_stateMutex.unlock();
		}
		catch(const Exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
			_stateMutex.unlock();
		}
		catch(...)
		{
			std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
			_stateMutex.unlock();
		}
	}
	close(_serverFileDescriptor);
	_serverFileDescriptor = -1;
}

int32_t Server::getClientFileDescriptor()
{
	try
	{
		sockaddr_un clientAddress;
		socklen_t addressSize = sizeof(addressSize);
		int32_t clientFileDescriptor = accept(_serverFileDescriptor, (struct sockaddr *) &clientAddress, &addressSize);
		if(clientFileDescriptor == -1) return -1;

		if(GD::debugLevel >= 4) std::cout << "CLI connection accepted. Client number: " << clientFileDescriptor << std::endl;

		return clientFileDescriptor;
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return -1;
}

void Server::removeClientData(int32_t clientFileDescriptor)
{
	try
	{
		_stateMutex.lock();
		for(std::vector<std::shared_ptr<ClientData>>::iterator i = _fileDescriptors.begin(); i != _fileDescriptors.end(); ++i)
		{
			if((*i)->fileDescriptor == clientFileDescriptor)
			{
				_fileDescriptors.erase(i);
				_stateMutex.unlock();
				return;
			}
		}
		_stateMutex.unlock();
	}
	catch(const std::exception& ex)
    {
    	_stateMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_stateMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_stateMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void Server::getFileDescriptor()
{
	try
	{
		struct stat sb;
		if(stat(GD::runDir.c_str(), &sb) == -1)
		{
			if(errno == ENOENT && GD::debugLevel >= 1) std::cerr << "Directory " << GD::runDir << " does not exist. Please create it before starting Homegear." << std::endl;
			else throw Exception("Error reading information of directory " + GD::runDir + ": " + strerror(errno));
			return;
		}
		if(!S_ISDIR(sb.st_mode))
		{
			if(GD::debugLevel >= 1) std::cerr << "Directory " << GD::runDir << " does not exist. Please create it before starting Homegear." << std::endl;
			return;
		}
		if(unlink(GD::socketPath.c_str()) == -1 && errno != ENOENT) throw(Exception("Couldn't delete existing socket: " + GD::socketPath + ". Error: " + strerror(errno)));
		int32_t fileDescriptor;
		if((fileDescriptor = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) throw(Exception("Couldn't create socket: " + GD::socketPath + ". Error: " + strerror(errno)));
		sockaddr_un serverAddress;
		serverAddress.sun_family = AF_UNIX;
		strcpy(serverAddress.sun_path, GD::socketPath.c_str());
		bool bound = (bind(fileDescriptor, (sockaddr*)&serverAddress, strlen(serverAddress.sun_path) + sizeof(serverAddress.sun_family)) != -1);
		if(!bound && fileDescriptor > -1) close(fileDescriptor);
		if(fileDescriptor == -1 || listen(fileDescriptor, _backlog) == -1 || !bound) throw Exception("Error: CLI server could not start listening. Error: " + std::string(strerror(errno)));
		_serverFileDescriptor = fileDescriptor;
		chmod(GD::socketPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP);
    }
    catch(const std::exception& ex)
    {
    	std::cerr << "Couldn't create socket file " << GD::socketPath << ": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void Server::readClient(std::shared_ptr<ClientData> clientData)
{
	try
	{
		int32_t clientFileDescriptor = clientData->fileDescriptor;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		int32_t lineBufferMax = 128;
		char lineBuffer[lineBufferMax];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t dataSize = 0;
		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		if(GD::debugLevel >= 5) std::cout << "Listening for incoming commands from client number " << clientFileDescriptor << "." << std::endl;
		while(!_stopServer)
		{
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			FD_SET(clientFileDescriptor, &readFileDescriptor);
			bytesRead = select(clientFileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(bytesRead == 0) continue;
			if(bytesRead != 1)
			{
				removeClientData(clientFileDescriptor);
				close(clientFileDescriptor);
				if(GD::debugLevel >= 5) std::cout << "Connection to client number " << clientFileDescriptor << " closed." << std::endl;
				close(_serverFileDescriptor);
				//Socket file gets deleted when closing connection, so create it again
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				getFileDescriptor();
				return;
			}

			bytesRead = read(clientFileDescriptor, buffer, bufferMax);
			if(bytesRead <= 0)
			{
				removeClientData(clientFileDescriptor);
				close(clientFileDescriptor);
				if(GD::debugLevel >= 5) std::cout << "Connection to client number " << clientFileDescriptor << " closed." << std::endl;
				close(_serverFileDescriptor);
				//Socket file gets deleted when closing connection, so create it again
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				getFileDescriptor();
				return;
			}

			std::string command;
			command.insert(command.end(), buffer, buffer + bytesRead);
			handleCommand(command, clientData);
		}
		//This point is only reached, when stopServer is true
		removeClientData(clientFileDescriptor);
		close(clientFileDescriptor);
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void Server::handleCommand(std::string& command, std::shared_ptr<ClientData> clientData)
{
	try
	{
		if(!command.empty() && command.at(0) == 0) return;
		std::string response(GD::devices.handleCLICommand(command));
		response.push_back(0);
		if(send(clientData->fileDescriptor, response.c_str(), response.size(), MSG_NOSIGNAL) == -1)
		{
			std::cerr << "Could not send data to client: " << clientData->fileDescriptor << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

} /* namespace CLI */
