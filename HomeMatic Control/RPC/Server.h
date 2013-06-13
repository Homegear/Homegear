#ifndef SERVER_H_
#define SERVER_H_

#include <memory>

#include "RPCServer.h"

namespace RPC {

class Server {
public:
	Server() { _server.reset(new RPCServer); }
	virtual ~Server() {}

	void registerMethods();
	void start();
protected:
	std::shared_ptr<RPCServer> _server;
};

} /* namespace RPC */
#endif /* SERVER_H_ */
