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

#include "SocketOperations.h"

namespace RPC
{
SocketOperations::SocketOperations(int32_t fileDescriptor, SSL* ssl)
{
	_fileDescriptor = fileDescriptor;
	_ssl = ssl;
}

int32_t SocketOperations::proofread(char* buffer, int32_t bufferSize)
{
	if(_fileDescriptor < 0) throw SocketOperationException("Filedescriptor is invalid.");
	//Timeout needs to be set every time, so don't put it outside of the while loop
	timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	fd_set readFileDescriptor;
	FD_ZERO(&readFileDescriptor);
	FD_SET(_fileDescriptor, &readFileDescriptor);
	int32_t bytesRead = select(_fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
	if(bytesRead == 0) throw SocketTimeOutException("Reading from socket timed out.");
	if(bytesRead != 1) throw SocketClosedException("Connection to client number " + std::to_string(_fileDescriptor) + " closed.");
	bytesRead = _ssl ? SSL_read(_ssl, buffer, bufferSize) : read(_fileDescriptor, buffer, bufferSize);
	if(bytesRead <= 0)
	{
		if(bytesRead < 0 && _ssl) throw SocketOperationException("Error reading SSL packet: " + HelperFunctions::getSSLError(SSL_get_error(_ssl, bytesRead)));
		else throw SocketClosedException("Connection to client number " + std::to_string(_fileDescriptor) + " closed.");
	}
	return bytesRead;
}

int32_t SocketOperations::proofwrite(std::shared_ptr<std::vector<char>> data)
{
	if(_fileDescriptor < 0) throw SocketOperationException("Filedescriptor is invalid.");
	if(!data || data->empty()) return 0;
	if(data->size() > 104857600) throw SocketDataLimitException("Data size is larger than 100MB.");
	int32_t ret;
	for(uint32_t i = 0; i < 3; ++i)
	{
		ret = _ssl ? SSL_write(_ssl, &data->at(0), data->size()) : send(_fileDescriptor, &data->at(0), data->size(), MSG_NOSIGNAL);
		if(ret <= 0) throw SocketOperationException(strerror(errno));
		if(ret != (signed)data->size() && i == 2) throw SocketSizeMismatchException("Not all data was sent to client.");
		else break;
	}
	return ret;
}

bool SocketOperations::connected()
{
	if(_fileDescriptor < 0) return false;
	char buffer[1];
	if(recv(_fileDescriptor, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0) return false;
	return true;
}
} /* namespace RPC */
