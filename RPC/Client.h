#ifndef CLIENT_H_
#define CLIENT_H_

#include <map>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include "RPCClient.h"
#include "RPCVariable.h"

namespace RPC
{
class RemoteRPCServer
{
public:
	RemoteRPCServer() { knownDevices.reset(new std::map<std::string, int32_t>()); }
	virtual ~RemoteRPCServer() {}

	bool initialized = false;
	std::pair<std::string, std::string> address;
	std::string id;
	std::shared_ptr<std::map<std::string, int32_t>> knownDevices;
	std::map<std::string, bool> knownMethods;
};

class Client
{
public:
	struct Hint
	{
		enum Enum { updateHintAll = 0, updateHintLinks = 1 };
	};

	Client() { _servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>()); }
	virtual ~Client();

	void initServerMethods(std::pair<std::string, std::string> address);
	void broadcastEvent(std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> values);
	void systemListMethods(std::pair<std::string, std::string> address);
	void listDevices(std::pair<std::string, std::string> address);
	void broadcastNewDevices(std::shared_ptr<RPCVariable> deviceDescriptions);
	void broadcastDeleteDevices(std::shared_ptr<RPCVariable> deviceAddresses);
	void broadcastUpdateDevice(std::string address, Hint::Enum hint);
	void sendUnknownDevices(std::pair<std::string, std::string> address);
	void addServer(std::pair<std::string, std::string> address, std::string id);
	void removeServer(std::pair<std::string, std::string> address);
	std::shared_ptr<RemoteRPCServer> getServer(std::pair<std::string, std::string>);
	void reset();
private:
	RPCClient _client;
	std::mutex _serversMutex;
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> _servers;
};

} /* namespace RPC */
#endif /* CLIENT_H_ */
