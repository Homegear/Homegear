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

#include "RPCClient.h"
#include "../../Version.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

namespace RPC
{
RemoteRPCServer::RemoteRPCServer()
{
	socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get()));
	knownDevices.reset(new std::set<uint64_t>());
	fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	path = "/RPC2";
}

RPCClient::RPCClient()
{
	try
	{
		signal(SIGPIPE, SIG_IGN);

		if(!GD::bl) GD::out.printCritical("Critical: Can't initialize RPC client, because base library is not initialized.");
		_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
		_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
		_xmlRpcDecoder = std::unique_ptr<BaseLib::RPC::XMLRPCDecoder>(new BaseLib::RPC::XMLRPCDecoder(GD::bl.get()));
		_xmlRpcEncoder = std::unique_ptr<BaseLib::RPC::XMLRPCEncoder>(new BaseLib::RPC::XMLRPCEncoder(GD::bl.get()));
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

RPCClient::~RPCClient()
{
	try
	{

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

void RPCClient::invokeBroadcast(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(methodName.empty())
		{
			GD::out.printError("Error: Could not invoke XML RPC method for server " + server->address.first + ". methodName is empty.");
			return;
		}
		if(!server)
		{
			GD::out.printError("RPC Client: Could not send packet. Pointer to server is nullptr.");
			return;
		}
		server->sendMutex.lock();
		GD::out.printInfo("Info: Calling XML RPC method \"" + methodName + "\" on server " + server->address.first + " and port " + server->address.second + ".");
		if(GD::bl->debugLevel >= 5)
		{
			GD::out.printDebug("Parameters:");
			for(std::list<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::vector<char> requestData;
		std::vector<char> responseData;
		if(server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
		else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
		for(uint32_t i = 0; i < 3; ++i)
		{
			timedout = false;
			if(i == 0) sendRequest(server, requestData, responseData, true, timedout);
			else sendRequest(server, requestData, responseData, false, timedout);
			if(!timedout || server->removed) break;
		}
		if(server->removed)
		{
			server->sendMutex.unlock();
			GD::rpcClient.removeServer(server->address);
			return;
		}
		if(timedout)
		{
			GD::out.printError("Removing server \"" + server->id + "\". Server has to send \"init\" again.");
			server->removed = true;
			server->sendMutex.unlock();
			GD::rpcClient.removeServer(server->address);
			return;
		}
		if(responseData.empty())
		{
			GD::out.printWarning("Warning: Response is empty. XML RPC method: " + methodName + " Server: " + server->address.first + " Port: " + server->address.second);
			server->sendMutex.unlock();
			return;
		}
		std::shared_ptr<BaseLib::RPC::Variable> returnValue;
		if(server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
		else returnValue = _xmlRpcDecoder->decodeResponse(responseData);

		if(returnValue->errorStruct) GD::out.printError("Error in RPC response from " + server->hostname + " on port " + server->address.second + ": faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(GD::bl->debugLevel >= 5)
			{
				GD::out.printDebug("Response was:");
				returnValue->print();
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
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
    server->sendMutex.unlock();
}

std::shared_ptr<BaseLib::RPC::Variable> RPCClient::invoke(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(methodName.empty()) return BaseLib::RPC::Variable::createError(-32601, "Method name is empty");
		if(!server) return BaseLib::RPC::Variable::createError(-32500, "Could not send packet. Pointer to server is nullptr.");
		server->sendMutex.lock();
		if(server->removed)
		{
			server->sendMutex.unlock();
			return BaseLib::RPC::Variable::createError(-32300, "Server was removed and has to send \"init\" again.");;
		}
		GD::out.printInfo("Info: Calling XML RPC method \"" + methodName + "\" on server " + server->address.first + " and port " + server->address.second + ".");
		if(GD::bl->debugLevel >= 5)
		{
			GD::out.printDebug("Parameters:");
			for(std::list<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::vector<char> requestData;
		std::vector<char> responseData;
		if(server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
		else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
		for(uint32_t i = 0; i < 3; ++i)
		{
			timedout = false;
			if(i == 0) sendRequest(server, requestData, responseData, true, timedout);
			else sendRequest(server, requestData, responseData, false, timedout);
			if(!timedout || server->removed) break;
		}
		if(server->removed)
		{
			server->sendMutex.unlock();
			GD::rpcClient.removeServer(server->address);
			return BaseLib::RPC::Variable::createError(-32300, "Server was removed and has to send \"init\" again.");
		}
		if(timedout)
		{
			GD::out.printError("Removing server \"" + server->id + "\". Server has to send \"init\" again.");
			server->removed = true;
			server->sendMutex.unlock();
			GD::rpcClient.removeServer(server->address);
			return BaseLib::RPC::Variable::createError(-32300, "Request timed out.");
		}
		if(responseData.empty())
		{
			server->sendMutex.unlock();
			return BaseLib::RPC::Variable::createError(-32700, "No response data.");
		}
		std::shared_ptr<BaseLib::RPC::Variable> returnValue;
		if(server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
		else returnValue = _xmlRpcDecoder->decodeResponse(responseData);
		if(returnValue->errorStruct) GD::out.printError("Error in RPC response from " + server->hostname + " on port " + server->address.second + ": faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(GD::bl->debugLevel >= 5)
			{
				GD::out.printDebug("Response was:");
				returnValue->print();
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
		}

		server->sendMutex.unlock();
		return returnValue;
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
    server->sendMutex.unlock();
    return BaseLib::RPC::Variable::createError(-32700, "No response data.");
}

std::string RPCClient::getIPAddress(std::string address)
{
	try
	{
		if(address.size() < 10)
		{
			GD::out.printError("Error: Server's address too short: " + address);
			return "";
		}
		if(address.compare(0, 7, "http://") == 0) address = address.substr(7);
		else if(address.compare(0, 8, "https://") == 0) address = address.substr(8);
		else if(address.compare(0, 9, "binary://") == 0) address = address.substr(9);
		else if(address.size() > 10 && address.compare(0, 10, "binarys://") == 0) address = address.substr(10);
		else if(address.size() > 13 && address.compare(0, 13, "xmlrpc_bin://") == 0) address = address.substr(13);
		if(address.empty())
		{
			GD::out.printError("Error: Server's address is empty.");
			return "";
		}
		//Remove "[" and "]" of IPv6 address
		if(address.front() == '[' && address.back() == ']') address = address.substr(1, address.size() - 2);
		if(address.empty())
		{
			GD::out.printError("Error: Server's address is empty.");
			return "";
		}

		if(GD::bl->settings.tunnelClients().find(address) != GD::bl->settings.tunnelClients().end()) return "localhost";
		return address;
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

void RPCClient::sendRequest(std::shared_ptr<RemoteRPCServer> server, std::vector<char>& data, std::vector<char>& responseData, bool insertHeader, bool& timedout)
{
	try
	{
		if(!server)
		{
			GD::out.printError("RPC Client: Could not send packet. Pointer to server or data is nullptr.");
			return;
		}
		if(server->removed) return;

		if(server->hostname.empty()) server->hostname = getIPAddress(server->address.first);
		if(server->hostname.empty()) return;
		//Get settings pointer every time this method is executed, because
		//the settings might change.
		server->settings = GD::clientSettings.get(server->hostname);
		if(server->settings) GD::out.printDebug("Debug: Settings found for host " + server->hostname);
		if(!server->useSSL && server->settings && server->settings->forceSSL)
		{
			GD::out.printError("RPC Client: Tried to send unencrypted packet to " + server->hostname + " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again.");
			server->removed = true;
			return;
		}

		_sendCounter++;
		if(_sendCounter > (signed)GD::bl->settings.rpcClientThreadMax())
		{
			GD::out.printCritical("Critical: Could not execute XML RPC method on server " + server->address.first + " and port " + server->address.second + ", because there are more than " + std::to_string(GD::bl->settings.rpcClientThreadMax()) + " requests queued. Your server is either not reachable currently or your connection is too slow.", false);
			_sendCounter--;
			return;
		}

		try
		{
			if(!server->socket->connected())
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
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			GD::out.printError(ex.what() + " Removing server. Server has to send \"init\" again.");
			server->removed = true;
			_sendCounter--;
			return;
		}


		if(server->settings && server->settings->authType != ClientSettings::Settings::AuthType::none && !server->auth.initialized())
		{
			if(server->settings->userName.empty() || server->settings->password.empty())
			{
				GD::out.printError("Error: No user name or password specified in config file for XML RPC server " + server->hostname + " on port " + server->address.second + ". Closing connection.");
				server->socket->close();
				_sendCounter--;
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
					GD::out.printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header.authorization = authField.second;
				}
				_rpcEncoder->insertHeader(data, header);
			}
			else
			{
				data.push_back('\r');
				data.push_back('\n');
				std::string header = "POST " + server->path + " HTTP/1.1\r\nUser-Agent: Homegear " + std::string(VERSION) + "\r\nHost: " + server->hostname + ":" + server->address.second + "\r\nContent-Type: text/xml\r\nContent-Length: " + std::to_string(data.size()) + "\r\nConnection: " + (server->keepAlive ? "Keep-Alive" : "close") + "\r\n";
				if(server->settings && server->settings->authType == ClientSettings::Settings::AuthType::basic)
				{
					GD::out.printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header += authField.first + ": " + authField.second + "\r\n";
				}
				header += "\r\n";
				data.insert(data.begin(), header.begin(), header.end());

			}
		}

		if(GD::bl->debugLevel >= 5)
		{
			if(server->binary) GD::out.printDebug("Sending packet: " + GD::bl->hf.getHexString(data));
			else GD::out.printDebug("Sending packet: " + std::string(&data.at(0), data.size()));
		}
		try
		{
			server->socket->proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			GD::out.printWarning("Warning: " + ex.what());
			server->socket->close();
			_sendCounter--;
			return;
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			GD::out.printError("Error: Could not send data to XML RPC server " + server->hostname + " on port " + server->address.second + ": " + ex.what() + ". Giving up.");
			server->socket->close();
			_sendCounter--;
			return;
		}

		responseData.clear();
		ssize_t receivedBytes;

		int32_t bufferMax = 2048;
		char buffer[bufferMax + 1];
		BaseLib::HTTP http;
		uint32_t packetLength = 0;
		uint32_t dataSize = 0;

		while(!http.isFinished()) //This is equal to while(true) for binary packets
		{
			try
			{
				receivedBytes = server->socket->proofread(buffer, bufferMax);

				//Some clients send only one byte in the first packet
				if(packetLength == 0 && receivedBytes == 1 && !http.headerIsFinished()) receivedBytes += server->socket->proofread(&buffer[1], bufferMax - 1);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				GD::out.printInfo("Info: Reading from XML RPC server timed out. Server: " + server->hostname + " Port: " + server->address.second);
				timedout = true;
				if(!server->keepAlive) server->socket->close();
				_sendCounter--;
				return;
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				GD::out.printWarning("Warning: " + ex.what());
				if(!server->keepAlive) server->socket->close();
				_sendCounter--;
				return;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				GD::out.printError(ex.what());
				if(!server->keepAlive) server->socket->close();
				_sendCounter--;
				return;
			}
			//We are using string functions to process the buffer. So just to make sure,
			//they don't do something in the memory after buffer, we add '\0'
			buffer[receivedBytes] = '\0';

			if(server->binary)
			{
				if(dataSize == 0)
				{
					if(!(buffer[3] & 1) && buffer[3] != 0xFF)
					{
						responseData.insert(responseData.end(), buffer, buffer + receivedBytes);
						GD::out.printError("Error: RPC client received binary request as response from server " + server->hostname + " on port " + server->address.second + ". Packet was: " + GD::bl->hf.getHexString(responseData));
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
					if(receivedBytes < 8)
					{
						GD::out.printError("Error: RPC client received binary packet smaller than 8 bytes from server " + server->hostname + " on port " + server->address.second);
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
					GD::bl->hf.memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
					GD::out.printDebug("RPC client receiving binary rpc packet with size: " + std::to_string(receivedBytes) + ". Payload size is: " + std::to_string(dataSize));
					if(dataSize == 0)
					{
						GD::out.printError("Error: RPC client received binary packet without data from server " + server->hostname + " on port " + server->address.second);
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
					if(dataSize > 10485760)
					{
						GD::out.printError("Error: RPC client received packet with data larger than 10 MiB.");
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
					packetLength = receivedBytes - 8;
					responseData.insert(responseData.end(), buffer, buffer + receivedBytes);
				}
				else
				{
					if(packetLength + receivedBytes > dataSize)
					{
						GD::out.printError("Error: RPC client received response packet larger than the expected data size.");
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
					responseData.insert(responseData.end(), buffer, buffer + receivedBytes);
					packetLength += receivedBytes;
				}
				if(packetLength == dataSize)
				{
					responseData.push_back('\0');
					break;
				}
			}
			else
			{
				if(!http.headerIsFinished())
				{
					if(!strncmp(buffer, "401", 3) || !strncmp(&buffer[9], "401", 3)) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
					{
						GD::out.printError("Error: Authentication failed. Server " + server->hostname + ", port " + server->address.second + ". Check user name and password in rpcclients.conf.");
						if(!server->keepAlive) server->socket->close();
						_sendCounter--;
						return;
					}
				}

				try
				{
					http.process(buffer, receivedBytes, true); //Check for chunked packets (HomeMatic Manager, ioBroker). Necessary, because HTTP header does not contain transfer-encoding.
				}
				catch(BaseLib::HTTPException& ex)
				{
					GD::out.printError("XML RPC Client: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
					if(!server->keepAlive) server->socket->close();
					_sendCounter--;
					return;
				}
				if(http.getContentSize() > 10485760 || http.getHeader()->contentLength > 10485760)
				{
					GD::out.printError("Error: Packet with data larger than 100 MiB received.");
					if(!server->keepAlive) server->socket->close();
					_sendCounter--;
					return;
				}
			}
		}
		if(!server->keepAlive) server->socket->close();
		if(GD::bl->debugLevel >= 5)
		{
			if(server->binary) GD::out.printDebug("Debug: Received packet from server " + server->hostname + " on port " + server->address.second + ": " + GD::bl->hf.getHexString(responseData));
			else GD::out.printDebug("Debug: Received packet from server " + server->hostname + " on port " + server->address.second + ":\n" + std::string(&http.getContent()->at(0), http.getContent()->size()));
		}
		_sendCounter--;
		if(http.isFinished()) responseData = *http.getContent();
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
    GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    _sendCounter--;
}
}
