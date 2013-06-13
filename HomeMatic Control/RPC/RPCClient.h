#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

namespace RPC
{

class RPCClient {
public:
	RPCClient() {}
	virtual ~RPCClient() {}

protected:
	std::vector<std::pair<std::string, std::string>> _servers;

	void sendRequest(std::string server, std::string port, std::string data);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
