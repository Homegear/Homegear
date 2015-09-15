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

#include "Auth.h"
#include "../GD/GD.h"

namespace RPC
{
Auth::Auth()
{
	_socket =  std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get()));
}

Auth::Auth(std::shared_ptr<BaseLib::SocketOperations>& socket, std::vector<std::string>& validUsers)
{
	_socket = socket;
	_validUsers = validUsers;
	_initialized = true;
	_rpcEncoder = std::shared_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
	_jsonDecoder = std::shared_ptr<BaseLib::RPC::JsonDecoder>(new BaseLib::RPC::JsonDecoder(GD::bl.get()));
}

Auth::Auth(std::shared_ptr<BaseLib::SocketOperations>& socket, std::string userName, std::string password)
{
	_socket = socket;
	_userName = userName;
	_password = password;
	if(!_userName.empty() && !_password.empty()) _initialized = true;
}

std::pair<std::string, std::string> Auth::basicClient()
{
	if(!_initialized) throw AuthException("Not initialized.");
	if(_basicAuthString.second.empty())
	{
		_basicAuthString.first = "Authorization";
		std::string credentials = _userName + ":" + _password;
		std::string encodedData;
		BaseLib::Base64::encode(credentials, encodedData);
		_basicAuthString.second = "Basic " + encodedData;
	}
	return _basicAuthString;
}

void Auth::sendBasicUnauthorized(bool binary)
{
	if(binary)
	{
		if(_basicUnauthBinaryHeader.empty())
		{
			BaseLib::PVariable error = BaseLib::Variable::createError(-32603, "Unauthorized");
			_rpcEncoder->encodeResponse(error, _basicUnauthBinaryHeader);
		}
		try
		{
			_socket->proofwrite(_basicUnauthBinaryHeader);
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			throw AuthException("Authorization failed because of socket exception: " + ex.what());
		}
	}
	else
	{
		if(_basicUnauthHTTPHeader.empty())
		{
			std::string header;
			header.append("HTTP/1.1 401 Unauthorized\r\n");
			header.append("Connection: Close\r\n");
			std::string content("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>Unauthorized</title></head><body>Unauthorized</body></html>");
			header.append("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n");
			header.append(content);
			_basicUnauthHTTPHeader.insert(_basicUnauthHTTPHeader.begin(), header.begin(), header.end());
		}
		try
		{
			_socket->proofwrite(_basicUnauthHTTPHeader);
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			throw AuthException("Authorization failed because of socket exception: " + ex.what());
		}
	}
}

bool Auth::basicServer(std::shared_ptr<BaseLib::RPC::RPCHeader>& binaryHeader)
{
	if(!_initialized) throw AuthException("Not initialized.");
	if(!binaryHeader) throw AuthException("Header is nullptr.");
	if(binaryHeader->authorization.empty())
	{
		sendBasicUnauthorized(true);
		throw AuthException("No header field \"Authorization\"");
	}
	std::pair<std::string, std::string> authData = BaseLib::HelperFunctions::split(binaryHeader->authorization, ' ');
	BaseLib::HelperFunctions::toLower(authData.first);
	if(authData.first != "basic")
	{
		sendBasicUnauthorized(true);
		throw AuthException("Authorization type is not basic but: " + authData.first);
	}
	std::string decodedData;
	BaseLib::Base64::decode(authData.second, decodedData);
	std::pair<std::string, std::string> credentials = BaseLib::HelperFunctions::split(decodedData, ':');
	BaseLib::HelperFunctions::toLower(credentials.first);
	if(std::find(_validUsers.begin(), _validUsers.end(), credentials.first) == _validUsers.end())
	{
		sendBasicUnauthorized(true);
		throw AuthException("User name " + credentials.first + " is not in the list of valid users.");
	}
	if(User::verify(credentials.first, credentials.second)) return true;
	sendBasicUnauthorized(true);
	return false;
}

