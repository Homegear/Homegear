#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <list>

#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "XMLRPCEncoder.h"
#include "XMLRPCDecoder.h"

namespace RPC
{
class RemoteRPCServer
{
public:
	std::pair<std::string, std::string> address;
	std::string id;
};

class RPCClient {
public:
	RPCClient() { _servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>()); }
	virtual ~RPCClient() {}

	void invokeBroadcast(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<RPCVariable> invoke(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);

	void addServer(std::pair<std::string, std::string> address, std::string id);
	void removeServer(std::pair<std::string, std::string> address);
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> getServers() { return _servers; }
	void reset();
protected:
	XMLRPCDecoder _xmlRpcDecoder;
	XMLRPCEncoder _xmlRpcEncoder;
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> _servers;

	std::string sendRequest(std::string server, std::string port, std::string data);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
