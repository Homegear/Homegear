#ifndef RPCCLIENT_H_
#define RPCCLIENT_H_

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <mutex>

#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "XMLRPCEncoder.h"
#include "XMLRPCDecoder.h"

namespace RPC
{
class RPCClient {
public:
	RPCClient() {}
	virtual ~RPCClient() {}

	void invokeBroadcast(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<RPCVariable> invoke(std::string server, std::string port, std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);

	void reset();
protected:
	XMLRPCDecoder _xmlRpcDecoder;
	XMLRPCEncoder _xmlRpcEncoder;
	int32_t _sendCounter = 0;
	std::mutex _sendMutex;

	std::string sendRequest(std::string server, std::string port, std::string data);
};

} /* namespace RPC */
#endif /* RPCCLIENT_H_ */
