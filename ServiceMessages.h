#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

class Peer;

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>
#include <map>
#include <mutex>

#include "RPC/RPCVariable.h"

class ServiceMessages {
public:
	void setPeer(Peer* peer) { _peer = peer; }

	ServiceMessages(Peer* peer) { _peer = peer; }
	virtual ~ServiceMessages();

	void serialize(std::vector<uint8_t>& encodedData);
	void unserialize(std::shared_ptr<std::vector<char>> serializedData);
	void unserialize_0_0_6(std::string serializedObject);
	bool set(std::string id, bool value);
	void set(std::string id, uint8_t value, uint32_t channel);
	std::shared_ptr<RPC::RPCVariable> get();
	void dispose();

	void setConfigPending(bool value);
	bool getConfigPending() { return _configPending; }

	void setUnreach(bool value);
	bool getUnreach() { return _unreach; }
	void checkUnreach();
    void endUnreach();
private:
    bool _disposing = false;
    bool _configPending = false;
    int32_t _unreachResendCounter = 0;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;

	std::mutex _errorMutex;
	std::map<uint32_t, std::map<std::string, uint8_t>> _errors;

	std::mutex _peerMutex;
	Peer* _peer = nullptr;
};

#endif /* SERVICEMESSAGES_H_ */
