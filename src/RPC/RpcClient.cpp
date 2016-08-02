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

#include "RpcClient.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace RPC
{

RpcClient::RpcClient()
{
	try
	{
		signal(SIGPIPE, SIG_IGN);

		_out.init(GD::bl.get());
		_out.setPrefix("RPC client: ");
		_out.setErrorCallback(nullptr); //Avoid endless loops

		if(!GD::bl) _out.printCritical("Critical: Can't initialize RPC client, because base library is not initialized.");
		_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
		_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
		_xmlRpcDecoder = std::unique_ptr<BaseLib::RPC::XMLRPCDecoder>(new BaseLib::RPC::XMLRPCDecoder(GD::bl.get()));
		_xmlRpcEncoder = std::unique_ptr<BaseLib::RPC::XMLRPCEncoder>(new BaseLib::RPC::XMLRPCEncoder(GD::bl.get()));
		_jsonDecoder = std::unique_ptr<BaseLib::RPC::JsonDecoder>(new BaseLib::RPC::JsonDecoder(GD::bl.get()));
		_jsonEncoder = std::unique_ptr<BaseLib::RPC::JsonEncoder>(new BaseLib::RPC::JsonEncoder(GD::bl.get()));
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

RpcClient::~RpcClient()
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

void RpcClient::invokeBroadcast(RemoteRpcServer* server, std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>> parameters)
{
	try
	{
		if(methodName.empty())
		{
			_out.printError("Error: Could not invoke RPC method for server " + server->hostname + ". methodName is empty.");
			return;
		}
		if(!server)
		{
			_out.printError("RPC Client: Could not send packet. Pointer to server is nullptr.");
			return;
		}
		server->sendMutex.lock();
		_out.printInfo("Info: Calling RPC method \"" + methodName + "\" on server " + (server->hostname.empty() ? server->address.first : server->hostname) + ".");
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Parameters:");
			for(std::list<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print(true, false);
			}
		}
		bool retry = false;
		std::vector<char> requestData;
		std::vector<char> responseData;
		if(server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
		else if(server->webSocket)
		{
			std::vector<char> json;
			_jsonEncoder->encodeRequest(methodName, parameters, json);
			BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, requestData);
		}
		else if(server->json) _jsonEncoder->encodeRequest(methodName, parameters, requestData);
		else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
		for(uint32_t i = 0; i < 3; ++i)
		{
			retry = false;
			if(i == 0) sendRequest(server, requestData, responseData, true, retry);
			else sendRequest(server, requestData, responseData, false, retry);
			if(!retry || server->removed || !server->autoConnect) break;
		}
		if(server->removed)
		{
			server->sendMutex.unlock();
			return;
		}
		if(retry && !server->reconnectInfinitely)
		{
			if(!server->webSocket) _out.printError("Removing server \"" + server->id + "\". Server has to send \"init\" again.");
			server->removed = true;
			server->sendMutex.unlock();
			return;
		}
		if(responseData.empty())
		{
			if(server->webSocket)
			{
				server->removed = true;
				server->sendMutex.unlock();
				_out.printInfo("Info: Connection to server closed. Host: " + server->hostname);
			}
			else
			{
				server->sendMutex.unlock();
				_out.printWarning("Warning: Response is empty. RPC method: " + methodName + " Server: " + server->hostname);
			}
			return;
		}
		BaseLib::PVariable returnValue;
		if(server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
		else if(server->webSocket || server->json) returnValue = _jsonDecoder->decode(responseData);
		else returnValue = _xmlRpcDecoder->decodeResponse(responseData);

		if(returnValue->errorStruct) _out.printError("Error in RPC response from " + server->hostname + ". faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response was:");
				returnValue->print(true, false);
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
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
    server->sendMutex.unlock();
}

BaseLib::PVariable RpcClient::invoke(std::shared_ptr<RemoteRpcServer> server, std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>> parameters)
{
	try
	{
		if(methodName.empty()) return BaseLib::Variable::createError(-32601, "Method name is empty");
		if(!server) return BaseLib::Variable::createError(-32500, "Could not send packet. Pointer to server is nullptr.");
		server->sendMutex.lock();
		if(server->removed)
		{
			server->sendMutex.unlock();
			return BaseLib::Variable::createError(-32300, "Server was removed and has to send \"init\" again.");;
		}
		_out.printInfo("Info: Calling RPC method \"" + methodName + "\" on server " + (server->hostname.empty() ? server->address.first : server->hostname) + ".");
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Parameters:");
			for(std::list<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print(true, false);
			}
		}
		bool retry = false;
		std::vector<char> requestData;
		std::vector<char> responseData;
		if(server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
		else if(server->webSocket)
		{
			std::vector<char> json;
			_jsonEncoder->encodeRequest(methodName, parameters, json);
			BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, requestData);
		}
		else if(server->json) _jsonEncoder->encodeRequest(methodName, parameters, requestData);
		else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
		for(uint32_t i = 0; i < 3; ++i)
		{
			retry = false;
			if(i == 0) sendRequest(server.get(), requestData, responseData, true, retry);
			else
			{
				_out.printWarning("Warning: Retry " + std::to_string(i) + ": Calling RPC method \"" + methodName + "\" on server " + (server->hostname.empty() ? server->address.first : server->hostname) + ".");
				sendRequest(server.get(), requestData, responseData, false, retry);
			}
			if(!retry || server->removed || !server->autoConnect) break;
		}
		if(server->removed)
		{
			server->sendMutex.unlock();
			return BaseLib::Variable::createError(-32300, "Server was removed and has to send \"init\" again.");
		}
		if(retry && !server->reconnectInfinitely)
		{
			if(!server->webSocket) _out.printError("Removing server \"" + server->id + "\". Server has to send \"init\" again.");
			server->removed = true;
			server->sendMutex.unlock();
			return BaseLib::Variable::createError(-32300, "Request timed out.");
		}
		if(responseData.empty())
		{
			if(server->webSocket)
			{
				server->removed = true;
				server->sendMutex.unlock();
				_out.printInfo("Info: Connection to server closed. Host: " + server->hostname);
			}
			else server->sendMutex.unlock();
			return BaseLib::Variable::createError(-32700, "No response data.");
		}
		BaseLib::PVariable returnValue;
		if(server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
		else if(server->webSocket || server->json) returnValue = _jsonDecoder->decode(responseData);
		else returnValue = _xmlRpcDecoder->decodeResponse(responseData);
		if(returnValue->errorStruct) _out.printError("Error in RPC response from " + server->hostname + ". faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response was:");
				returnValue->print(true, false);
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
		}

		server->sendMutex.unlock();
		return returnValue;
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
    server->sendMutex.unlock();
    return BaseLib::Variable::createError(-32700, "No response data.");
}

std::string RpcClient::getIPAddress(std::string address)
{
	try
	{
		if(address.size() < 10)
		{
			_out.printError("Error: Server's address too short: " + address);
			return "";
		}
		if(address.compare(0, 7, "http://") == 0) address = address.substr(7);
		else if(address.compare(0, 8, "https://") == 0) address = address.substr(8);
		else if(address.compare(0, 9, "binary://") == 0) address = address.substr(9);
		else if(address.size() > 10 && address.compare(0, 10, "binarys://") == 0) address = address.substr(10);
		else if(address.size() > 13 && address.compare(0, 13, "xmlrpc_bin://") == 0) address = address.substr(13);
		if(address.empty())
		{
			_out.printError("Error: Server's address is empty.");
			return "";
		}
		//Remove "[" and "]" of IPv6 address
		if(address.front() == '[' && address.back() == ']') address = address.substr(1, address.size() - 2);
		if(address.empty())
		{
			_out.printError("Error: Server's address is empty.");
			return "";
		}

		if(GD::bl->settings.tunnelClients().find(address) != GD::bl->settings.tunnelClients().end()) return "localhost";
		return address;
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
    return "";
}

void RpcClient::sendRequest(RemoteRpcServer* server, std::vector<char>& data, std::vector<char>& responseData, bool insertHeader, bool& retry)
{
	try
	{
		if(!server)
		{
			_out.printError("RPC Client: Could not send packet. Pointer to server or data is nullptr.");
			return;
		}
		if(server->removed) return;

		if(server->autoConnect)
		{
			if(server->hostname.empty()) server->hostname = getIPAddress(server->address.first);
			if(server->hostname.empty())
			{
				_out.printError("RPC Client: Error: hostname is empty.");
				server->removed = true;
				return;
			}
			//Get settings pointer every time this method is executed, because
			//the settings might change.
			server->settings = GD::clientSettings.get(server->hostname);
			if(server->settings) _out.printDebug("Debug: Settings found for host " + server->hostname);
			if(!server->useSSL && server->settings && server->settings->forceSSL)
			{
				_out.printError("RPC Client: Tried to send unencrypted packet to " + server->hostname + " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again.");
				server->removed = true;
				return;
			}
		}

		try
		{
			if(!server->socket->connected())
			{
				if(server->autoConnect)
				{
					server->socket->setHostname(server->hostname);
					server->socket->setPort(server->address.second);
					if(server->settings)
					{
						server->socket->setCAFile(server->settings->caFile);
						server->socket->setVerifyCertificate(server->settings->verifyCertificate);
					}
					server->socket->setUseSSL(server->useSSL);
					server->socket->open();
				}
				else
				{
					_out.printError("Connection to server with id " + std::to_string(server->uid) + " closed. Removing server.");
					server->removed = true;
					return;
				}
			}
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			if(!server->reconnectInfinitely) server->removed = true;
			GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
			_out.printError(ex.what() + " Removing server. Server has to send \"init\" again.");
			return;
		}


		if(server->settings && server->settings->authType != ClientSettings::Settings::AuthType::none && !server->auth.initialized())
		{
			if(server->settings->userName.empty() || server->settings->password.empty())
			{
				server->socket->close();
				_out.printError("Error: No user name or password specified in config file for RPC server " + server->hostname + ". Closing connection.");
				return;
			}
			server->auth = Auth(server->socket, server->settings->userName, server->settings->password);
		}

		if(insertHeader)
		{
			if(server->binary)
			{
				BaseLib::RPC::RPCHeader header;
				if(server->settings && server->settings->authType == ClientSettings::Settings::AuthType::basic)
				{
					_out.printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header.authorization = authField.second;
				}
				_rpcEncoder->insertHeader(data, header);
			}
			else if(server->webSocket) {}
			else //XML-RPC, JSON-RPC
			{
				data.push_back('\r');
				data.push_back('\n');
				std::string header = "POST " + server->path + " HTTP/1.1\r\nUser-Agent: Homegear " + std::string(VERSION) + "\r\nHost: " + server->hostname + ":" + server->address.second + "\r\nContent-Type: " + (server->json ? "application/json" : "text/xml") + "\r\nContent-Length: " + std::to_string(data.size()) + "\r\nConnection: " + (server->keepAlive ? "Keep-Alive" : "close") + "\r\n";
				if(server->settings && server->settings->authType == ClientSettings::Settings::AuthType::basic)
				{
					_out.printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header += authField.first + ": " + authField.second + "\r\n";
				}
				header += "\r\n";
				data.insert(data.begin(), header.begin(), header.end());

			}
		}

		if(GD::bl->debugLevel >= 5)
		{
			if(server->binary || server->webSocket) _out.printDebug("Debug: Sending packet: " + GD::bl->hf.getHexString(data));
			else _out.printDebug("Debug: Sending packet: " + std::string(&data.at(0), data.size()));
		}
		try
		{
			server->socket->proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			server->socket->close();
			_out.printWarning("Warning: " + ex.what());
			return;
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			retry = true;
			server->socket->close();
			_out.printError("Error: Could not send data to XML RPC server " + server->hostname + ": " + ex.what() + ".");
			return;
		}
	}
    catch(const std::exception& ex)
    {
		if(!server->reconnectInfinitely) server->removed = true;
		else retry = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_out.printError("Removing server. Server has to send \"init\" again.");
    	return;
    }
    catch(BaseLib::Exception& ex)
    {
		if(!server->reconnectInfinitely) server->removed = true;
		else retry = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_out.printError("Removing server. Server has to send \"init\" again.");
    	return;
    }
    catch(...)
    {
		if(!server->reconnectInfinitely) server->removed = true;
		else retry = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	_out.printError("Removing server. Server has to send \"init\" again.");
    	return;
    }

    //Receive response
	try
	{
		responseData.clear();
		ssize_t receivedBytes = 0;

		int32_t bufferMax = 2048;
		char buffer[bufferMax + 1];
		BaseLib::Rpc::BinaryRpc binaryRpc(GD::bl.get());
		BaseLib::Http http;
		BaseLib::WebSocket webSocket;

		while(!binaryRpc.isFinished() && !http.isFinished() && !webSocket.isFinished()) //This is equal to while(true) for binary packets
		{
			try
			{
				receivedBytes = server->socket->proofread(buffer, bufferMax);
				//Some clients send only one byte in the first packet
				if(receivedBytes == 1 && !binaryRpc.processingStarted() && !http.headerIsFinished() && !webSocket.dataProcessingStarted()) receivedBytes += server->socket->proofread(&buffer[1], bufferMax - 1);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				_out.printInfo("Info: Reading from RPC server timed out. Server: " + server->hostname);
				retry = true;
				if(!server->keepAlive) server->socket->close();
				return;
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				retry = true;
				if(!server->keepAlive) server->socket->close();
				_out.printWarning("Warning: " + ex.what());
				return;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				retry = true;
				if(!server->keepAlive) server->socket->close();
				_out.printError(ex.what());
				return;
			}

			//We are using string functions to process the buffer. So just to make sure,
			//they don't do something in the memory after buffer, we add '\0'
			buffer[receivedBytes] = '\0';
			if(GD::bl->debugLevel >= 5)
			{
				std::vector<uint8_t> rawPacket(buffer, buffer + receivedBytes);
				_out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
			}

			if(server->binary)
			{
				try
				{
					int32_t processedBytes = binaryRpc.process(&buffer[0], receivedBytes);
					if(processedBytes < receivedBytes)
					{
						_out.printError("Received more bytes (" + std::to_string(receivedBytes) + ") than binary packet size (" + std::to_string(processedBytes) + ").");
					}
				}
				catch(BaseLib::Rpc::BinaryRpcException& ex)
				{
					if(!server->keepAlive) server->socket->close();
					_out.printError("Error processing packet: " + ex.what());
					return;
				}
			}
			else if(server->webSocket)
			{
				try
				{
					webSocket.process(buffer, receivedBytes); //Check for chunked packets (HomeMatic Manager, ioBroker). Necessary, because HTTP header does not contain transfer-encoding.
				}
				catch(BaseLib::WebSocketException& ex)
				{
					if(!server->keepAlive) server->socket->close();
					_out.printError("RPC Client: Could not process WebSocket packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
					return;
				}
				if(http.getContentSize() > 10485760 || http.getHeader().contentLength > 10485760)
				{
					if(!server->keepAlive) server->socket->close();
					_out.printError("Error: Packet with data larger than 100 MiB received.");
					return;
				}
				if(webSocket.isFinished() && webSocket.getHeader().opcode == BaseLib::WebSocket::Header::Opcode::ping)
				{
					_out.printInfo("Info: Websocket ping received.");
					std::vector<char> pong;
					webSocket.encode(webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::pong, pong);
					try
					{
						server->socket->proofwrite(pong);
					}
					catch(BaseLib::SocketDataLimitException& ex)
					{
						server->socket->close();
						_out.printWarning("Warning: " + ex.what());
						return;
					}
					catch(const BaseLib::SocketOperationException& ex)
					{
						retry = true;
						server->socket->close();
						_out.printError("Error: Could not send data to XML RPC server " + server->hostname + ": " + ex.what() + ".");
						return;
					}
					webSocket = BaseLib::WebSocket();
				}
			}
			else
			{
				if(!http.headerIsFinished())
				{
					if(!strncmp(buffer, "401", 3) || !strncmp(&buffer[9], "401", 3)) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
					{
						if(!server->keepAlive) server->socket->close();
						_out.printError("Error: Authentication failed. Server " + server->hostname + ". Check user name and password in rpcclients.conf.");
						return;
					}
				}

				try
				{
					http.process(buffer, receivedBytes, !server->json, server->json); //Check for chunked packets (HomeMatic Manager, ioBroker). Necessary, because HTTP header does not contain transfer-encoding.
				}
				catch(BaseLib::HttpException& ex)
				{
					if(!server->keepAlive) server->socket->close();
					_out.printError("XML RPC Client: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
					return;
				}
				if(http.getContentSize() > 10485760 || http.getHeader().contentLength > 10485760)
				{
					if(!server->keepAlive) server->socket->close();
					_out.printError("Error: Packet with data larger than 100 MiB received.");
					return;
				}
			}
		}
		if(!server->keepAlive) server->socket->close();
		if(GD::bl->debugLevel >= 5)
		{
			if(server->binary) _out.printDebug("Debug: Received packet from server " + server->hostname + ": " + GD::bl->hf.getHexString(binaryRpc.getData()));
			else if(http.isFinished() && !http.getContent().empty()) _out.printDebug("Debug: Received packet from server " + server->hostname + ":\n" + std::string(&http.getContent().at(0), http.getContent().size()));
			else if(webSocket.isFinished() && !webSocket.getContent().empty()) _out.printDebug("Debug: Received packet from server " + server->hostname + ":\n" + std::string(&webSocket.getContent().at(0), webSocket.getContent().size()));
		}
		if(server->binary && binaryRpc.isFinished()) responseData = std::move(binaryRpc.getData());
		else if(server->webSocket && webSocket.isFinished()) responseData = std::move(webSocket.getContent());
		else if(http.isFinished()) responseData = std::move(http.getContent());
    }
    catch(const std::exception& ex)
    {
    	if(!server->reconnectInfinitely) server->removed = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_out.printError("Removing server. Server has to send \"init\" again.");
    }
    catch(BaseLib::Exception& ex)
    {
    	if(!server->reconnectInfinitely) server->removed = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_out.printError("Removing server. Server has to send \"init\" again.");
    }
    catch(...)
    {
    	if(!server->reconnectInfinitely) server->removed = true;
    	GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	_out.printError("Removing server. Server has to send \"init\" again.");
    }
}
}
