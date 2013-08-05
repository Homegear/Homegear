#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

class Peer;

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>

#include "RPC/RPCVariable.h"

class ServiceMessages {
public:
	void setPeer(Peer* peer) { _peer = peer; }

	ServiceMessages(Peer* peer) { _peer = peer; }
	ServiceMessages(Peer* peer, std::string serializedObject);
	virtual ~ServiceMessages() { _peer = nullptr; }

	std::string serialize();
	bool set(std::string id, std::shared_ptr<RPC::RPCVariable> value);
	std::shared_ptr<RPC::RPCVariable> get();

	bool getUnreach() { return _unreach; }

	void setConfigPending(bool value);

	void checkUnreach();
    void endUnreach();
private:
    bool _configPending = false;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;
	int32_t _rssiDevice = 0;
	int32_t _rssiPeer = 0;

	Peer* _peer = nullptr;

	void setConfigPendingThread(bool value);
	void checkUnreachThread();
    void endUnreachThread();
};

#endif /* SERVICEMESSAGES_H_ */
