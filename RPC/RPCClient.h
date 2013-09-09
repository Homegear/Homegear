#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <map>

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
#include <signal.h>

#include "RPCEncoder.h"
#include "RPCDecoder.h"
#include "XMLRPCEncoder.h"
#include "XMLRPCDecoder.h"

namespace RPC
{
class RemoteRPCServer
{
public:
	RemoteRPCServer() { knownDevices.reset(new std::map<std::string, int32_t>()); }
	virtual ~RemoteRPCServer() {}

	bool initialized = false;
	bool useSSL = false;
	bool keepAlive = false;
	bool binary = false;
	std::string hostname;
	std::string ipAddress;
	std::pair<std::string, std::string> address;
	std::string id;
	std::shared_ptr<std::map<std::string, int32_t>> knownDevices;
	std::map<std::string, bool> knownMethods;
	int32_t fileDescriptor = -1;
	SSL* ssl = nullptr;
};

class RPCClient {
public:
	RPCClient();
	virtual ~RPCClient();

	void invokeBroadcast(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<RemoteRPCServer> server, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);

	void reset();
protected:
	RPCDecoder _rpcDecoder;
	RPCEncoder _rpcEncoder;
	XMLRPCDecoder _xmlRpcDecoder;
	XMLRPCEncoder _xmlRpcEncoder;
	int32_t _sendCounter = 0;
	SSL_CTX* _sslCTX = nullptr;

	std::shared_ptr<std::vector<char>> sendRequest(std::shared_ptr<RemoteRPCServer> server, std::shared_ptr<std::vector<char>>& data, bool& timedout);
	std::string getIPAddress(std::string address);
	int32_t getConnection(std::string& hostname, const std::string& port, std::string& ipAddress);
	SSL* getSSL(int32_t fileDescriptor);
	bool connected(int32_t fileDescriptor);
	void getFileDescriptor(std::shared_ptr<RemoteRPCServer>& server, bool& timedout);
	void closeConnection(std::shared_ptr<RemoteRPCServer>& server);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
