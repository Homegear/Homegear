#ifndef SERVER_H_
#define SERVER_H_

#include <memory>

#include "RPCServer.h"

namespace RPC {

class Server {
public:
	Server() {}
	virtual ~Server() {}

	void registerMethods();
	void start();
protected:
	RPCServer _server;
};

} /* namespace RPC */
#endif /* SERVER_H_ */
