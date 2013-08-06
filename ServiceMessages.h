#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

class Peer;

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>
#include <map>

#include "RPC/RPCVariable.h"

class ServiceMessages {
public:
	void setPeer(Peer* peer) { _peer = peer; }

	ServiceMessages(Peer* peer) { _peer = peer; }
	ServiceMessages(Peer* peer, std::string serializedObject);
	virtual ~ServiceMessages() { _peer = nullptr; }

	std::string serialize();
	bool set(std::string id, bool value);
	void set(std::string id, uint8_t value, uint32_t channel);
	std::shared_ptr<RPC::RPCVariable> get();

	void setConfigPending(bool value);
	bool getConfigPending() { return _configPending; }

	void setUnreach(bool value);
	bool getUnreach() { return _unreach; }
	void checkUnreach();
    void endUnreach();
private:
    bool _configPending = false;
    int32_t _unreachResendCounter = 0;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;

	std::map<uint32_t, uint8_t> _errors;

	Peer* _peer = nullptr;

	void setConfigPendingThread(bool value);
	void setUnreachThread(bool value);
	void checkUnreachThread();
    void endUnreachThread();
};

#endif /* SERVICEMESSAGES_H_ */
