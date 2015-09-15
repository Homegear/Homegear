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

#ifndef LOGICALDEVICE_H_
#define LOGICALDEVICE_H_

#include "../HelperFunctions/HelperFunctions.h"
#include "IPhysicalInterface.h"
#include "DeviceFamilies.h"
#include "Packet.h"
#include "../Variable.h"
#include "../IEvents.h"
#include "Peer.h"

#include <memory>
#include <set>

using namespace BaseLib::DeviceDescription;

namespace BaseLib
{

class Obj;

namespace Systems
{

class Central;

class LogicalDevice : public Peer::IPeerEventSink, public IPhysicalInterface::IPhysicalInterfaceEventSink, public IEvents
{
public:
	//Event handling
	class IDeviceEventSink : public IEventSinkBase
	{
	public:
		//Database
			//General
			virtual void onCreateSavepointSynchronous(std::string name) = 0;
			virtual void onReleaseSavepointSynchronous(std::string name) = 0;
			virtual void onCreateSavepointAsynchronous(std::string name) = 0;
			virtual void onReleaseSavepointAsynchronous(std::string name) = 0;

			//Metadata
			virtual void onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "") = 0;

			//Device
			virtual uint64_t onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family) = 0;
			virtual void onSaveDeviceVariable(Database::DataRow& data) = 0;
			virtual void onDeletePeers(int32_t deviceID) = 0;
			virtual std::shared_ptr<Database::DataTable> onGetPeers(uint64_t deviceID) = 0;
			virtual std::shared_ptr<Database::DataTable> onGetDeviceVariables(uint64_t deviceID) = 0;

			//Peer
			virtual void onDeletePeer(uint64_t id) = 0;
			virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber) = 0;
			virtual void onSavePeerParameter(uint64_t peerID, Database::DataRow& data) = 0;
			virtual void onSavePeerVariable(uint64_t peerID, Database::DataRow& data) = 0;
			virtual std::shared_ptr<Database::DataTable> onGetPeerParameters(uint64_t peerID) = 0;
			virtual std::shared_ptr<Database::DataTable> onGetPeerVariables(uint64_t peerID) = 0;
			virtual void onDeletePeerParameter(uint64_t peerID, Database::DataRow& data) = 0;
			virtual bool onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID) = 0;

			//Service messages
			virtual std::shared_ptr<Database::DataTable> onGetServiceMessages(uint64_t peerID) = 0;
			virtual void onSaveServiceMessage(uint64_t peerID, Database::DataRow& data) = 0;
			virtual void onDeleteServiceMessage(uint64_t databaseID) = 0;

			//Hooks
			virtual void onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers) = 0;
			virtual void onRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers) = 0;
		//End database

		virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values) = 0;
		virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint) = 0;
		virtual void onRPCNewDevices(std::shared_ptr<Variable> deviceDescriptions) = 0;
		virtual void onRPCDeleteDevices(std::shared_ptr<Variable> deviceAddresses, std::shared_ptr<Variable> deviceInfo) = 0;
		virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::Variable>>> values) = 0;
		virtual int32_t onIsAddonClient(int32_t clientID) = 0;
	};
	//End event handling

	LogicalDevice(DeviceFamilies deviceFamily, BaseLib::Obj* baseLib, IDeviceEventSink* eventHandler);
	LogicalDevice(DeviceFamilies deviceFamily, BaseLib::Obj* baseLib, uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
	virtual ~LogicalDevice();
	virtual void dispose(bool wait = true);

	virtual DeviceFamilies deviceFamily();

	virtual int32_t getAddress() { return _address; }
	virtual uint64_t getID() { return _deviceID; }
    virtual std::string getSerialNumber() { return _serialNumber; }
    virtual uint32_t getDeviceType() { return _deviceType; }
	virtual std::string handleCLICommand(std::string command) { return ""; }
	virtual std::shared_ptr<Central> getCentral() = 0;
	void getPeers(std::vector<std::shared_ptr<Peer>>& peers, std::shared_ptr<std::set<uint64_t>> knownDevices = std::shared_ptr<std::set<uint64_t>>());
	std::shared_ptr<Peer> getPeer(int32_t address);
    std::shared_ptr<Peer> getPeer(uint64_t id);
    std::shared_ptr<Peer> getPeer(std::string serialNumber);
	virtual bool peerSelected() { return (bool)_currentPeer; }
	virtual void setPeerID(uint64_t oldPeerID, uint64_t newPeerID);
	virtual void deletePeersFromDatabase();
	virtual void load();
	virtual void loadVariables() = 0;
	virtual void loadPeers() {}
	virtual void save(bool saveDevice);
	virtual void saveVariables() = 0;
	virtual void saveVariable(uint32_t index, int64_t intValue);
	virtual void saveVariable(uint32_t index, std::string& stringValue);
	virtual void saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue);
	virtual void savePeers(bool full) = 0;

	virtual bool peerExists(int32_t address);
	virtual bool peerExists(std::string serialNumber);
	virtual bool peerExists(uint64_t id);

	/*
     * Executed when Homegear is fully started.
     */
    virtual void homegearStarted();

	/*
     * Executed before Homegear starts shutting down.
     */
    virtual void homegearShuttingDown();
