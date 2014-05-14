/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

#include "../RPC/RPCVariable.h"
#include "../Encoding/BinaryEncoder.h"
#include "../Encoding/BinaryDecoder.h"
#include "../IEvents.h"

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>

namespace BaseLib
{
namespace Systems
{
class ServiceMessages : public BaseLib::IEvents
{
public:
	//Event handling
	class IEventSink : public IEventSinkBase
	{
	public:
		virtual void onRPCBroadcast(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values) = 0;
		virtual void onSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data) = 0;
		virtual void onEnqueuePendingQueues() = 0;
	};

	virtual void raiseOnRPCBroadcast(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void raiseOnSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data);
	virtual void raiseOnEnqueuePendingQueues();
	//End event handling

	ServiceMessages(uint64_t peerID, std::string peerSerial);
	virtual ~ServiceMessages();

	virtual void setPeerID(uint64_t peerID) { _peerID = peerID; }
	virtual void setPeerSerial(std::string peerSerial) { _peerSerial = peerSerial; }

	virtual void serialize(std::vector<uint8_t>& encodedData);
	virtual void unserialize(std::shared_ptr<std::vector<char>> serializedData);
	virtual bool set(std::string id, bool value);
	virtual void set(std::string id, uint8_t value, uint32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> get(bool returnID);

	virtual void setConfigPending(bool value);
	virtual bool getConfigPending() { return _configPending; }

	virtual void setUnreach(bool value);
	virtual bool getUnreach() { return _unreach; }
	virtual void checkUnreach(int32_t cyclicTimeout, uint32_t lastPacketReceived);
    virtual void endUnreach();
protected:
    uint64_t _peerID = 0;
    std::string _peerSerial;
    bool _disposing = false;
    bool _configPending = false;
    int32_t _unreachResendCounter = 0;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;

	std::mutex _errorMutex;
	std::map<uint32_t, std::map<std::string, uint8_t>> _errors;
};

}
}
#endif /* SERVICEMESSAGES_H_ */
