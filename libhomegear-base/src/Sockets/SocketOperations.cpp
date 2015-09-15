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

#include "SocketOperations.h"
#include "../BaseLib.h"

namespace BaseLib
{
SocketOperations::SocketOperations(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
	_autoConnect = false;
	_socketDescriptor.reset(new FileDescriptor);
}

SocketOperations::SocketOperations(BaseLib::Obj* baseLib, std::shared_ptr<FileDescriptor> socketDescriptor)
{
	_bl = baseLib;
	_autoConnect = false;
	if(socketDescriptor) _socketDescriptor = socketDescriptor;
	else _socketDescriptor.reset(new FileDescriptor);
}

SocketOperations::SocketOperations(BaseLib::Obj* baseLib, std::string hostname, std::string port)
{
	_bl = baseLib;
	signal(SIGPIPE, SIG_IGN);

	_socketDescriptor.reset(new FileDescriptor);
	_hostname = hostname;
	_port = port;
}

SocketOperations::SocketOperations(BaseLib::Obj* baseLib, std::string hostname, std::string port, bool useSSL, std::string caFile, bool verifyCertificate) : SocketOperations(baseLib, hostname, port)
{
	_useSSL = useSSL;
	_caFile = caFile;
	_verifyCertificate = verifyCertificate;

	if(_useSSL) initSSL();
}

SocketOperations::~SocketOperations()
{
	_bl->fileDescriptorManager.close(_socketDescriptor);
	if(_x509Cred) gnutls_certificate_free_credentials(_x509Cred);
}

void SocketOperations::initSSL()
{
	if(_x509Cred) gnutls_certificate_free_credentials(_x509Cred);

	int32_t result = 0;
	if((result = gnutls_certificate_allocate_credentials(&_x509Cred)) != GNUTLS_E_SUCCESS)
	{
		_x509Cred = nullptr;
		throw SocketSSLException("Could not allocate certificate credentials: " + std::string(gnutls_strerror(result)));
	}
	if(_caFile.length() == 0)
	{
		if(_verifyCertificate) throw SocketSSLException("Certificate verification is enabled, but \"caFile\" is not specified for the host \"" + _hostname + "\" in rpcclients.conf.");
		else _bl->out.printWarning("Warning: \"caFile\" is not specified for the host \"" + _hostname + "\" in rpcclients.conf and certificate verification is disabled. It is highly recommended to enable certificate verification.");
	}
	else if((result = gnutls_certificate_set_x509_trust_file(_x509Cred, _caFile.c_str(), GNUTLS_X509_FMT_PEM)) < 0)
	{
		gnutls_certificate_free_credentials(_x509Cred);
		_x509Cred = nullptr;
		throw SocketSSLException("Could not load trusted certificates from \"" + _caFile + "\": " + std::string(gnutls_strerror(result)));
	}
	if(result == 0 && _verifyCertificate)
	{
		gnutls_certificate_free_credentials(_x509Cred);
		_x509Cred = nullptr;
		throw SocketSSLException("No certificates found in \"" + _caFile + "\".");
	}
}

void SocketOperations::open()
{
	if(!_socketDescriptor || _socketDescriptor->descriptor < 0) getSocketDescriptor();
	else if(!connected())
	{
		close();
		getSocketDescriptor();
	}
}

void SocketOperations::autoConnect()
{
	if(!_autoConnect) return;
	if(!_socketDescriptor || _socketDescriptor->descriptor < 0) getSocketDescriptor();
	else if(!connected())
	{
		close();
		getSocketDescriptor();
	}
}

void SocketOperations::close()
{
	_bl->fileDescriptorManager.close(_socketDescriptor);
}

int32_t SocketOperations::proofread(char* buffer, int32_t bufferSize)
{
	if(!_socketDescriptor) throw SocketOperationException("Socket descriptor is nullptr.");
	if(!connected()) autoConnect();
	//Timeout needs to be set every time, so don't put it outside of the while loop
	timeval timeout;
	int32_t seconds = _readTimeout / 1000000;
	timeout.tv_sec = seconds;
	timeout.tv_usec = _readTimeout - (1000000 * seconds);
	fd_set readFileDescriptor;
	FD_ZERO(&readFileDescriptor);
	_bl->fileDescriptorManager.lock();
	int32_t nfds = _socketDescriptor->descriptor + 1;
	if(nfds <= 0)
	{
		_bl->fileDescriptorManager.unlock();
		throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (1).");
	}
	FD_SET(_socketDescriptor->descriptor, &readFileDescriptor);
	_bl->fileDescriptorManager.unlock();
	int32_t bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
	if(bytesRead == 0) throw SocketTimeOutException("Reading from socket timed out.");
	if(bytesRead != 1) throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (2).");
	if(_socketDescriptor->tlsSession)
	{
		do
		{
			bytesRead = gnutls_record_recv(_socketDescriptor->tlsSession, buffer, bufferSize);
		} while(bytesRead == GNUTLS_E_INTERRUPTED || bytesRead == GNUTLS_E_AGAIN);
	}
	else
	{
		do
		{
			bytesRead = read(_socketDescriptor->descriptor, buffer, bufferSize);
		} while(bytesRead < 0 && errno == EAGAIN);
	}
	if(bytesRead <= 0) throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (3).");
	return bytesRead;
}

int32_t SocketOperations::proofwrite(const std::shared_ptr<std::vector<char>> data)
{
	if(!connected()) autoConnect();
	if(!data || data->empty()) return 0;
	return proofwrite(*data);
}

int32_t SocketOperations::proofwrite(const std::vector<char>& data)
{

	if(!_socketDescriptor) throw SocketOperationException("Socket descriptor is nullptr.");
	if(!connected()) autoConnect();
	if(data.empty()) return 0;
	if(data.size() > 10485760) throw SocketDataLimitException("Data size is larger than 10 MiB.");

	int32_t totalBytesWritten = 0;
	while (totalBytesWritten < (signed)data.size())
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set writeFileDescriptor;
		FD_ZERO(&writeFileDescriptor);
		_bl->fileDescriptorManager.lock();
		int32_t nfds = _socketDescriptor->descriptor + 1;
		if(nfds <= 0)
		{
			_bl->fileDescriptorManager.unlock();
			throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (4).");
		}
		FD_SET(_socketDescriptor->descriptor, &writeFileDescriptor);
		_bl->fileDescriptorManager.unlock();
		int32_t readyFds = select(nfds, NULL, &writeFileDescriptor, NULL, &timeout);
		if(readyFds == 0) throw SocketTimeOutException("Writing to socket timed out.");
		if(readyFds != 1) throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (5).");

		int32_t bytesWritten = _socketDescriptor->tlsSession ? gnutls_record_send(_socketDescriptor->tlsSession, &data.at(totalBytesWritten), data.size() - totalBytesWritten) : send(_socketDescriptor->descriptor, &data.at(totalBytesWritten), data.size() - totalBytesWritten, MSG_NOSIGNAL);
		if(bytesWritten <= 0)
		{
			close();
			if(_socketDescriptor->tlsSession) throw SocketOperationException(gnutls_strerror(bytesWritten));
			else throw SocketOperationException(strerror(errno));
		}
		totalBytesWritten += bytesWritten;
	}
	return totalBytesWritten;
}