protected:
	BaseLib::Obj* _bl = nullptr;
	DeviceFamilies _deviceFamily = DeviceFamilies::none;
	uint64_t _deviceID = 0;
	int32_t _address = 0;
    std::string _serialNumber;
    uint32_t _deviceType = 0;
    std::map<uint32_t, uint32_t> _variableDatabaseIDs;
    bool _initialized = false;
    bool _disposing = false;
	bool _disposed = false;

	std::shared_ptr<Central> _central;
	std::shared_ptr<Peer> _currentPeer;
    std::unordered_map<int32_t, std::shared_ptr<Peer>> _peers;
    std::unordered_map<std::string, std::shared_ptr<Peer>> _peersBySerial;
    std::map<uint64_t, std::shared_ptr<Peer>> _peersByID;
    std::mutex _peersMutex;

	//Event handling
    PEventHandler _physicalInterfaceEventhandler;

	//Database
		//General
		virtual void raiseCreateSavepointSynchronous(std::string name);
		virtual void raiseReleaseSavepointSynchronous(std::string name);
		virtual void raiseCreateSavepointAsynchronous(std::string name);
		virtual void raiseReleaseSavepointAsynchronous(std::string name);

		//Metadata
		virtual void raiseDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "");

		//Device
		virtual uint64_t raiseSaveDevice();
		virtual void raiseSaveDeviceVariable(Database::DataRow& data);
		virtual void raiseDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeers();
		virtual std::shared_ptr<Database::DataTable> raiseGetDeviceVariables();

		//Peer
		virtual void raiseDeletePeer(uint64_t id);
		virtual uint64_t raiseSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual void raiseSavePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual void raiseSavePeerVariable(uint64_t peerID, Database::DataRow& data);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeerVariables(uint64_t peerID);
		virtual void raiseDeletePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual bool raiseSetPeerID(uint64_t oldPeerID, uint64_t newPeerID);

		//Service messages
		virtual std::shared_ptr<Database::DataTable> raiseGetServiceMessages(uint64_t peerID);
		virtual void raiseSaveServiceMessage(uint64_t peerID, Database::DataRow& data);
		virtual void raiseDeleteServiceMessage(uint64_t id);

		//Hooks
		virtual void raiseAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers);
		virtual void raiseRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers);
	//End database

	virtual void raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual void raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void raiseRPCNewDevices(std::shared_ptr<Variable> deviceDescriptions);
	virtual void raiseRPCDeleteDevices(std::shared_ptr<Variable> deviceAddresses, std::shared_ptr<Variable> deviceInfo);
	virtual void raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::Variable>>> values);
	virtual int32_t raiseIsAddonClient(int32_t clientID);
	//End event handling

	//Physical device event handling
	virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<Packet> packet) = 0;
	//End physical device event handling

	//Peer event handling
	//Database
		//General
		virtual void onCreateSavepointSynchronous(std::string name);
		virtual void onReleaseSavepointSynchronous(std::string name);
		virtual void onCreateSavepointAsynchronous(std::string name);
		virtual void onReleaseSavepointAsynchronous(std::string name);

		//Metadata
		virtual void onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "");

		//Peer
		virtual void onDeletePeer(uint64_t id);
		virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual void onSavePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual void onSavePeerVariable(uint64_t peerID, Database::DataRow& data);
		virtual std::shared_ptr<Database::DataTable> onGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<Database::DataTable> onGetPeerVariables(uint64_t peerID);
		virtual void onDeletePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual bool onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID);

		//Service messages
		virtual std::shared_ptr<Database::DataTable> onGetServiceMessages(uint64_t peerID);
		virtual void onSaveServiceMessage(uint64_t peerID, Database::DataRow& data);
		virtual void onDeleteServiceMessage(uint64_t databaseID);

		//Hooks
		virtual void onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers);
		virtual void onRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers);
	//End database

	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::Variable>>> values);
	virtual int32_t onIsAddonClient(int32_t clientID);
	//End Peer event handling
};

}
}
#endif /* LOGICALDEVICE_H_ */
