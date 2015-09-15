/* Copyright 2013-2015 Sathya Laufer
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

#include "../Variable.h"
#include "../Encoding/BinaryEncoder.h"
#include "../Encoding/BinaryDecoder.h"
#include "../IEvents.h"
#include "../Database/DatabaseTypes.h"

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>

namespace BaseLib
{

class Obj;

namespace Systems
{
class ServiceMessages : public IEvents
{
public:
	//Event handling
	class IServiceEventSink : public IEventSinkBase
	{
	public:
		virtual void onConfigPending(bool configPending) = 0;

		virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values) = 0;
		virtual void onSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data) = 0;
		virtual std::shared_ptr<Database::DataTable> onGetServiceMessages() = 0;
		virtual void onSaveServiceMessage(Database::DataRow& data) = 0;
		virtual void onDeleteServiceMessage(uint64_t databaseID) = 0;
		virtual void onEnqueuePendingQueues() = 0;
	};
	//End event handling

	ServiceMessages(BaseLib::Obj* baseLib, uint64_t peerID, std::string peerSerial, IServiceEventSink* eventHandler);
	virtual ~ServiceMessages();

	virtual void setPeerID(uint64_t peerID) { _peerID = peerID; }
	virtual void setPeerSerial(std::string peerSerial) { _peerSerial = peerSerial; }

	virtual void load();
	virtual void save(uint32_t index, bool value);
	virtual void save(int32_t channel, std::string id, uint8_t value);
	virtual bool set(std::string id, bool value);
	virtual void set(std::string id, uint8_t value, uint32_t channel);
	virtual std::shared_ptr<Variable> get(int32_t clientID, bool returnID);

	virtual void setConfigPending(bool value);
	virtual bool getConfigPending() { return _configPending; }
	virtual void resetConfigPendingSetTime();
	virtual int64_t getConfigPendingSetTime() { return _configPendingSetTime; }

	virtual void setUnreach(bool value, bool requeue);
	virtual bool getUnreach() { return _unreach; }
	virtual void checkUnreach(int32_t cyclicTimeout, uint32_t lastPacketReceived);
    virtual void endUnreach();

    virtual bool getLowbat() { return _lowbat; }
protected:
    BaseLib::Obj* _bl = nullptr;
    std::map<uint32_t, uint32_t> _variableDatabaseIDs;
    uint64_t _peerID = 0;
    std::string _peerSerial;
    bool _disposing = false;
    bool _configPending = false;
    int64_t _configPendingSetTime = 0;
    int32_t _unreachResendCounter = 0;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;

	std::mutex _errorMutex;
	std::map<uint32_t, std::map<std::string, uint8_t>> _errors;

	//Event handling
	virtual void raiseConfigPending(bool configPending);

	virtual void raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual void raiseSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data);
	virtual std::shared_ptr<Database::DataTable> raiseGetServiceMessages();
	virtual void raiseSaveServiceMessage(Database::DataRow& data);
	virtual void raiseDeleteServiceMessage(uint64_t databaseID);
	virtual void raiseEnqueuePendingQueues();
	//End event handling
};

}
}
#endif /* SERVICEMESSAGES_H_ */