int32_t SocketOperations::proofwrite(const char* buffer, int32_t bytesToWrite)
{

	if(!_socketDescriptor) throw SocketOperationException("Socket descriptor is nullptr.");
	if(!connected()) autoConnect();
	if(bytesToWrite <= 0) return 0;
	if(bytesToWrite > 10485760) throw SocketDataLimitException("Data size is larger than 10 MiB.");

	int32_t totalBytesWritten = 0;
	while (totalBytesWritten < bytesToWrite)
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set writeFileDescriptor;
		FD_ZERO(&writeFileDescriptor);
		_bl->fileDescriptorManager.lock();
		int32_t nfds = _socketDescriptor->descriptor + 1;
		if(nfds <= 0)
		{
			_bl->fileDescriptorManager.unlock();
			throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (4).");
		}
		FD_SET(_socketDescriptor->descriptor, &writeFileDescriptor);
		_bl->fileDescriptorManager.unlock();
		int32_t readyFds = select(nfds, NULL, &writeFileDescriptor, NULL, &timeout);
		if(readyFds == 0) throw SocketTimeOutException("Writing to socket timed out.");
		if(readyFds != 1) throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (5).");

		int32_t bytesWritten = _socketDescriptor->tlsSession ? gnutls_record_send(_socketDescriptor->tlsSession, buffer + totalBytesWritten, bytesToWrite - totalBytesWritten) : send(_socketDescriptor->descriptor, buffer + totalBytesWritten, bytesToWrite - totalBytesWritten, MSG_NOSIGNAL);
		if(bytesWritten <= 0)
		{
			close();
			if(_socketDescriptor->tlsSession) throw SocketOperationException(gnutls_strerror(bytesWritten));
			else throw SocketOperationException(strerror(errno));
		}
		totalBytesWritten += bytesWritten;
	}
	return totalBytesWritten;
}