bool Auth::basicServer(BaseLib::HTTP& httpPacket)
{
	if(!_initialized) throw AuthException("Not initialized.");
	_http.reset();
	uint32_t bufferLength = 1024;
	char buffer[bufferLength + 1];
	if(httpPacket.getHeader()->authorization.empty())
	{
		if(_basicAuthHTTPHeader.empty())
		{
			_basicAuthHTTPHeader.append("HTTP/1.1 401 Authorization Required\r\n");
			_basicAuthHTTPHeader.append("WWW-Authenticate: Basic realm=\"Authentication Required\"\r\n");
			_basicAuthHTTPHeader.append("Connection: Keep-Alive\r\n");
			std::string content("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>Authorization Required</title></head><body>Authorization Required</body></html>");
			_basicAuthHTTPHeader.append("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n");
			_basicAuthHTTPHeader.append(content);
		}
		std::shared_ptr<std::vector<char>> data(new std::vector<char>());
		data->insert(data->begin(), _basicAuthHTTPHeader.begin(), _basicAuthHTTPHeader.end());
		try
		{
			_socket->proofwrite(data);
			int32_t bytesRead = _socket->proofread(buffer, bufferLength);
			//Some clients send only one byte in the first packet
			if(bytesRead == 1) bytesRead += _socket->proofread(&buffer[1], bufferLength - 1);
			buffer[bytesRead] = '\0';
			try
			{
				_http.process(buffer, bufferLength);
			}
			catch(BaseLib::HTTPException& ex)
			{
				throw AuthException("Authorization failed because of HTTP exception: " + ex.what());
			}
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			throw AuthException("Authorization failed because of socket exception: " + ex.what());
		}
	}
	else _http = httpPacket;
	if(_http.getHeader()->authorization.empty())
	{
		sendBasicUnauthorized(false);
		throw AuthException("No header field \"Authorization\"");
	}
	std::pair<std::string, std::string> authData = BaseLib::HelperFunctions::split(_http.getHeader()->authorization, ' ');
	BaseLib::HelperFunctions::toLower(authData.first);
	if(authData.first != "basic")
	{
		sendBasicUnauthorized(false);
		throw AuthException("Authorization type is not basic but: " + authData.first);
	}
	std::string decodedData;
	BaseLib::Base64::decode(authData.second, decodedData);
	std::pair<std::string, std::string> credentials = BaseLib::HelperFunctions::split(decodedData, ':');
	BaseLib::HelperFunctions::toLower(credentials.first);
	if(std::find(_validUsers.begin(), _validUsers.end(), credentials.first) == _validUsers.end())
	{
		sendBasicUnauthorized(false);
		throw AuthException("User name " + credentials.first + " is not in the list of valid users.");
	}
	if(User::verify(credentials.first, credentials.second)) return true;
	sendBasicUnauthorized(false);
	return false;
}

bool Auth::basicServer(BaseLib::WebSocket& webSocket)
{
	if(!_initialized) throw AuthException("Not initialized.");
	if(webSocket.getContent()->empty())
	{
		sendWebSocketUnauthorized(webSocket, "No data received.");
		return false;
	}
	BaseLib::PVariable variable;
	try
	{
		variable = _jsonDecoder->decode(*webSocket.getContent());
	}
	catch(BaseLib::RPC::JsonDecoderException& ex)
	{
		sendWebSocketUnauthorized(webSocket, "Error decoding json (is packet in json format?).");
		return false;
	}
	if(variable->type != BaseLib::VariableType::tStruct)
	{
		sendWebSocketUnauthorized(webSocket, "Received data is no json object.");
		return false;
	}
#ifdef SCRIPTENGINE
	if(variable->structValue->find("user") == variable->structValue->end() || variable->structValue->find("password") == variable->structValue->end())
	{
		if(variable->structValue->find("user") != variable->structValue->end() && GD::scriptEngine.checkSessionId(variable->structValue->at("user")->stringValue))
		{
			sendWebSocketAuthorized(webSocket);
			return true;
		}
		else
		{
			sendWebSocketUnauthorized(webSocket, "Either \"user\" or \"password\" is not specified.");
			return false;
		}
	}
#else
	sendWebSocketUnauthorized(webSocket, "Either \"user\" or \"password\" is not specified.");
	return false;
#endif
	if(User::verify(variable->structValue->at("user")->stringValue, variable->structValue->at("password")->stringValue))
	{
		sendWebSocketAuthorized(webSocket);
		return true;
	}
	sendWebSocketUnauthorized(webSocket, "Wrong user name or password.");
	return false;
}

void Auth::sendWebSocketAuthorized(BaseLib::WebSocket& webSocket)
{
	std::vector <char> output;
	std::string json("{\"auth\":\"success\"}");
	std::vector<char> data(&json[0], &json[0] + json.size());
	BaseLib::WebSocket::encode(data, BaseLib::WebSocket::Header::Opcode::text, output);
	_socket->proofwrite(output);
}

void Auth::sendWebSocketUnauthorized(BaseLib::WebSocket& webSocket, std::string reason)
{
	std::vector <char> output;
	std::string json("{\"auth\":\"failure\",\"reason\":\"" + reason + "\"}");
	std::vector<char> data(&json[0], &json[0] + json.size());
	BaseLib::WebSocket::encode(data, BaseLib::WebSocket::Header::Opcode::text, output);
	_socket->proofwrite(output);
}

} /* namespace RPC */
