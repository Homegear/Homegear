#ifndef CLIENT_H_
#define CLIENT_H_

#include <thread>
#include <vector>

#include "RPCClient.h"

namespace RPC
{

class Client
{
public:
	Client() {}
	virtual ~Client() {}

	void broadcastEvent(std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> values);
	void addServer(std::pair<std::string, std::string> address, std::string id) { _client.addServer(address, id); }
	void removeServer(std::pair<std::string, std::string> address)  { _client.removeServer(address); }
	void reset() { _client.reset(); }
private:
	RPCClient _client;
};

} /* namespace RPC */
#endif /* CLIENT_H_ */