int32_t SocketOperations::proofwrite(const std::string& data)
{

	_bl->out.printDebug("Debug: Calling proofwrite ...", 6);
	if(!_socketDescriptor) throw SocketOperationException("Socket descriptor is nullptr.");
	if(!connected()) autoConnect();
	if(data.empty()) return 0;
	if(data.size() > 10485760) throw SocketDataLimitException("Data size is larger than 10 MiB.");
	_bl->out.printDebug("Debug: ... data size is " + std::to_string(data.size()), 6);

	int32_t bytesSentSoFar = 0;
	while (bytesSentSoFar < (signed)data.size())
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set writeFileDescriptor;
		FD_ZERO(&writeFileDescriptor);
		_bl->fileDescriptorManager.lock();
		int32_t nfds = _socketDescriptor->descriptor + 1;
		if(nfds <= 0)
		{
			_bl->fileDescriptorManager.unlock();
			throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (6).");
		}
		FD_SET(_socketDescriptor->descriptor, &writeFileDescriptor);
		_bl->fileDescriptorManager.unlock();
		int32_t readyFds = select(nfds, NULL, &writeFileDescriptor, NULL, &timeout);
		if(readyFds == 0) throw SocketTimeOutException("Writing to socket timed out.");
		if(readyFds != 1) throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (7).");

		int32_t bytesToSend = data.size() - bytesSentSoFar;
		int32_t bytesSentInStep = _socketDescriptor->tlsSession ? gnutls_record_send(_socketDescriptor->tlsSession, &data.at(bytesSentSoFar), bytesToSend) : send(_socketDescriptor->descriptor, &data.at(bytesSentSoFar), bytesToSend, MSG_NOSIGNAL);
		if(bytesSentInStep <= 0)
		{
			_bl->out.printDebug("Debug: ... exception at " + std::to_string(bytesSentSoFar) + " error is " + strerror(errno));
			close();
			if(_socketDescriptor->tlsSession) throw SocketOperationException(gnutls_strerror(bytesSentInStep));
			else throw SocketOperationException(strerror(errno));
		}
		bytesSentSoFar += bytesSentInStep;
	}
	_bl->out.printDebug("Debug: ... sent " + std::to_string(bytesSentSoFar), 6);
	return bytesSentSoFar;
}

