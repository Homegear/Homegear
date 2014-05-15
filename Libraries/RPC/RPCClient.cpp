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

#include "RPCClient.h"
#include "../../Version.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

namespace RPC
{
RPCClient::RPCClient()
{
	try
	{
		signal(SIGPIPE, SIG_IGN);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

RPCClient::~RPCClient()
{
	try
	{

	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCClient::invokeBroadcast(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty())
		{
			BaseLib::Output::printError("Error: Could not invoke XML RPC method for server " + server->address.first + ". methodName is empty.");
			return;
		}
		server->sendMutex.lock();
		BaseLib::Output::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server->address.first + " and port " + server->address.second + ".");
		if(BaseLib::Obj::ins->debugLevel >= 5)
		{
			BaseLib::Output::printDebug("Parameters:");
			for(std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> requestData;
		if(server->binary) requestData = _rpcEncoder.encodeRequest(methodName, parameters);
		else requestData = _xmlRpcEncoder.encodeRequest(methodName, parameters);
		std::shared_ptr<std::vector<char>> result;
		for(uint32_t i = 0; i < 5; ++i)
		{
			if(i == 0) result = sendRequest(server, requestData, true, timedout);
			else result = sendRequest(server, requestData, false, timedout);
			if(!timedout) break;
		}
		if(timedout)
		{
			server->sendMutex.unlock();
			return;
		}
		if(!result || result->empty())
		{
			BaseLib::Output::printWarning("Warning: Response is empty. XML RPC method: " + methodName + " Server: " + server->address.first + " Port: " + server->address.second);
			server->sendMutex.unlock();
			return;
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> returnValue;
		if(server->binary) returnValue = _rpcDecoder.decodeResponse(result);
		else returnValue = _xmlRpcDecoder.decodeResponse(result);

		if(returnValue->errorStruct) BaseLib::Output::printError("Error in RPC response from " + server->hostname + " on port " + server->address.second + ": faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(BaseLib::Obj::ins->debugLevel >= 5)
			{
				BaseLib::Output::printDebug("Response was:");
				returnValue->print();
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    server->sendMutex.unlock();
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCClient::invoke(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(methodName.empty()) return BaseLib::RPC::RPCVariable::createError(-32601, "Method name is empty");
		server->sendMutex.lock();
		BaseLib::Output::printInfo("Info: Calling XML RPC method " + methodName + " on server " + server->address.first + " and port " + server->address.second + ".");
		if(BaseLib::Obj::ins->debugLevel >= 5)
		{
			BaseLib::Output::printDebug("Parameters:");
			for(std::list<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		bool timedout = false;
		std::shared_ptr<std::vector<char>> requestData;
		if(server->binary) requestData = _rpcEncoder.encodeRequest(methodName, parameters);
		else requestData = _xmlRpcEncoder.encodeRequest(methodName, parameters);
		std::shared_ptr<std::vector<char>> result;
		for(uint32_t i = 0; i < 5; ++i)
		{
			if(i == 0) result = sendRequest(server, requestData, true, timedout);
			else result = sendRequest(server, requestData, false, timedout);
			if(!timedout) break;
		}
		if(timedout)
		{
			server->sendMutex.unlock();
			return BaseLib::RPC::RPCVariable::createError(-32300, "Request timed out.");
		}
		if(!result || result->empty())
		{
			server->sendMutex.unlock();
			return BaseLib::RPC::RPCVariable::createError(-32700, "No response data.");
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> returnValue;
		if(server->binary) returnValue = _rpcDecoder.decodeResponse(result);
		else returnValue = _xmlRpcDecoder.decodeResponse(result);
		if(returnValue->errorStruct) BaseLib::Output::printError("Error in RPC response from " + server->hostname + " on port " + server->address.second + ": faultCode: " + std::to_string(returnValue->structValue->at("faultCode")->integerValue) + " faultString: " + returnValue->structValue->at("faultString")->stringValue);
		else
		{
			if(BaseLib::Obj::ins->debugLevel >= 5)
			{
				BaseLib::Output::printDebug("Response was:");
				returnValue->print();
			}
			server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
		}

		server->sendMutex.unlock();
		return returnValue;
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    server->sendMutex.unlock();
    return BaseLib::RPC::RPCVariable::createError(-32700, "No response data.");
}

std::string RPCClient::getIPAddress(std::string address)
{
	try
	{
		if(address.size() < 9)
		{
			BaseLib::Output::printError("Error: Server's address too short: " + address);
			return "";
		}
		if(address.substr(0, 7) == "http://") address = address.substr(7);
		else if(address.substr(0, 8) == "https://") address = address.substr(8);
		if(address.empty())
		{
			BaseLib::Output::printError("Error: Server's address is empty.");
			return "";
		}
		//Remove "[" and "]" of IPv6 address
		if(address.front() == '[' && address.back() == ']') address = address.substr(1, address.size() - 2);
		if(address.empty())
		{
			BaseLib::Output::printError("Error: Server's address is empty.");
			return "";
		}

		if(BaseLib::Obj::ins->settings.tunnelClients().find(address) != BaseLib::Obj::ins->settings.tunnelClients().end()) return "localhost";
		return address;
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

std::shared_ptr<std::vector<char>> RPCClient::sendRequest(std::shared_ptr<RemoteRPCServer> server, std::shared_ptr<std::vector<char>> data, bool insertHeader, bool& timedout)
{
	try
	{
		if(!server || !data)
		{
			BaseLib::Output::printError("RPC Client: Could not send packet. Pointer to server or data is nullptr.");
			return std::shared_ptr<std::vector<char>>();
		}

		if(server->hostname.empty()) server->hostname = getIPAddress(server->address.first);
		if(server->hostname.empty()) return std::shared_ptr<std::vector<char>>();
		//Get settings pointer every time this method is executed, because
		//the settings might change.
		server->settings = GD::clientSettings.get(server->hostname);
		if(!server->useSSL && server->settings && server->settings->forceSSL)
		{
			BaseLib::Output::printError("RPC Client: Tried to send unencrypted packet to " + server->hostname + " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again.");
			GD::rpcClient.removeServer(server->address);
			return std::shared_ptr<std::vector<char>>();
		}

		_sendCounter++;
		if(_sendCounter > 100)
		{
			BaseLib::Output::printCritical("Could not execute XML RPC method on server " + server->address.first + " and port " + server->address.second + ", because there are more than 100 requests queued. Your server is either not reachable currently or your connection is too slow.");
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		try
		{
			if(!server->socket.connected())
			{
				server->socket.setHostname(server->hostname);
				server->socket.setPort(server->address.second);
				server->socket.setUseSSL(server->useSSL);
				if(server->settings) server->socket.setVerifyCertificate(server->settings->verifyCertificate);
				server->socket.open();
			}
		}
		catch(BaseLib::SocketOperationException& ex)
		{
			BaseLib::Output::printError(ex.what() + " Removing server. Server has to send \"init\" again.");
			GD::rpcClient.removeServer(server->address);
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}


		if(server->settings && server->settings->authType != ClientSettings::Settings::AuthType::none && !server->auth.initialized())
		{
			if(server->settings->userName.empty() || server->settings->password.empty())
			{
				BaseLib::Output::printError("Error: No user name or password specified in config file for XML RPC server " + server->hostname + " on port " + server->address.second + ". Closing connection.");
				server->socket.close();
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			server->auth = Auth(server->socket, server->settings->userName, server->settings->password);
		}

		if(insertHeader)
		{
			if(server->binary)
			{
				std::shared_ptr<BaseLib::RPC::RPCHeader> header(new BaseLib::RPC::RPCHeader());
				if(server->settings && server->settings->authType == ClientSettings::Settings::AuthType::basic)
				{
					BaseLib::Output::printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header->authorization = authField.second;
				}
				_rpcEncoder.insertHeader(data, header);
			}
			else
			{
				std::string header = "POST " + server->path + " HTTP/1.1\r\nUser_Agent: Homegear " + std::string(VERSION) + "\r\nHost: " + server->hostname + ":" + server->address.second + "\r\nContent-Type: text/xml\r\nContent-Length: " + std::to_string(data->size()) + "\r\nConnection: " + (server->keepAlive ? "Keep-Alive" : "close") + "\r\nTE: chunked\r\n";
				if(server->settings && server->settings->authType == ClientSettings::Settings::AuthType::basic)
				{
					BaseLib::Output::printDebug("Using Basic Access Authentication.");
					std::pair<std::string, std::string> authField = server->auth.basicClient();
					header += authField.first + ": " + authField.second + "\r\n";
				}
				header += "\r\n";
				data->insert(data->begin(), header.begin(), header.end());
				data->push_back('\r');
				data->push_back('\n');
			}
		}

		if(BaseLib::Obj::ins->debugLevel >= 5) BaseLib::Output::printDebug("Sending packet: " + std::string(&data->at(0), data->size()));
		try
		{
			server->socket.proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			BaseLib::Output::printWarning("Warning: " + ex.what());
			server->socket.close();
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}
		catch(BaseLib::SocketOperationException& ex)
		{
			BaseLib::Output::printError("Error: Could not send data to XML RPC server " + server->hostname + " on port " + server->address.second + ": " + ex.what() + ". Giving up.");
			server->socket.close();
			_sendCounter--;
			return std::shared_ptr<std::vector<char>>();
		}

		ssize_t receivedBytes;

		int32_t bufferMax = 1024;
		char buffer[bufferMax];
		HTTP http;
		uint32_t packetLength = 0;
		uint32_t dataSize = 0;
		std::shared_ptr<std::vector<char>> packet;
		if(server->binary) packet.reset(new std::vector<char>());

		while(!http.isFinished()) //This is equal to while(true) for binary packets
		{
			try
			{
				receivedBytes = server->socket.proofread(buffer, bufferMax);
				//Some clients send only one byte in the first packet
				if(packetLength == 0 && receivedBytes == 1) receivedBytes += server->socket.proofread(&buffer[1], bufferMax - 1);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				BaseLib::Output::printInfo("Info: Reading from XML RPC server timed out. Server: " + server->hostname + " Port: " + server->address.second);
				timedout = true;
				if(!server->keepAlive) server->socket.close();
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				BaseLib::Output::printWarning("Warning: " + ex.what());
				if(!server->keepAlive) server->socket.close();
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				BaseLib::Output::printError(ex.what());
				if(!server->keepAlive) server->socket.close();
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}
			//We are using string functions to process the buffer. So just to make sure,
			//they don't do something in the memory after buffer, we add '\0'
			buffer[receivedBytes] = '\0';
			if(!strncmp(buffer, "401", 3) || !strncmp(&buffer[9], "401", 3)) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
			{
				BaseLib::Output::printError("Error: Authentication failed. Server " + server->hostname + ", port " + server->address.second + ". Check user name and password in rpcclients.conf.");
				if(!server->keepAlive) server->socket.close();
				_sendCounter--;
				return std::shared_ptr<std::vector<char>>();
			}

			if(server->binary)
			{
				if(dataSize == 0)
				{
					if(!(buffer[3] & 1) && buffer[3] != 0xFF)
					{
						BaseLib::Output::printError("Error: RPC client received binary request as response from server " + server->hostname + " on port " + server->address.second);
						if(!server->keepAlive) server->socket.close();
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					if(receivedBytes < 8)
					{
						BaseLib::Output::printError("Error: RPC client received binary packet smaller than 8 bytes from server " + server->hostname + " on port " + server->address.second);
						if(!server->keepAlive) server->socket.close();
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					BaseLib::HelperFunctions::memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
					BaseLib::Output::printDebug("RPC client receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
					if(dataSize == 0)
					{
						BaseLib::Output::printError("Error: RPC client received binary packet without data from server " + server->hostname + " on port " + server->address.second);
						if(!server->keepAlive) server->socket.close();
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					if(dataSize > 104857600)
					{
						BaseLib::Output::printError("Error: RPC client received packet with data larger than 100 MiB received.");
						if(!server->keepAlive) server->socket.close();
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					packetLength = receivedBytes - 8;
					packet->insert(packet->begin(), buffer, buffer + receivedBytes);
				}
				else
				{
					if(packetLength + receivedBytes > dataSize)
					{
						BaseLib::Output::printError("Error: RPC client received response packet larger than the expected data size.");
						if(!server->keepAlive) server->socket.close();
						_sendCounter--;
						return std::shared_ptr<std::vector<char>>();
					}
					packet->insert(packet->end(), buffer, buffer + receivedBytes);
					packetLength += receivedBytes;
					if(packetLength == dataSize)
					{
						packet->push_back('\0');
						break;
					}
				}
			}
			else
			{
				try
				{
					http.process(buffer, receivedBytes);
				}
				catch(HTTPException& ex)
				{
					BaseLib::Output::printError("XML RPC Client: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, receivedBytes));
					if(!server->keepAlive) server->socket.close();
					_sendCounter--;
					return std::shared_ptr<std::vector<char>>();
				}
				if(http.getContentSize() > 104857600 || http.getHeader()->contentLength > 104857600)
				{
					BaseLib::Output::printError("Error: Packet with data larger than 100 MiB received.");
					if(!server->keepAlive) server->socket.close();
					_sendCounter--;
					return std::shared_ptr<std::vector<char>>();
				}
			}
		}
		if(!server->keepAlive) server->socket.close();
		BaseLib::Output::printDebug("Debug: Received packet from server " + server->hostname + " on port " + server->address.second + ":\n" + std::string(&http.getContent()->at(0), http.getContent()->size()));
		_sendCounter--;
		if(server->binary) return packet;
		else if(http.isFinished()) return http.getContent();
		else return std::shared_ptr<std::vector<char>>();;
    }
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    BaseLib::Obj::ins->fileDescriptorManager.shutdown(server->fileDescriptor);
    _sendCounter--;
    return std::shared_ptr<std::vector<char>>();
}
} /* namespace RPC */
