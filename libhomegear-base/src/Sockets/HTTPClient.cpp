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

#include "HTTPClient.h"
#include "../BaseLib.h"
#include "../Encoding/HTTP.h"

namespace BaseLib
{

HTTPClient::HTTPClient(BaseLib::Obj* baseLib, std::string hostname, int32_t port, bool keepAlive, bool useSSL, std::string caFile, bool verifyCertificate)
{
	_bl = baseLib;
	_hostname = hostname;
	if(_hostname.empty()) throw HTTPClientException("The provided hostname is empty.");
	if(port > 0 && port < 65536) _port = port;
	_keepAlive = keepAlive;
	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, hostname, std::to_string(port), useSSL, caFile, verifyCertificate));
}

HTTPClient::~HTTPClient()
{
	_socketMutex.lock();
	if(_socket)
	{
		_socket->close();
		_socket.reset();
	}
	_socketMutex.unlock();
}

void HTTPClient::get(const std::string& path, std::string& data)
{
	std::string fixedPath = path;
	if(fixedPath.empty()) fixedPath = "/";
	std::string getRequest = "GET " + fixedPath + " HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nConnection: " + (_keepAlive ? "Keep-Alive" : "Close") + "\r\n\r\n";
	if(_bl->debugLevel >= 5) _bl->out.printDebug("Debug: HTTP request: " + getRequest);
	sendRequest(getRequest, data);
}

void HTTPClient::sendRequest(const std::string& request, std::string& response, bool responseIsHeaderOnly)
{
	response.clear();
	HTTP http;
	sendRequest(request, http, responseIsHeaderOnly);
	if(http.isFinished() && http.getContentSize() > 0)
	{
		std::shared_ptr<std::vector<char>> content = http.getContent();
		response.insert(response.end(), content->begin(), content->begin() + http.getContentSize());
	}
}

void HTTPClient::sendRequest(const std::string& request, HTTP& http, bool responseIsHeaderOnly)
{
	if(request.empty()) throw HTTPClientException("Request is empty.");

	_socketMutex.lock();
	try
	{
		if(!_socket->connected()) _socket->open();
	}
	catch(const BaseLib::SocketOperationException& ex)
	{
		_socketMutex.unlock();
		throw HTTPClientException("Unable to connect to HTTP server \"" + _hostname + "\": " + ex.what());
	}

	try
	{
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Debug: Sending packet to HTTP server \"" + _hostname + "\": " + request);
		_socket->proofwrite(request);
	}
	catch(BaseLib::SocketDataLimitException& ex)
	{
		if(!_keepAlive) _socket->close();
		_socketMutex.unlock();
		throw HTTPClientException("Unable to write to HTTP server \"" + _hostname + "\": " + ex.what());
	}
	catch(const BaseLib::SocketOperationException& ex)
	{
		if(!_keepAlive) _socket->close();
		_socketMutex.unlock();
		throw HTTPClientException("Unable to write to HTTP server \"" + _hostname + "\": " + ex.what());
	}

	ssize_t receivedBytes;

	int32_t bufferPos = 0;
	int32_t bufferMax = 2048;
	char buffer[bufferMax + 1];

	std::this_thread::sleep_for(std::chrono::milliseconds(5)); //Some servers need a little, before the socket can be read.

	for(int32_t i = 0; i < 10000; i++) //Max 10000 reads.
	{
		if(i > 0 && !_socket->connected())
		{
			if(http.getContentSize() == 0)
			{
				_socketMutex.unlock();
				throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": Connection closed.");
			}
			else
			{
				http.setFinished();
				break;
			}
		}

		try
		{
			if(bufferPos > bufferMax - 1) bufferPos = 0;
			receivedBytes = _socket->proofread(buffer + bufferPos, bufferMax - bufferPos);

			//Some clients send only one byte in the first packet
			if(receivedBytes == 1 && bufferPos == 0 && !http.headerIsFinished()) receivedBytes += _socket->proofread(buffer + bufferPos + 1, bufferMax - bufferPos - 1);
		}
		catch(const BaseLib::SocketTimeOutException& ex)
		{
			if(!_keepAlive) _socket->close();
			_socketMutex.unlock();
			throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": " + ex.what());
		}
		catch(const BaseLib::SocketClosedException& ex)
		{
			if(http.getContentSize() == 0)
			{
				_socketMutex.unlock();
				throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": " + ex.what());
			}
			else
			{
				http.setFinished();
				break;
			}
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			if(!_keepAlive) _socket->close();
			_socketMutex.unlock();
			throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": " + ex.what());
		}
		if(bufferPos + receivedBytes > bufferMax)
		{
			bufferPos = 0;
			continue;
		}
		//We are using string functions to process the buffer. So just to make sure,
		//they don't do something in the memory after buffer, we add '\0'
		buffer[bufferPos + receivedBytes] = '\0';

		if(!http.headerIsFinished() && (!strncmp(buffer, "401", 3) || !strncmp(&buffer[9], "401", 3))) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
		{
			_socketMutex.unlock();
			throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": Server requires authentication.");
		}
		if(!http.headerIsFinished() && !strncmp(&buffer[9], "200", 3) && !strstr(buffer, "\r\n\r\n") && !strstr(buffer, "\n\n"))
		{
			bufferPos = receivedBytes;
			continue;
		}
		receivedBytes = bufferPos + receivedBytes;
		bufferPos = 0;

		try
		{
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Debug: Received packet from HTTP server \"" + _hostname + "\": " + std::string(buffer, receivedBytes));
			http.process(buffer, receivedBytes);
			if(http.headerIsFinished() && responseIsHeaderOnly)
			{
				http.setFinished();
				break;
			}
		}
		catch(HTTPException& ex)
		{
			if(!_keepAlive) _socket->close();
			_socketMutex.unlock();
			throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": " + ex.what());
		}
		if(http.getContentSize() > 10485760 || http.getHeader()->contentLength > 10485760)
		{
			if(!_keepAlive) _socket->close();
			_socketMutex.unlock();
			throw HTTPClientException("Unable to read from HTTP server \"" + _hostname + "\": Packet with data larger than 10 MiB received.");
		}
	}
	if(!_keepAlive) _socket->close();
	_socketMutex.unlock();
}
}