bool SocketOperations::connected()
{
	if(!_socketDescriptor || _socketDescriptor->descriptor < 0) return false;
	char buffer[1];
	if(recv(_socketDescriptor->descriptor, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0) return false;
	return true;
}

void SocketOperations::getSocketDescriptor()
{
	_bl->out.printDebug("Debug: Calling getFileDescriptor...");
	_bl->fileDescriptorManager.shutdown(_socketDescriptor);

	getConnection();
	if(!_socketDescriptor || _socketDescriptor->descriptor < 0) throw SocketOperationException("Could not connect to server.");

	if(_useSSL) getSSL();
}

void SocketOperations::getSSL()
{
	if(!_socketDescriptor || _socketDescriptor->descriptor < 0) throw SocketSSLException("Could not connect to server using SSL. File descriptor is invalid.");
	if(!_x509Cred)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not connect to server using SSL. Certificate credentials are not initialized. Look for previous error messages.");
	}
	int32_t result = 0;
	//Disable TCP Nagle algorithm to improve performance
	const int32_t value = 1;
	if ((result = setsockopt(_socketDescriptor->descriptor, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value))) < 0) {
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not disable Nagle algorithm.");
	}
	if((result = gnutls_init(&_socketDescriptor->tlsSession, GNUTLS_CLIENT)) != GNUTLS_E_SUCCESS)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not initialize TLS session: " + std::string(gnutls_strerror(result)));
	}
	if(!_socketDescriptor->tlsSession) throw SocketSSLException("Could not initialize TLS session.");
	if((result = gnutls_priority_set_direct(_socketDescriptor->tlsSession, "NORMAL", NULL)) != GNUTLS_E_SUCCESS)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not set cipher priorities: " + std::string(gnutls_strerror(result)));
	}
	if((result = gnutls_credentials_set(_socketDescriptor->tlsSession, GNUTLS_CRD_CERTIFICATE, _x509Cred)) != GNUTLS_E_SUCCESS)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not set trusted certificates: " + std::string(gnutls_strerror(result)));
	}
	gnutls_transport_set_ptr(_socketDescriptor->tlsSession, (gnutls_transport_ptr_t)(uintptr_t)_socketDescriptor->descriptor);
	if((result = gnutls_server_name_set(_socketDescriptor->tlsSession, GNUTLS_NAME_DNS, _hostname.c_str(), _hostname.length())) != GNUTLS_E_SUCCESS)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Could not set server's hostname: " + std::string(gnutls_strerror(result)));
	}
	do
	{
		result = gnutls_handshake(_socketDescriptor->tlsSession);
	} while (result < 0 && gnutls_error_is_fatal(result) == 0);
	if(result != GNUTLS_E_SUCCESS)
	{
		_bl->fileDescriptorManager.shutdown(_socketDescriptor);
		throw SocketSSLException("Error during TLS handshake: " + std::string(gnutls_strerror(result)));
	}

	//Now verify the certificate
	uint32_t serverCertChainLength = 0;
	const gnutls_datum_t* const serverCertChain = gnutls_certificate_get_peers(_socketDescriptor->tlsSession, &serverCertChainLength);
	if(!serverCertChain || serverCertChainLength == 0)
	{
		close();
		throw SocketSSLException("Could not get server certificate.");
	}
	uint32_t status = (uint32_t)-1;
	if((result = gnutls_certificate_verify_peers2(_socketDescriptor->tlsSession, &status)) != GNUTLS_E_SUCCESS)
	{
		close();
		throw SocketSSLException("Could not verify server certificate: " + std::string(gnutls_strerror(result)));
	}
	if(status > 0)
	{
		if(_verifyCertificate)
		{
			close();
			throw SocketSSLException("Error verifying server certificate (Code: " + std::to_string(status) + "): " + HelperFunctions::getGNUTLSCertVerificationError(status));
		}
		else if((status & GNUTLS_CERT_REVOKED) || (status & GNUTLS_CERT_INSECURE_ALGORITHM) || (status & GNUTLS_CERT_NOT_ACTIVATED) || (status & GNUTLS_CERT_EXPIRED))
		{
			close();
			throw SocketSSLException("Error verifying server certificate (Code: " + std::to_string(status) + "): " + HelperFunctions::getGNUTLSCertVerificationError(status));
		}
		else _bl->out.printWarning("Warning: Certificate verification failed (Code: " + std::to_string(status) + "): " + HelperFunctions::getGNUTLSCertVerificationError(status));
	}
	else //Verify hostname
	{
		gnutls_x509_crt_t serverCert;
		if((result = gnutls_x509_crt_init(&serverCert)) != GNUTLS_E_SUCCESS)
		{
			close();
			throw SocketSSLException("Could not initialize server certificate structure: " + std::string(gnutls_strerror(result)));
		}
		//The peer certificate is the first certificate in the list
		if((result = gnutls_x509_crt_import(serverCert, serverCertChain, GNUTLS_X509_FMT_DER)) != GNUTLS_E_SUCCESS)
		{
			gnutls_x509_crt_deinit(serverCert);
			close();
			throw SocketSSLException("Could not import server certificate: " + std::string(gnutls_strerror(result)));
		}
		if((result = gnutls_x509_crt_check_hostname(serverCert, _hostname.c_str())) == 0)
		{
			gnutls_x509_crt_deinit(serverCert);
			close();
			throw SocketSSLException("Server's hostname does not match the server certificate.");
		}
		gnutls_x509_crt_deinit(serverCert);
	}
}

bool SocketOperations::waitForSocket()
{
	if(!_socketDescriptor) throw SocketOperationException("Socket descriptor is nullptr.");
	timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	fd_set readFileDescriptor;
	FD_ZERO(&readFileDescriptor);
	_bl->fileDescriptorManager.lock();
	int32_t nfds = _socketDescriptor->descriptor + 1;
	if(nfds <= 0)
	{
		_bl->fileDescriptorManager.unlock();
		throw SocketClosedException("Connection to client number " + std::to_string(_socketDescriptor->id) + " closed (8).");
	}
	FD_SET(_socketDescriptor->descriptor, &readFileDescriptor);
	_bl->fileDescriptorManager.unlock();
	int32_t bytesRead = select(nfds, &readFileDescriptor, NULL, NULL, &timeout);
	if(bytesRead != 1) return false;
	return true;
}

