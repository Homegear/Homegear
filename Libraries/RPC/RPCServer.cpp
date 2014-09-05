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

#include "RPCServer.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

using namespace RPC;

RPCServer::Client::Client()
{
	 socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get()));
	 socketDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor());
}

RPCServer::Client::~Client()
{
	GD::bl->fileDescriptorManager.shutdown(socketDescriptor);
}

RPCServer::RPCServer()
{
	_out.init(GD::bl.get());

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
	_xmlRpcDecoder = std::unique_ptr<BaseLib::RPC::XMLRPCDecoder>(new BaseLib::RPC::XMLRPCDecoder(GD::bl.get()));
	_xmlRpcEncoder = std::unique_ptr<BaseLib::RPC::XMLRPCEncoder>(new BaseLib::RPC::XMLRPCEncoder(GD::bl.get()));

	_settings.reset(new ServerSettings::Settings());
	_rpcMethods.reset(new std::map<std::string, std::shared_ptr<RPCMethod>>);
	_serverFileDescriptor.reset(new BaseLib::FileDescriptor);
	_threadPriority = GD::bl->settings.rpcServerThreadPriority();
	_threadPolicy = GD::bl->settings.rpcServerThreadPolicy();
}

RPCServer::~RPCServer()
{
	stop();
	_rpcMethods->clear();
}

