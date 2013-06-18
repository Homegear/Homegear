#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

#include <string>
#include <iomanip>
#include <memory>

#include "RPC/RPCVariable.h"

class ServiceMessages {
public:
	bool unreach = false;
	bool stickyUnreach = false;
	bool configPending = false;
	bool lowbat = false;
	int32_t rssiDevice = 0;
	int32_t rssiPeer = 0;
	void setPeerSerialNumber(std::string& serialNumber) { _peerSerialNumber = serialNumber; }

	ServiceMessages(std::string peerSerialNumber) { _peerSerialNumber = peerSerialNumber; }
	ServiceMessages(std::string peerSerialNumber, std::string serializedObject);
	virtual ~ServiceMessages() {}

	std::string serialize();
	bool set(std::string id, std::shared_ptr<RPC::RPCVariable> value);
	std::shared_ptr<RPC::RPCVariable> get();
private:
	std::string _peerSerialNumber;
};

#endif /* SERVICEMESSAGES_H_ */