void SocketOperations::getConnection()
{
	_socketDescriptor.reset();
	if(_hostname.empty()) throw SocketInvalidParametersException("Hostname is empty");
	if(_port.empty()) throw SocketInvalidParametersException("Port is empty");

	_bl->out.printInfo("Info: Connecting to host " + _hostname + " on port " + _port + "...");

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

		_socketDescriptor = _bl->fileDescriptorManager.add(socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol));
		if(!_socketDescriptor || _socketDescriptor->descriptor == -1)
		{
			freeaddrinfo(serverInfo);
			throw SocketOperationException("Could not create socket for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}
		int32_t optValue = 1;
		if(setsockopt(_socketDescriptor->descriptor, SOL_SOCKET, SO_KEEPALIVE, (void*)&optValue, sizeof(int32_t)) == -1)
		{
			freeaddrinfo(serverInfo);
			_bl->fileDescriptorManager.shutdown(_socketDescriptor);
			throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}
		optValue = 30;
		//Don't use SOL_TCP, as this constant doesn't exists in BSD
		if(setsockopt(_socketDescriptor->descriptor, getprotobyname("TCP")->p_proto, TCP_KEEPIDLE, (void*)&optValue, sizeof(int32_t)) == -1)
		{
			freeaddrinfo(serverInfo);
			_bl->fileDescriptorManager.shutdown(_socketDescriptor);
			throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}
		optValue = 4;
		if(setsockopt(_socketDescriptor->descriptor, getprotobyname("TCP")->p_proto, TCP_KEEPCNT, (void*)&optValue, sizeof(int32_t)) == -1)
		{
			freeaddrinfo(serverInfo);
			_bl->fileDescriptorManager.shutdown(_socketDescriptor);
			throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}
		optValue = 15;
		if(setsockopt(_socketDescriptor->descriptor, getprotobyname("TCP")->p_proto, TCP_KEEPINTVL, (void*)&optValue, sizeof(int32_t)) == -1)
		{
			freeaddrinfo(serverInfo);
			_bl->fileDescriptorManager.shutdown(_socketDescriptor);
			throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
		}

		if(!(fcntl(_socketDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
		{
			if(fcntl(_socketDescriptor->descriptor, F_SETFL, fcntl(_socketDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0)
			{
				freeaddrinfo(serverInfo);
				_bl->fileDescriptorManager.shutdown(_socketDescriptor);
				throw SocketOperationException("Could not set socket options for server " + ipAddress + " on port " + _port + ": " + strerror(errno));
			}
		}

		int32_t connectResult;
		if((connectResult = connect(_socketDescriptor->descriptor, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1 && errno != EINPROGRESS)
		{
			if(i < 5)
			{
				freeaddrinfo(serverInfo);
				_bl->fileDescriptorManager.shutdown(_socketDescriptor);
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				continue;
			}
			else
			{
				freeaddrinfo(serverInfo);
				_bl->fileDescriptorManager.shutdown(_socketDescriptor);
				throw SocketTimeOutException("Connecting to server " + ipAddress + " on port " + _port + " timed out: " + strerror(errno));
			}
		}
		freeaddrinfo(serverInfo);

		if(connectResult != 0) //We have to wait for the connection
		{
			pollfd pollstruct
			{
				(int)_socketDescriptor->descriptor,
				(short)(POLLIN | POLLOUT | POLLERR),
				(short)0
			};

			int32_t pollResult = poll(&pollstruct, 1, 5000);
			if(pollResult < 0 || (pollstruct.revents & POLLERR))
			{
				if(i < 5)
				{
					_bl->fileDescriptorManager.shutdown(_socketDescriptor);
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
					continue;
				}
				else
				{
					_bl->fileDescriptorManager.shutdown(_socketDescriptor);
					throw SocketTimeOutException("Could not connect to server " + ipAddress + " on port " + _port + ". Poll failed with error code: " + std::to_string(pollResult) + ".");
				}
			}
			else if(pollResult > 0)
			{
				socklen_t resultLength = sizeof(connectResult);
				if(getsockopt(_socketDescriptor->descriptor, SOL_SOCKET, SO_ERROR, &connectResult, &resultLength) < 0)
				{
					_bl->fileDescriptorManager.shutdown(_socketDescriptor);
					throw SocketOperationException("Could not connect to server " + ipAddress + " on port " + _port + ": " + strerror(errno) + ".");
				}
				break;
			}
			else if(pollResult == 0)
			{
				if(i < 5)
				{
					_bl->fileDescriptorManager.shutdown(_socketDescriptor);
					continue;
				}
				else
				{
					_bl->fileDescriptorManager.shutdown(_socketDescriptor);
					throw SocketTimeOutException("Connecting to server " + ipAddress + " on port " + _port + " timed out.");
				}
			}
		}
	}
	_bl->out.printInfo("Info: Connected to host " + _hostname + " on port " + _port + ". Client number is: " + std::to_string(_socketDescriptor->id));
}
}
