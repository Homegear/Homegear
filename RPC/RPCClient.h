#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <mutex>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>

#include "XMLRPCEncoder.h"
#include "XMLRPCDecoder.h"

namespace RPC
{
class RPCClient {
public:
	RPCClient();
	virtual ~RPCClient();

	void invokeBroadcast(std::string server, std::string port, bool useSSL, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<RPCVariable> invoke(std::string server, std::string port, bool useSSL, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);

	void reset();
protected:
	XMLRPCDecoder _xmlRpcDecoder;
	XMLRPCEncoder _xmlRpcEncoder;
	int32_t _sendCounter = 0;
	const SSL_METHOD* _sslMethod = nullptr;
	SSL_CTX* _sslCTX = nullptr;

	std::shared_ptr<std::vector<char>> sendRequest(std::string server, std::string port, bool useSSL, std::string data, bool& timedout);
	std::string getIPAddress(std::string address);
	int32_t getConnection(std::string& ipAddress, std::string& port, std::string& ipString);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
