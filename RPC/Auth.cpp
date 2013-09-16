/* Copyright 2013 Sathya Laufer
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
#include "../HelperFunctions.h"

namespace RPC
{
Auth::Auth(SocketOperations& socket, std::vector<std::string>& validUsers)
{
	_socket = socket;
	_validUsers = validUsers;
	_initialized = true;
}

bool Auth::basicServer(std::shared_ptr<std::vector<char>>& binaryPacket)
{
	if(!_initialized) throw AuthException("Not initialized.");
	throw AuthException("Not implemented yet.");
}

bool Auth::basicServer(HTTP& httpPacket)
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
			_socket.proofwrite(data);
			int32_t bytesRead = _socket.proofread(buffer, bufferLength);
			//Some clients send only one byte in the first packet
			if(bytesRead == 1) bytesRead += _socket.proofread(&buffer[1], bufferLength - 1);
			buffer[bytesRead] = '\0';
			try
			{
				_http.process(buffer, bufferLength);
			}
			catch(HTTPException& ex)
			{
				throw AuthException("Authorization failed because of HTTP exception: " + ex.what());
			}
		}
		catch(SocketOperationException& ex)
		{
			throw AuthException("Authorization failed because of socket exception: " + ex.what());
		}
	}
	else _http = httpPacket;
	if(_http.getHeader()->authorization.empty()) throw AuthException("No header field \"Authorization\"");
	std::pair<std::string, std::string> authData = HelperFunctions::split(_http.getHeader()->authorization, ' ');
	HelperFunctions::toLower(authData.first);
	if(authData.first != "basic") throw AuthException("Authorization type is not basic but: " + authData.first);
	std::pair<std::string, std::string> credentials = HelperFunctions::split(Base64::base64Decode(authData.second), ':');
	HelperFunctions::toLower(credentials.first);
	if(std::find(_validUsers.begin(), _validUsers.end(), credentials.first) == _validUsers.end()) throw AuthException("User name " + credentials.first + " is not in the list of valid users.");
	if(User::verify(credentials.first, credentials.second)) return true;
	return false;
}


} /* namespace RPC */
