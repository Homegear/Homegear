/* Copyright 2013-2015 Sathya Laufer
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
	if(readThread.joinable()) readThread.detach();
}

RPCServer::RPCServer()
{
	_out.init(GD::bl.get());

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
	_xmlRpcDecoder = std::unique_ptr<BaseLib::RPC::XMLRPCDecoder>(new BaseLib::RPC::XMLRPCDecoder(GD::bl.get()));
	_xmlRpcEncoder = std::unique_ptr<BaseLib::RPC::XMLRPCEncoder>(new BaseLib::RPC::XMLRPCEncoder(GD::bl.get()));
	_jsonDecoder = std::unique_ptr<BaseLib::RPC::JsonDecoder>(new BaseLib::RPC::JsonDecoder(GD::bl.get()));
	_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));

	_info.reset(new ServerInfo::Info());
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

void RPCServer::start(std::shared_ptr<ServerInfo::Info>& info)
{
	try
	{
		stop();
		_stopServer = false;
		_info = info;
		if(!_info)
		{
			_out.printError("Error: Settings is nullptr.");
			return;
		}
		if(!_info->webServer && !_info->xmlrpcServer && !_info->jsonrpcServer) return;
		_out.setPrefix("RPC Server (Port " + std::to_string(info->port) + "): ");
		if(_info->ssl)
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
		_webServer.reset(new WebServer(_info));
		_mainThread = std::thread(&RPCServer::mainThread, this);
		BaseLib::Threads::setThreadPriority(GD::bl.get(), _mainThread.native_handle(), _threadPriority, _threadPolicy);
		_stopped = false;
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
		_out.printInfo("Info: Waiting for threads to finish.");
		while(_clients.size() > 0)
		{
			collectGarbage();
			if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
		_webServer.reset();
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
		GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
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

void RPCServer::mainThread()
{
	try
	{
		getSocketDescriptor();
		std::string address;
		int32_t port = -1;
		while(!_stopServer)
		{
			try
			{
				if(!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1)
				{
					if(_stopServer) break;
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					getSocketDescriptor();
					continue;
				}
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = getClientSocketDescriptor(address, port);
				if(!clientFileDescriptor || clientFileDescriptor->descriptor < 0) continue;
				_stateMutex.lock();
				std::shared_ptr<Client> client(new Client());
				client->id = _currentClientID++;
				client->socketDescriptor = clientFileDescriptor;
				while(_clients.find(client->id) != _clients.end())
				{
					_out.printError("Error: Client id was used twice. This shouldn't happen. Please report this error to the developer.");
					_currentClientID++;
					client->id++;
				}
				_clients[client->id] = client;
				_stateMutex.unlock();

				try
				{
					if(_info->ssl)
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
					client->address = address;
					client->port = port;

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
    GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
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
			//Sleep a tiny little bit. Some clients like the linux version of IP-Symcon don't accept responses too fast.
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			if(!keepAlive || !client->binaryPacket) std::this_thread::sleep_for(std::chrono::milliseconds(15)); //Add additional time for XMLRPC requests. Otherwise clients might not receive response.
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
		int32_t messageId = 0;
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters;
		if(packetType == PacketType::Enum::binaryRequest) parameters = _rpcDecoder->decodeRequest(packet, methodName);
		else if(packetType == PacketType::Enum::xmlRequest) parameters = _xmlRpcDecoder->decodeRequest(packet, methodName);
		else if(packetType == PacketType::Enum::jsonRequest || packetType == PacketType::Enum::webSocketRequest)
		{
			std::shared_ptr<BaseLib::RPC::Variable> result = _jsonDecoder->decode(packet);
			if(result->type == BaseLib::RPC::VariableType::rpcStruct)
			{
				if(result->structValue->find("method") == result->structValue->end())
				{
					_out.printWarning("Warning: Could not decode JSON RPC packet.");
					return;
				}
				methodName = result->structValue->at("method")->stringValue;
				if(result->structValue->find("id") != result->structValue->end()) messageId = result->structValue->at("id")->integerValue;
				if(result->structValue->find("params") != result->structValue->end()) parameters = result->structValue->at("params")->arrayValue;
				else parameters.reset(new std::vector<std::shared_ptr<BaseLib::RPC::Variable>>());
			}
		}
		if(!parameters)
		{
			_out.printWarning("Warning: Could not decode RPC packet.");
			return;
		}
		PacketType::Enum responseType = PacketType::Enum::xmlResponse;
		if(packetType == PacketType::Enum::binaryRequest) responseType = PacketType::Enum::binaryResponse;
		else if(packetType == PacketType::Enum::xmlRequest) responseType = PacketType::Enum::xmlResponse;
		else if(packetType == PacketType::Enum::jsonRequest) responseType = PacketType::Enum::jsonResponse;
		else if(packetType == PacketType::Enum::webSocketRequest) responseType = PacketType::Enum::webSocketResponse;

		if(!parameters->empty() && parameters->at(0)->errorStruct)
		{
			sendRPCResponseToClient(client, parameters->at(0), messageId, responseType, keepAlive);
			return;
		}
		callMethod(client, methodName, parameters, messageId, responseType, keepAlive);
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

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<BaseLib::RPC::Variable> variable, int32_t messageId, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::vector<char> data;
		if(responseType == PacketType::Enum::xmlResponse)
		{
			_xmlRpcEncoder->encodeResponse(variable, data);
			data.push_back('\r');
			data.push_back('\n');
			std::string header = getHttpResponseHeader("text/xml", data.size() + 21, !keepAlive);
			header.append("<?xml version=\"1.0\"?>");
			data.insert(data.begin(), header.begin(), header.end());
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response packet: " + std::string(&data.at(0), data.size()));
			}
		}
		else if(responseType == PacketType::Enum::binaryResponse)
		{
			_rpcEncoder->encodeResponse(variable, data);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response binary:");
				_out.printBinary(data);
			}
		}
		else if(responseType == PacketType::Enum::jsonResponse)
		{
			_jsonEncoder->encodeResponse(variable, messageId, data);
			data.push_back('\r');
			data.push_back('\n');
			std::string header = getHttpResponseHeader("application/json", data.size(), !keepAlive);
			data.insert(data.begin(), header.begin(), header.end());
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response packet: " + std::string(&data.at(0), data.size()));
			}
		}
		else if(responseType == PacketType::Enum::webSocketResponse)
		{
			std::vector<char> json;
			_jsonEncoder->encodeResponse(variable, messageId, json);
			BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, data);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response WebSocket packet: ");
				_out.printBinary(data);
			}
		}
		sendRPCResponseToClient(client, data, keepAlive);
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

std::shared_ptr<BaseLib::RPC::Variable> RPCServer::callMethod(std::string& methodName, std::shared_ptr<BaseLib::RPC::Variable>& parameters)
{
	try
	{
		if(!parameters) parameters = std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		if(_stopped) return BaseLib::RPC::Variable::createError(100000, "Server is stopped.");
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			return BaseLib::RPC::Variable::createError(-32601, ": Requested method not found.");
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: RPC Method called: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->arrayValue->begin(); i != parameters->arrayValue->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::Variable> ret = _rpcMethods->at(methodName)->invoke(parameters->arrayValue);
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
    return BaseLib::RPC::Variable::createError(-32500, ": Unknown application error.");
}

void RPCServer::callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters, int32_t messageId, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			sendRPCResponseToClient(client, BaseLib::RPC::Variable::createError(-32601, ": Requested method not found."), messageId, responseType, keepAlive);
			return;
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: Client number " + std::to_string(client->socketDescriptor->id) + " is calling RPC method: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::Variable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Response: ");
			ret->print();
		}
		sendRPCResponseToClient(client, ret, messageId, responseType, keepAlive);
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

std::string RPCServer::getHttpResponseHeader(std::string contentType, uint32_t contentLength, bool closeConnection)
{
	std::string header;
	header.append("HTTP/1.1 200 OK\r\n");
	header.append("Connection: ");
	header.append(closeConnection ? "close\r\n" : "Keep-Alive\r\n");
	header.append("Content-Type: " + contentType + "\r\n");
	header.append("Content-Length: ").append(std::to_string(contentLength)).append("\r\n\r\n");
	return header;
}

void RPCServer::analyzeRPCResponse(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::shared_ptr<BaseLib::RPC::Variable> response;
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
		if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest || packetType == PacketType::Enum::jsonRequest || packetType == PacketType::Enum::webSocketRequest) analyzeRPC(client, packet, packetType, keepAlive);
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

void RPCServer::collectGarbage()
{
	try
	{
		_lastGargabeCollection = GD::bl->hf.getTime();
		std::vector<std::shared_ptr<Client>> clientsToRemove;
		_stateMutex.lock();
		try
		{
			for(std::map<int32_t, std::shared_ptr<Client>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
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
		for(std::vector<std::shared_ptr<Client>>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i)
		{
			_stateMutex.lock();
			try
			{
				_clients.erase((*i)->id);
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
			_out.printDebug("Debug: Joining read thread of client " + std::to_string((*i)->id));
			if((*i)->readThread.joinable()) (*i)->readThread.join();
			_out.printDebug("Debug: Client " + std::to_string((*i)->id) + " removed.");
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

void RPCServer::handleConnectionUpgrade(std::shared_ptr<Client> client, BaseLib::HTTP& http)
{
	try
	{
		if(http.getHeader()->fields.find("upgrade") != http.getHeader()->fields.end() && http.getHeader()->fields["upgrade"] == "websocket")
		{
			if(http.getHeader()->fields.find("sec-websocket-protocol") == http.getHeader()->fields.end() && (http.getHeader()->path.empty() || http.getHeader()->path == "/"))
			{
				closeClientConnection(client);
				_out.printError("Error: No websocket protocol specified.");
				return;
			}
			if(http.getHeader()->fields.find("sec-websocket-key") == http.getHeader()->fields.end())
			{
				closeClientConnection(client);
				_out.printError("Error: No websocket key specified.");
				return;
			}
			std::string protocol = http.getHeader()->fields["sec-websocket-protocol"];
			BaseLib::HelperFunctions::toLower(protocol);
			std::string websocketKey = http.getHeader()->fields["sec-websocket-key"] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			std::vector<char> data(&websocketKey[0], &websocketKey[0] + websocketKey.size());
			std::vector<char> sha1;
			BaseLib::Crypt::sha1(data, sha1);
			std::string websocketAccept;
			BaseLib::Base64::encode(sha1, websocketAccept);
			if(protocol == "server" || http.getHeader()->path == "/server")
			{
				client->webSocket = true;
				std::string header;
				header.reserve(133 + websocketAccept.size());
				header.append("HTTP/1.1 101 Switching Protocols\r\n");
				header.append("Connection: Upgrade\r\n");
				header.append("Upgrade: websocket\r\n");
				header.append("Sec-WebSocket-Accept: ").append(websocketAccept).append("\r\n");
				if(!protocol.empty()) header.append("Sec-WebSocket-Protocol: server\r\n");
				header.append("\r\n");
				std::vector<char> data(&header[0], &header[0] + header.size());
				sendRPCResponseToClient(client, data, true);
			}
			else if(protocol == "client" || http.getHeader()->path == "/client")
			{
				client->webSocket = true;
				client->webSocketClient = true;
				std::string header;
				header.reserve(133 + websocketAccept.size());
				header.append("HTTP/1.1 101 Switching Protocols\r\n");
				header.append("Connection: Upgrade\r\n");
				header.append("Upgrade: websocket\r\n");
				header.append("Sec-WebSocket-Accept: ").append(websocketAccept).append("\r\n");
				if(!protocol.empty()) header.append("Sec-WebSocket-Protocol: client\r\n");
				header.append("\r\n");
				std::vector<char> data(&header[0], &header[0] + header.size());
				sendRPCResponseToClient(client, data, true);
				if(_info->authType != ServerInfo::Info::AuthType::basic)
				{
					_out.printInfo("Info: Transferring client number " + std::to_string(client->id) + " to rpc client.");
					GD::rpcClient.addWebSocketServer(client->socket, client->address, std::to_string(client->port));
					client->socketDescriptor.reset(new BaseLib::FileDescriptor());
					client->socket.reset(new BaseLib::SocketOperations(GD::bl.get()));
					client->closed = true;
				}
			}
			else
			{
				closeClientConnection(client);
				_out.printError("Error: Unknown websocket protocol. Known protocols are \"server\" and \"client\".");
			}
		}
		else
		{
			closeClientConnection(client);
			_out.printError("Error: Connection upgrade type not supported.");
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

void RPCServer::readClient(std::shared_ptr<Client> client)
{
	try
	{
		if(!client) return;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		//Make sure the buffer is null terminated.
		buffer[bufferMax] = '\0';
		std::vector<char> packet;
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t dataSize = 0;
		PacketType::Enum packetType = PacketType::binaryRequest;
		BaseLib::HTTP http;
		BaseLib::WebSocket webSocket;

		_out.printDebug("Listening for incoming packets from client number " + std::to_string(client->socketDescriptor->id) + ".");
		while(!_stopServer)
		{
			try
			{
				bytesRead = client->socket->proofread(buffer, bufferMax);
				buffer[bufferMax] = 0; //Even though it shouldn't matter, make sure there is a null termination.
				//Some clients send only one byte in the first packet
				if(packetLength == 0 && !http.headerProcessingStarted() && !webSocket.dataProcessingStarted() && bytesRead == 1) bytesRead += client->socket->proofread(&buffer[1], bufferMax - 1);
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
				std::vector<uint8_t> rawPacket(buffer, buffer + bytesRead);
				_out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
			}
			if(!http.headerProcessingStarted() && !webSocket.dataProcessingStarted() && packetLength == 0 && !strncmp(&buffer[0], "Bin", 3))
			{
				if(!_info->xmlrpcServer) continue;
				http.reset();
				//buffer[3] & 1 is true for buffer[3] == 0xFF, too
				packetType = (buffer[3] & 1) ? PacketType::Enum::binaryResponse : PacketType::Enum::binaryRequest;
				client->binaryPacket = true;
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
				packet.reserve(dataSize + 9);
				packet.insert(packet.end(), buffer, buffer + bytesRead);
				std::shared_ptr<BaseLib::RPC::RPCHeader> header = _rpcDecoder->decodeHeader(packet);
				if(_info->authType == ServerInfo::Info::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _info->validUsers);
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
					if(client->socketDescriptor->descriptor == -1)
					{
						if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Connection to client number " + std::to_string(client->socketDescriptor->id) + " closed.");
						break;
					}
				}
			}
			else if(!http.headerProcessingStarted() && !webSocket.dataProcessingStarted() && (!strncmp(&buffer[0], "GET ", 4) || !strncmp(&buffer[0], "HEAD ", 5)))
			{
				if(bytesRead < 8) continue;
				buffer[bytesRead] = '\0';
				packetType = PacketType::Enum::xmlRequest;

				if(!_info->redirectTo.empty())
				{
					std::vector<char> data;
					std::vector<std::string> additionalHeaders({std::string("Location: ") + _info->redirectTo});
					_webServer->getError(301, "Moved Permanently", "The document has moved <a href=\"" + _info->redirectTo + "\">here</a>.", data, additionalHeaders);
					sendRPCResponseToClient(client, data, false);
					continue;
				}
				if(!_info->webServer)
				{
					std::vector<char> data;
					_webServer->getError(400, "Bad Request", "Your client sent a request that this server could not understand.", data);
					sendRPCResponseToClient(client, data, false);
					continue;
				}

				try
				{
					http.reset();
					http.process(buffer, bytesRead);
				}
				catch(BaseLib::HTTPException& ex)
				{
					_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
					std::vector<char> data;
					_webServer->getError(400, "Bad Request", "Your client sent a request that this server could not understand.", data);
					sendRPCResponseToClient(client, data, false);
				}
			}
			else if(!http.headerProcessingStarted() && !webSocket.dataProcessingStarted() && (!strncmp(&buffer[0], "POST", 4) || !strncmp(&buffer[0], "HTTP/1.", 7)))
			{
				buffer[bytesRead] = '\0';
				packetType = (!strncmp(&buffer[0], "POST", 4)) ? PacketType::Enum::xmlRequest : PacketType::Enum::xmlResponse;

				try
				{
					http.reset();
					http.process(buffer, bytesRead);
				}
				catch(BaseLib::HTTPException& ex)
				{
					_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
				}
			}
			else if(client->webSocket && !webSocket.dataProcessingStarted())
			{
				packetType = PacketType::Enum::webSocketRequest;
				webSocket.reset();
				webSocket.process(buffer, bytesRead);
			}
			else if(packetLength > 0 || http.headerProcessingStarted() || webSocket.dataProcessingStarted())
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
						if(client->socketDescriptor->descriptor == -1)
						{
							if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Connection to client number " + std::to_string(client->socketDescriptor->id) + " closed.");
							break;
						}
					}
				}
				else
				{
					buffer[bytesRead] = '\0';
					if(client->webSocket) webSocket.process(buffer, bytesRead);
					else
					{
						try
						{
							http.process(buffer, bytesRead);
						}
						catch(BaseLib::HTTPException& ex)
						{
							_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
							http.reset();
							std::vector<char> data;
							_webServer->getError(400, "Bad Request", "Your client sent a request that the server couldn't understand..", data);
							sendRPCResponseToClient(client, data, false);
						}

						if(http.getContentSize() > 10485760)
						{
							http.reset();
							std::vector<char> data;
							_webServer->getError(400, "Bad Request", "Your client sent a request larger than 10 MiB.", data);
							sendRPCResponseToClient(client, data, false);
						}
					}
				}
			}
			else
			{
				_out.printError("Error: Uninterpretable packet received. Closing connection. Packet was: " + std::string(buffer, bytesRead));
				break;
			}
			if(client->webSocket && webSocket.isFinished())
			{
				if(webSocket.getHeader()->close)
				{
					std::vector<char> response;
					webSocket.encode(*webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::close, response);
					sendRPCResponseToClient(client, response, false);
					closeClientConnection(client);
				}
				else if(_info->authType == ServerInfo::Info::AuthType::basic && !client->webSocketAuthorized)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _info->validUsers);
					try
					{
						if(!client->auth.basicServer(webSocket))
						{
							_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection.");
							std::vector <char> output;
							BaseLib::WebSocket::encodeClose(output);
							sendRPCResponseToClient(client, output, false);
							break;
						}
						else
						{
							client->webSocketAuthorized = true;
							_out.printDebug("Client successfully authorized using basic authentification.");
							if(client->webSocketClient)
							{
								_out.printInfo("Info: Transferring client number " + std::to_string(client->id) + " to rpc client.");
								GD::rpcClient.addWebSocketServer(client->socket, client->address, std::to_string(client->port));
								client->socketDescriptor.reset(new BaseLib::FileDescriptor());
								client->socket.reset(new BaseLib::SocketOperations(GD::bl.get()));
								client->closed = true;
								break;
							}
						}
					}
					catch(AuthException& ex)
					{
						_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection. Error was: " + ex.what());
						break;
					}
				}
				else if(webSocket.getHeader()->opcode == BaseLib::WebSocket::Header::Opcode::ping)
				{
					std::vector<char> response;
					webSocket.encode(*webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::pong, response);
					sendRPCResponseToClient(client, response, false);
				}
				else
				{
					packetReceived(client, *webSocket.getContent(), packetType, true);
				}
				webSocket.reset();
			}
			else if(http.isFinished())
			{
				if(_info->webSocket && (http.getHeader()->connection & BaseLib::HTTP::Connection::upgrade))
				{
					//Do this before basic auth, because currently basic auth is not supported by websockets. Authorization takes place after the upgrade.
					handleConnectionUpgrade(client, http);
					if(client->closed) break; //No auth and client transferred.
					packetLength = 0;
					http.reset();
					continue;
				}

				if(_info->authType == ServerInfo::Info::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _info->validUsers);
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
				else if(_info->webServer && (!_info->xmlrpcServer || http.getHeader()->contentType != "text/xml") && (!_info->jsonrpcServer || http.getHeader()->contentType != "application/json"))
				{

					http.getHeader()->remoteAddress = client->address;
					http.getHeader()->remotePort = client->port;
					std::vector<char> response;
					if(http.getHeader()->method == "POST") _webServer->post(http, response);
					else if(http.getHeader()->method == "GET" || http.getHeader()->method == "HEAD") _webServer->get(http, response);
					sendRPCResponseToClient(client, response, false);
				}
				else if(http.getContentSize() > 0 && (_info->xmlrpcServer || _info->jsonrpcServer))
				{
					if(http.getHeader()->contentType == "application/json" || http.getContent()->at(0) == '{') packetType = PacketType::jsonRequest;
					packetReceived(client, *http.getContent(), packetType, http.getHeader()->connection & BaseLib::HTTP::Connection::Enum::keepAlive);
				}
				packetLength = 0;
				http.reset();
				if(client->socketDescriptor->descriptor == -1)
				{
					if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Connection to client number " + std::to_string(client->socketDescriptor->id) + " closed.");
					break;
				}
			}
		}
		if(client->webSocket) //Send close packet
		{
			std::vector<char> payload;
			std::vector<char> response;
			BaseLib::WebSocket::encode(payload, BaseLib::WebSocket::Header::Opcode::close, response);
			sendRPCResponseToClient(client, response, false);
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

std::shared_ptr<BaseLib::FileDescriptor> RPCServer::getClientSocketDescriptor(std::string& address, int32_t& port)
{
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	try
	{
		_stateMutex.lock();
		if(_clients.size() > GD::bl->settings.rpcServerMaxConnections())
		{
			_stateMutex.unlock();
			collectGarbage();
			if(_clients.size() > GD::bl->settings.rpcServerMaxConnections())
			{
				_out.printError("Error: There are too many clients connected to me. Waiting for connections to close. You can increase the number of allowed connections in main.conf.");
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				return fileDescriptor;
			}
		}
		_stateMutex.unlock();

		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		GD::bl->fileDescriptorManager.lock();
		int32_t nfds = _serverFileDescriptor->descriptor + 1;
		if(nfds <= 0)
		{
			GD::bl->fileDescriptorManager.unlock();
			GD::out.printError("Error: Server file descriptor is invalid.");
			return fileDescriptor;
		}
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		GD::bl->fileDescriptorManager.unlock();
		if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout))
		{
			if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.rpcServerMaxConnections() * 100 / 112) collectGarbage();
			return fileDescriptor;
		}

		struct sockaddr_storage clientInfo;
		socklen_t addressSize = sizeof(addressSize);
		fileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientInfo, &addressSize));
		if(!fileDescriptor) return fileDescriptor;

		getpeername(fileDescriptor->descriptor, (struct sockaddr*)&clientInfo, &addressSize);

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
		address = std::string(&ipString[0]);

		_out.printInfo("Info: Connection from " + address + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(fileDescriptor->id));
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
		std::string port = std::to_string(_info->port);
		int32_t result;
		if((result = getaddrinfo(_info->interface.c_str(), port.c_str(), &hostInfo, &serverInfo)) != 0)
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
			switch (info->ai_family)
			{
				case AF_INET:
					inet_ntop (info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, buffer, 100);
					_info->address = std::string(buffer);
					break;
				case AF_INET6:
					inet_ntop (info->ai_family, &((struct sockaddr_in6 *) info->ai_addr)->sin6_addr, buffer, 100);
					_info->address = std::string(buffer);
					break;
			}
			_out.printInfo("Info: RPC Server started listening on address " + _info->address + " and port " + port);
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