void RPCServer::start(std::shared_ptr<ServerSettings::Settings>& settings)
{
	try
	{
		stop();
		_stopServer = false;
		_settings = settings;
		if(!_settings)
		{
			_out.printError("Error: Settings is nullptr.");
			return;
		}
		_out.setPrefix("RPC Server (Port " + std::to_string(settings->port) + "): ");
		if(_settings->ssl)
		{
			int32_t result = 0;
			if((result = gnutls_certificate_allocate_credentials(&_x509Cred)) != GNUTLS_E_SUCCESS)
			{
				_out.printError("Error: Could not allocate TLS certificate structure: " + std::string(gnutls_strerror(result)));
				return;
			}
			if((result = gnutls_certificate_set_x509_key_file(_x509Cred, GD::bl->settings.certPath().c_str(), GD::bl->settings.keyPath().c_str(), GNUTLS_X509_FMT_PEM)) < 0)
			{
				_out.printError("Error: Could not load certificate or key file: " + std::string(gnutls_strerror(result)));
				gnutls_certificate_free_credentials(_x509Cred);
				return;
			}
			if(!_dhParams)
			{
			if(GD::bl->settings.loadDHParamsFromFile())
			{
				if((result = gnutls_dh_params_init(&_dhParams)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not initialize DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
				std::vector<uint8_t> binaryData;
				try
				{
					binaryData = GD::bl->hf.getUBinaryFileContent(GD::bl->settings.dhParamPath().c_str());
					binaryData.push_back(0); //gnutls_datum_t.data needs to be null terminated
				}
				catch(BaseLib::Exception& ex)
				{
					_out.printError("Error: Could not load DH parameter file \"" + GD::bl->settings.dhParamPath() + "\": " + std::string(ex.what()));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
				catch(...)
				{
					_out.printError("Error: Could not load DH parameter file \"" + GD::bl->settings.dhParamPath() + "\".");
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
				gnutls_datum_t data;
				data.data = &binaryData.at(0);
				data.size = binaryData.size();
				if((result = gnutls_dh_params_import_pkcs3(_dhParams, &data, GNUTLS_X509_FMT_PEM)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not import DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
			}
			else
			{
				uint32_t bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_ULTRA);
				if((result = gnutls_dh_params_init(&_dhParams)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not initialize DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
				if((result = gnutls_dh_params_generate2(_dhParams, bits)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not generate DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
			}
			}
			if((result = gnutls_priority_init(&_tlsPriorityCache, "NORMAL", NULL)) != GNUTLS_E_SUCCESS)
			{
				_out.printError("Error: Could not initialize cipher priorities: " + std::string(gnutls_strerror(result)));
				gnutls_certificate_free_credentials(_x509Cred);
				return;
			}
			gnutls_certificate_set_dh_params(_x509Cred, _dhParams);
		}
		_mainThread = std::thread(&RPCServer::mainThread, this);
		BaseLib::Threads::setThreadPriority(GD::bl.get(), _mainThread.native_handle(), _threadPriority, _threadPolicy);
		_stopped = false;
		return;
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

void RPCServer::stop()
{
	try
	{
		if(_stopped) return;
		_stopped = true;
		_stopServer = true;
		if(_mainThread.joinable()) _mainThread.join();
		int32_t i = 0;
		while(_clients.size() > 0 && i < 300)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			i++;
			if(i == 299) GD::out.printError("Error: " + std::to_string(_clients.size()) + " RPC clients are still connected to RPC server.");
		}
		if(_x509Cred)
		{
			gnutls_certificate_free_credentials(_x509Cred);
			_x509Cred = nullptr;
		}
		if(_tlsPriorityCache)
		{
			gnutls_priority_deinit(_tlsPriorityCache);
			_tlsPriorityCache = nullptr;
		}
		if(_dhParams)
		{
			gnutls_dh_params_deinit(_dhParams);
			_dhParams = nullptr;
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
}

uint32_t RPCServer::connectionCount()
{
	try
	{
		_stateMutex.lock();
		uint32_t connectionCount = _clients.size();
		_stateMutex.unlock();
		return connectionCount;
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
    _stateMutex.unlock();
    return 0;
}

void RPCServer::registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method)
{
	try
	{
		if(_rpcMethods->find(methodName) != _rpcMethods->end())
		{
			_out.printWarning("Warning: Could not register RPC method, because a method with this name already exists.");
			return;
		}
		_rpcMethods->insert(std::pair<std::string, std::shared_ptr<RPCMethod>>(methodName, method));
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

void RPCServer::closeClientConnection(std::shared_ptr<Client> client)
{
	try
	{
		removeClient(client->id);
		GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
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

void RPCServer::mainThread()
{
	try
	{
		getSocketDescriptor();
		while(!_stopServer)
		{
			try
			{
				if(!_serverFileDescriptor || _serverFileDescriptor->descriptor < 0)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					getSocketDescriptor();
					continue;
				}
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = getClientSocketDescriptor();
				if(!clientFileDescriptor || clientFileDescriptor->descriptor < 0) continue;
				_stateMutex.lock();
				if(_clients.size() >= (unsigned)_maxConnections)
				{
					_stateMutex.unlock();
					_out.printError("Error: Client connection rejected, because there are too many clients connected to me.");
					GD::bl->fileDescriptorManager.shutdown(clientFileDescriptor);
					continue;
				}
				std::shared_ptr<Client> client(new Client());
				client->id = _currentClientID++;
				client->socketDescriptor = clientFileDescriptor;
				_clients[client->id] = client;
				_stateMutex.unlock();

				try
				{
					if(_settings->ssl)
					{
						getSSLSocketDescriptor(client);
						if(!client->socketDescriptor->tlsSession)
						{
							//Remove client from _clients again. Socket is already closed.
							closeClientConnection(client);
							continue;
						}
					}
					client->socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get(), client->socketDescriptor));

					client->readThread = std::thread(&RPCServer::readClient, this, client);
					BaseLib::Threads::setThreadPriority(GD::bl.get(), client->readThread.native_handle(), _threadPriority, _threadPolicy);
				}
				catch(const std::exception& ex)
				{
					closeClientConnection(client);
					_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(BaseLib::Exception& ex)
				{
					closeClientConnection(client);
					_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(...)
				{
					closeClientConnection(client);
					_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				}
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				_stateMutex.unlock();
			}
		}
		GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
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

bool RPCServer::clientValid(std::shared_ptr<Client>& client)
{
	try
	{
		if(client->socketDescriptor->descriptor < 0) return false;
		return true;
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
    _stateMutex.unlock();
    return false;
}

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::vector<char>& data, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		if(!clientValid(client)) return;
		if(data.empty()) return;
		bool error = false;
		try
		{
			client->socket->proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			_out.printWarning("Warning: " + ex.what());
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			_out.printError("Error: " + ex.what());
			error = true;
		}
		if(!keepAlive || error) closeClientConnection(client);
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

void RPCServer::analyzeRPC(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::string methodName;
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters;
		if(packetType == PacketType::Enum::binaryRequest) parameters = _rpcDecoder->decodeRequest(packet, methodName);
		else if(packetType == PacketType::Enum::xmlRequest) parameters = _xmlRpcDecoder->decodeRequest(packet, methodName);
		if(!parameters)
		{
			_out.printWarning("Warning: Could not decode RPC packet.");
			return;
		}
		PacketType::Enum responseType = (packetType == PacketType::Enum::binaryRequest) ? PacketType::Enum::binaryResponse : PacketType::Enum::xmlResponse;
		if(!parameters->empty() && parameters->at(0)->errorStruct)
		{
			sendRPCResponseToClient(client, parameters->at(0), responseType, keepAlive);
			return;
		}
		callMethod(client, methodName, parameters, responseType, keepAlive);
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

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<BaseLib::RPC::RPCVariable> variable, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::vector<char> data;
		if(responseType == PacketType::Enum::xmlResponse)
		{
			_xmlRpcEncoder->encodeResponse(variable, data);
			std::string header = getHttpResponseHeader(data.size());
			data.push_back('\r');
			data.push_back('\n');
			data.insert(data.begin(), header.begin(), header.end());
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response packet: " + std::string(&data.at(0), data.size()));
			}
			sendRPCResponseToClient(client, data, keepAlive);
		}
		else if(responseType == PacketType::Enum::binaryResponse)
		{
			_rpcEncoder->encodeResponse(variable, data);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response binary:");
				_out.printBinary(data);
			}
			sendRPCResponseToClient(client, data, keepAlive);
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
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCServer::callMethod(std::string& methodName, std::shared_ptr<BaseLib::RPC::RPCVariable>& parameters)
{
	try
	{
		if(!parameters) parameters = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		if(_stopped) return BaseLib::RPC::RPCVariable::createError(100000, "Server is stopped.");
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			return BaseLib::RPC::RPCVariable::createError(-32601, ": Requested method not found.");
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: RPC Method called: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->arrayValue->begin(); i != parameters->arrayValue->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters->arrayValue);
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Response: ");
			ret->print();
		}
		return ret;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, ": Unknown application error.");
}

void RPCServer::callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			sendRPCResponseToClient(client, BaseLib::RPC::RPCVariable::createError(-32601, ": Requested method not found."), responseType, keepAlive);
			return;
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: Client number " + std::to_string(client->socketDescriptor->id) + " is calling RPC method: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Response: ");
			ret->print();
		}
		sendRPCResponseToClient(client, ret, responseType, keepAlive);
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

std::string RPCServer::getHttpResponseHeader(uint32_t contentLength)
{
	std::string header;
	header.append("HTTP/1.1 200 OK\r\n");
	header.append("Connection: close\r\n");
	header.append("Content-Type: text/xml\r\n");
	header.append("Content-Length: ").append(std::to_string(contentLength + 21)).append("\r\n\r\n");
	header.append("<?xml version=\"1.0\"?>");
	return header;
}

void RPCServer::analyzeRPCResponse(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::shared_ptr<BaseLib::RPC::RPCVariable> response;
		if(packetType == PacketType::Enum::binaryResponse) response = _rpcDecoder->decodeResponse(packet);
		else if(packetType == PacketType::Enum::xmlResponse) response = _xmlRpcDecoder->decodeResponse(packet);
		if(!response) return;
		if(GD::bl->debugLevel >= 3)
		{
			_out.printWarning("Warning: RPC server received RPC response. This shouldn't happen. Response data: ");
			response->print();
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
}

void RPCServer::packetReceived(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest) analyzeRPC(client, packet, packetType, keepAlive);
		else if(packetType == PacketType::Enum::binaryResponse || packetType == PacketType::Enum::xmlResponse) analyzeRPCResponse(client, packet, packetType, keepAlive);
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

void RPCServer::removeClient(int32_t clientID)
{
	try
	{
		_stateMutex.lock();
		if(_clients.find(clientID) != _clients.end())
		{
			std::shared_ptr<Client> client = _clients.at(clientID);
			_clients.erase(clientID);
			if(client->readThread.joinable()) client->readThread.detach();
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
    _stateMutex.unlock();
}

void RPCServer::readClient(std::shared_ptr<Client> client)
{
	try
	{
		if(!client) return;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		std::vector<char> packet;
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t dataSize = 0;
		PacketType::Enum packetType = PacketType::binaryRequest;
		BaseLib::HTTP http;

		_out.printDebug("Listening for incoming packets from client number " + std::to_string(client->socketDescriptor->id) + ".");
		while(!_stopServer)
		{
			try
			{
				bytesRead = client->socket->proofread(buffer, bufferMax);
				buffer[bufferMax] = 0; //Even though it shouldn't matter, make sure there is a null termination.
				//Some clients send only one byte in the first packet
				if(packetLength == 0 && bytesRead == 1) bytesRead += client->socket->proofread(&buffer[1], bufferMax - 1);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				continue;
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_out.printInfo("Info: " + ex.what());
				break;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_out.printError(ex.what());
				break;
			}

			if(!clientValid(client)) break;

			if(GD::bl->debugLevel >= 5)
			{
				std::vector<uint8_t> rawPacket;
				rawPacket.insert(rawPacket.begin(), buffer, buffer + bytesRead);
				_out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
			}
			if(!strncmp(&buffer[0], "Bin", 3))
			{
				http.reset();
				//buffer[3] & 1 is true for buffer[3] == 0xFF, too
				packetType = (buffer[3] & 1) ? PacketType::Enum::binaryResponse : PacketType::Enum::binaryRequest;
				if(bytesRead < 8) continue;
				uint32_t headerSize = 0;
				if(buffer[3] & 0x40)
				{
					GD::bl->hf.memcpyBigEndian((char*)&headerSize, buffer + 4, 4);
					if(bytesRead < (signed)headerSize + 12)
					{
						_out.printError("Error: Binary rpc packet has invalid header size.");
						continue;
					}
					GD::bl->hf.memcpyBigEndian((char*)&dataSize, buffer + 8 + headerSize, 4);
					dataSize += headerSize + 4;
				}
				else GD::bl->hf.memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				_out.printDebug("Receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
				if(dataSize == 0) continue;
				if(headerSize > 1024)
				{
					_out.printError("Error: Binary rpc packet with header larger than 1 KiB received.");
					continue;
				}
				if(dataSize > 10485760)
				{
					_out.printError("Error: Packet with data larger than 10 MiB received.");
					continue;
				}
				packet.clear();
				packet.insert(packet.end(), buffer, buffer + bytesRead);
				std::shared_ptr<BaseLib::RPC::RPCHeader> header = _rpcDecoder->decodeHeader(packet);
				if(_settings->authType == ServerSettings::Settings::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _settings->validUsers);
					try
					{
						if(!client->auth.basicServer(header))
						{
							_out.printError("Error: Authorization failed. Closing connection.");
							break;
						}
						else _out.printDebug("Client successfully authorized using basic authentification.");
					}
					catch(AuthException& ex)
					{
						_out.printError("Error: Authorization failed. Closing connection. Error was: " + ex.what());
						break;
					}
				}
				if(dataSize > (unsigned)bytesRead - 8) packetLength = bytesRead - 8;
				else
				{
					packetLength = 0;
					packetReceived(client, packet, packetType, true);
				}
			}
			else if(!strncmp(&buffer[0], "POST", 4) || !strncmp(&buffer[0], "HTTP/1.", 7))
			{
				packetType = (!strncmp(&buffer[0], "POST", 4)) ? PacketType::Enum::xmlRequest : PacketType::Enum::xmlResponse;
				//We are using string functions to process the buffer. So just to make sure,
				//they don't do something in the memory after buffer, we add '\0'
				buffer[bytesRead] = '\0';

				try
				{
					http.reset();
					http.process(buffer, bytesRead);
				}
				catch(BaseLib::HTTPException& ex)
				{
					_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
				}

				if(http.getHeader()->contentLength > 10485760)
				{
					_out.printError("Error: Packet with data larger than 10 MiB received.");
					continue;
				}

				if(_settings->authType == ServerSettings::Settings::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _settings->validUsers);
					try
					{
						if(!client->auth.basicServer(http))
						{
							_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection.");
							break;
						}
						else _out.printDebug("Client successfully authorized using basic authentification.");
					}
					catch(AuthException& ex)
					{
						_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection. Error was: " + ex.what());
						break;
					}
				}
			}
			else if(packetLength > 0 || http.dataProcessed())
			{
				if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::binaryResponse)
				{
					if(packetLength + bytesRead > dataSize)
					{
						_out.printError("Error: Packet length is wrong.");
						packetLength = 0;
						continue;
					}
					packet.insert(packet.end(), buffer, buffer + bytesRead);
					packetLength += bytesRead;
					if(packetLength == dataSize)
					{
						packet.push_back('\0');
						packetReceived(client, packet, packetType, true);
						packetLength = 0;
					}
				}
				else
				{
					try
					{
						http.process(buffer, bytesRead);
					}
					catch(BaseLib::HTTPException& ex)
					{
						_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
					}

					if(http.getContentSize() > 10485760)
					{
						http.reset();
						_out.printError("Error: Packet with data larger than 10 MiB received.");
					}
				}
			}
			else
			{
				_out.printError("Error: Uninterpretable packet received. Closing connection. Packet was: " + std::string(buffer, buffer + bytesRead));
				break;
			}
			if(http.isFinished())
			{
				packetReceived(client, *http.getContent(), packetType, http.getHeader()->connection == BaseLib::HTTP::Connection::Enum::keepAlive);
				packetLength = 0;
				http.reset();
			}
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
    //This point is only reached, when stopServer is true, the socket is closed or an error occured
	closeClientConnection(client);
}

std::shared_ptr<BaseLib::FileDescriptor> RPCServer::getClientSocketDescriptor()
{
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	try
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		GD::bl->fileDescriptorManager.lock();
		int32_t nfds = _serverFileDescriptor->descriptor + 1;
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		GD::bl->fileDescriptorManager.unlock();
		if(nfds <= 0)
		{
			GD::out.printError("Error: Server file descriptor is invalid.");
			return fileDescriptor;
		}
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout)) return fileDescriptor;

		struct sockaddr_storage clientInfo;
		socklen_t addressSize = sizeof(addressSize);
		fileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientInfo, &addressSize));
		if(!fileDescriptor) return fileDescriptor;

		getpeername(fileDescriptor->descriptor, (struct sockaddr*)&clientInfo, &addressSize);

		uint32_t port;
		char ipString[INET6_ADDRSTRLEN];
		if (clientInfo.ss_family == AF_INET) {
			struct sockaddr_in *s = (struct sockaddr_in *)&clientInfo;
			port = ntohs(s->sin_port);
			inet_ntop(AF_INET, &s->sin_addr, ipString, sizeof(ipString));
		} else { // AF_INET6
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clientInfo;
			port = ntohs(s->sin6_port);
			inet_ntop(AF_INET6, &s->sin6_addr, ipString, sizeof(ipString));
		}
		std::string ipString2(&ipString[0]);
		_out.printInfo("Info: Connection from " + ipString2 + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(fileDescriptor->id));
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
    return fileDescriptor;
}

void RPCServer::getSSLSocketDescriptor(std::shared_ptr<Client> client)
{
	try
	{
		if(!_tlsPriorityCache)
		{
			_out.printError("Error: Could not initiate TLS connection. _tlsPriorityCache is nullptr.");
			return;
		}
		if(!_x509Cred)
		{
			_out.printError("Error: Could not initiate TLS connection. _x509Cred is nullptr.");
			return;
		}
		int32_t result = 0;
		if((result = gnutls_init(&client->socketDescriptor->tlsSession, GNUTLS_SERVER)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not initialize TLS session: " + std::string(gnutls_strerror(result)));
			client->socketDescriptor->tlsSession = nullptr;
			return;
		}
		if(!client->socketDescriptor->tlsSession)
		{
			_out.printError("Error: Client TLS session is nullptr.");
			return;
		}
		if((result = gnutls_priority_set(client->socketDescriptor->tlsSession, _tlsPriorityCache)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not set cipher priority on TLS session: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			return;
		}
		if((result = gnutls_credentials_set(client->socketDescriptor->tlsSession, GNUTLS_CRD_CERTIFICATE, _x509Cred)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not set x509 credentials on TLS session: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			return;
		}
		gnutls_certificate_server_set_request(client->socketDescriptor->tlsSession, GNUTLS_CERT_IGNORE);
		if(!client->socketDescriptor || client->socketDescriptor->descriptor == -1)
		{
			_out.printError("Error setting TLS socket descriptor: Provided socket descriptor is invalid.");
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			return;
		}
		gnutls_transport_set_ptr(client->socketDescriptor->tlsSession, (gnutls_transport_ptr_t)(uintptr_t)client->socketDescriptor->descriptor);
		do
		{
			result = gnutls_handshake(client->socketDescriptor->tlsSession);
        } while (result < 0 && gnutls_error_is_fatal(result) == 0);
		if(result < 0)
		{
			_out.printError("Error: TLS handshake has failed: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			return;
		}
		return;
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
    GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
}

void RPCServer::getSocketDescriptor()
{
	try
	{
		addrinfo hostInfo;
		addrinfo *serverInfo = nullptr;

		int32_t yes = 1;

		memset(&hostInfo, 0, sizeof(hostInfo));

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;
		hostInfo.ai_flags = AI_PASSIVE;
		char buffer[100];
		std::string port = std::to_string(_settings->port);
		int32_t result;
		if((result = getaddrinfo(_settings->interface.c_str(), port.c_str(), &hostInfo, &serverInfo)) != 0)
		{
			_out.printCritical("Error: Could not get address information: " + std::string(gai_strerror(result)));
			return;
		}

		bool bound = false;
		int32_t error = 0;
		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next)
		{
			_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(info->ai_family, info->ai_socktype, info->ai_protocol));
			if(_serverFileDescriptor->descriptor == -1) continue;
			if(!(fcntl(_serverFileDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(_serverFileDescriptor->descriptor, F_SETFL, fcntl(_serverFileDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0) throw BaseLib::Exception("Error: Could not set socket options.");
			}
			if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw BaseLib::Exception("Error: Could not set socket options.");
			if(bind(_serverFileDescriptor->descriptor, info->ai_addr, info->ai_addrlen) == -1)
			{
				error = errno;
				continue;
			}
			std::string address;
			switch (info->ai_family)
			{
				case AF_INET:
					inet_ntop (info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, buffer, 100);
					address = std::string(buffer);
					break;
				case AF_INET6:
					inet_ntop (info->ai_family, &((struct sockaddr_in6 *) info->ai_addr)->sin6_addr, buffer, 100);
					address = std::string(buffer);
					break;
			}
			_out.printInfo("Info: RPC Server started listening on address " + address + " and port " + port);
			bound = true;
			break;
		}
		freeaddrinfo(serverInfo);
		if(!bound)
		{
			GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
			_out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(error)));
			return;
		}
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
			_out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(errno)));
			return;
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
}
