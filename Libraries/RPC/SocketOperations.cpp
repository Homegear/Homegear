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

#include "SocketOperations.h"
#include "../GD/GD.h"

namespace RPC
{
SocketOperations::SocketOperations()
{
	_autoConnect = false;
	_fileDescriptor.reset(new FileDescriptor());
}

SocketOperations::SocketOperations(std::shared_ptr<FileDescriptor> fileDescriptor, SSL* ssl)
{
	_autoConnect = false;
	if(fileDescriptor) _fileDescriptor = fileDescriptor;
	else _fileDescriptor.reset(new FileDescriptor());
	_ssl = ssl;
}

SocketOperations::SocketOperations(std::string hostname, std::string port)
{
	signal(SIGPIPE, SIG_IGN);

	_fileDescriptor.reset(new FileDescriptor());
	_hostname = hostname;
	_port = port;
}

SocketOperations::SocketOperations(std::string hostname, std::string port, bool useSSL, bool verifyCertificate) : SocketOperations(hostname, port)
{
	signal(SIGPIPE, SIG_IGN);

	_useSSL = useSSL;
	_verifyCertificate = verifyCertificate;

	if(_useSSL) initSSL();
}

SocketOperations::~SocketOperations()
{
	if(_sslCTX)
	{
		SSL_CTX_free(_sslCTX);
		_sslCTX = nullptr;
	}
}

void SocketOperations::initSSL()
{
	if(_sslCTX)
	{
		SSL_CTX_free(_sslCTX);
		_sslCTX = nullptr;
	}

	SSLeay_add_ssl_algorithms();
	SSL_load_error_strings();
	_sslCTX = SSL_CTX_new(SSLv23_client_method());
	SSL_CTX_set_options(_sslCTX, SSL_OP_NO_SSLv2);
	if(!_sslCTX)
	{
		_sslCTX = nullptr;
		throw SocketSSLException("Could not initialize SSL support for TCP client." + std::string(ERR_reason_error_string(ERR_get_error())));
	}
	if(!SSL_CTX_load_verify_locations(_sslCTX, NULL, "/etc/ssl/certs"))
	{
		throw SocketSSLException("Could not load root certificates from /etc/ssl/certs." + std::string(ERR_reason_error_string(ERR_get_error())));
	}
}

void SocketOperations::open()
{
	if(!_fileDescriptor || _fileDescriptor->descriptor < 0) getFileDescriptor();
	else if(!connected())
	{
		close();
		getFileDescriptor();
	}
}

void SocketOperations::autoConnect()
{
	if(!_autoConnect) return;
	if(!_fileDescriptor || _fileDescriptor->descriptor < 0) getFileDescriptor();
	else if(!connected())
	{
		close();
		getFileDescriptor();
	}
}

void SocketOperations::close()
{
	if(_ssl) SSL_free(_ssl);
	_ssl = nullptr;
	GD::fileDescriptorManager.close(_fileDescriptor);
}

int32_t SocketOperations::proofread(char* buffer, int32_t bufferSize)
{
	Output::printDebug("Calling proofread...");
	if(!connected()) autoConnect();
	//Timeout needs to be set every time, so don't put it outside of the while loop
	timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	fd_set readFileDescriptor;
	FD_ZERO(&readFileDescriptor);
	FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
	int32_t bytesRead = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
	if(bytesRead == 0) throw SocketTimeOutException("Reading from socket timed out.");
	if(bytesRead != 1) throw SocketClosedException("Connection to client number " + std::to_string(_fileDescriptor->descriptor) + " closed.");
	bytesRead = _ssl ? SSL_read(_ssl, buffer, bufferSize) : read(_fileDescriptor->descriptor, buffer, bufferSize);
	if(bytesRead <= 0)
	{
		if(bytesRead < 0 && _ssl) throw SocketOperationException("Error reading SSL packet: " + HelperFunctions::getSSLError(SSL_get_error(_ssl, bytesRead)));
		else throw SocketClosedException("Connection to client number " + std::to_string(_fileDescriptor->descriptor) + " closed.");
	}
	return bytesRead;
}

int32_t SocketOperations::proofwrite(std::shared_ptr<std::vector<char>> data)
{
	if(!connected()) autoConnect();
	if(!data || data->empty()) return 0;
	return this->proofwrite(*data);
}

int32_t SocketOperations::proofwrite(std::vector<char>& data)
{
	Output::printDebug("Calling proofwrite ...");
	if(!connected()) autoConnect();
	if(data.empty()) return 0;
	if(data.size() > 104857600) throw SocketDataLimitException("Data size is larger than 100MB.");
	Output::printDebug(" ... data size is " + std::to_string(data.size()));

	int32_t bytesSentSoFar = 0;
	while (bytesSentSoFar < (signed)data.size()) {
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		fd_set writeFileDescriptor;
		FD_ZERO(&writeFileDescriptor);
		FD_SET(_fileDescriptor->descriptor, &writeFileDescriptor);
		int32_t readyFds = select(_fileDescriptor->descriptor + 1, NULL, &writeFileDescriptor, NULL, &timeout);
		if(readyFds == 0) throw SocketTimeOutException("Writing to socket timed out.");
		if(readyFds != 1) throw SocketClosedException("Connection to client number " + std::to_string(_fileDescriptor->descriptor) + " closed.");

		int32_t bytesToSend = data.size() - bytesSentSoFar;
		int32_t bytesSentInStep = _ssl ? SSL_write(_ssl, &data.at(bytesSentSoFar), bytesToSend) : send(_fileDescriptor->descriptor, &data.at(bytesSentSoFar), bytesToSend, MSG_NOSIGNAL);
		if(bytesSentInStep <= 0)
		{
			Output::printDebug(" ... exception at " + std::to_string(bytesSentSoFar) + " error is " + strerror(errno));
			close();
			throw SocketOperationException(strerror(errno));
		}
		bytesSentSoFar += bytesSentInStep;
	}
	Output::printDebug(" ... sent " + std::to_string(bytesSentSoFar));
	return bytesSentSoFar;
}


bool SocketOperations::connected()
{
	if(!_fileDescriptor || _fileDescriptor->descriptor < 0) return false;
	char buffer[1];
	if(recv(_fileDescriptor->descriptor, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0) return false;
	return true;
}

void SocketOperations::getFileDescriptor()
{
	Output::printDebug("Calling getFileDescriptor...");
	GD::fileDescriptorManager.shutdown(_fileDescriptor);

	getConnection();
	if(!_fileDescriptor || _fileDescriptor->descriptor < 0) throw SocketOperationException("Could not connect to server.");

	if(_useSSL) getSSL();
}

void SocketOperations::getSSL()
{
	if(!_fileDescriptor || _fileDescriptor->descriptor < 0) throw SocketSSLException("Could not connect to server using SSL. File descriptor is invalid.");
	if(!_sslCTX)
	{
		GD::fileDescriptorManager.shutdown(_fileDescriptor);
		throw SocketSSLException("Could not connect to server using SSL. SSL is not initialized. Look for previous error messages.");
	}
	_ssl = SSL_new(_sslCTX);
	SSL_set_fd(_ssl, _fileDescriptor->descriptor);
	int32_t result = SSL_connect(_ssl);
	if(!_ssl || result < 1)
	{
		SSL_free(_ssl);
		_ssl = nullptr;
		GD::fileDescriptorManager.shutdown(_fileDescriptor);
		throw SocketSSLException("Error during TLS/SSL handshake: " + HelperFunctions::getSSLError(SSL_get_error(_ssl, result)));
	}

	X509* serverCert = SSL_get_peer_certificate(_ssl);
	if(!serverCert)
	{
		SSL_free(_ssl);
		_ssl = nullptr;
		GD::fileDescriptorManager.shutdown(_fileDescriptor);
		throw SocketSSLException("Could not get server certificate.");
	}

	//When no certificate was received, verify returns X509_V_OK!
	if((result = SSL_get_verify_result(_ssl)) != X509_V_OK && (result != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT || _verifyCertificate))
	{
		SSL_free(_ssl);
		_ssl = nullptr;
		GD::fileDescriptorManager.shutdown(_fileDescriptor);
		throw SocketSSLException("Error during TLS/SSL handshake: " + HelperFunctions::getSSLCertVerificationError(result));
	}
}

void SocketOperations::getConnection()
{
	if(_hostname.empty()) throw SocketInvalidParametersException("Hostname is empty");
	if(_port.empty()) throw SocketInvalidParametersException("Port is empty");

	//Retry for two minutes
	for(uint32_t i = 0; i < 6; ++i)
	{
		struct addrinfo *serverInfo = nullptr;
		struct addrinfo hostInfo;
		memset(&hostInfo, 0, sizeof hostInfo);

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;

		if(getaddrinfo(_hostname.c_str(), _port.c_str(), &hostInfo, &serverInfo) != 0)
		{
			freeaddrinfo(serverInfo);
			throw SocketOperationException("Could not get address information: " + std::string(strerror(errno)));
		}

		char ipStringBuffer[INET6_ADDRSTRLEN];
		if (serverInfo->ai_family == AF_INET) {
			struct sockaddr_in *s = (struct sockaddr_in *)serverInfo->ai_addr;
			inet_ntop(AF_INET, &s->sin_addr, ipStringBuffer, sizeof(ipStringBuffer));
		} else { // AF_INET6
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)serverInfo->ai_addr;
			inet_ntop(AF_INET6, &s->sin6_addr, ipStringBuffer, sizeof(ipStringBuffer));
		}
		std::string ipAddress = std::string(&ipStringBuffer[0]);

		_fileDescriptor = GD::fileDescriptorManager.add(socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol));
		if(_fileDescriptor->descriptor == -1)
		{
			freeaddrinfo(serverInfo);
			throw SocketOperationException("Could not create socket for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}
		int32_t optValue = 1;
		if(setsockopt(_fileDescriptor->descriptor, SOL_SOCKET, SO_KEEPALIVE, (void*)&optValue, sizeof(int32_t)) == -1)
		{
			freeaddrinfo(serverInfo);
			GD::fileDescriptorManager.shutdown(_fileDescriptor);
			throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}

		if(!(fcntl(_fileDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor->descriptor, F_SETFL, fcntl(_fileDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0)
			{
				freeaddrinfo(serverInfo);
				GD::fileDescriptorManager.shutdown(_fileDescriptor);
				throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
			}
		}

		int32_t connectResult;
		if((connectResult = connect(_fileDescriptor->descriptor, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1 && errno != EINPROGRESS)
		{
			if(i < 5)
			{
				freeaddrinfo(serverInfo);
				GD::fileDescriptorManager.shutdown(_fileDescriptor);
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				continue;
			}
			else
			{
				freeaddrinfo(serverInfo);
				GD::fileDescriptorManager.shutdown(_fileDescriptor);
				throw SocketTimeOutException("Connecting to server " + ipAddress + " on port " + _port + " timed out: " + strerror(errno));
			}
		}
		freeaddrinfo(serverInfo);

		if(connectResult != 0) //We have to wait for the connection
		{
			timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			pollfd pollstruct
			{
				(int)_fileDescriptor->descriptor,
				(short)(POLLIN | POLLOUT | POLLERR),
				(short)0
			};

			int32_t pollResult = poll(&pollstruct, 1, 5000);
			if(pollResult < 0 || (pollstruct.revents & POLLERR))
			{
				if(i < 5)
				{
					GD::fileDescriptorManager.shutdown(_fileDescriptor);
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
					continue;
				}
				else
				{
					GD::fileDescriptorManager.shutdown(_fileDescriptor);
					throw SocketTimeOutException("Could not connect to server " + ipAddress + " on port " + _port + ". Poll failed with error code: " + std::to_string(pollResult) + ".");
				}
			}
			else if(pollResult > 0)
			{
				socklen_t resultLength = sizeof(connectResult);
				if(getsockopt(_fileDescriptor->descriptor, SOL_SOCKET, SO_ERROR, &connectResult, &resultLength) < 0)
				{
					GD::fileDescriptorManager.shutdown(_fileDescriptor);
					throw SocketOperationException("Could not connect to server " + ipAddress + " on port " + _port + ": " + strerror(errno) + ".");
				}
				break;
			}
			else if(pollResult == 0)
			{
				if(i < 5)
				{
					GD::fileDescriptorManager.shutdown(_fileDescriptor);
					continue;
				}
				else
				{
					GD::fileDescriptorManager.shutdown(_fileDescriptor);
					throw SocketTimeOutException("Connecting to server " + ipAddress + " on port " + _port + " timed out.");
				}
			}
		}
	}
}
} /* namespace RPC */
