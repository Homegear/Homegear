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

#ifndef DEVICEFAMILY_H_
#define DEVICEFAMILY_H_

#include "../Database/DatabaseTypes.h"
#include "Central.h"
#include "LogicalDevice.h"
#include "PhysicalInterfaceSettings.h"
#include "IPhysicalInterface.h"
#include "DeviceFamilies.h"
#include "../Variable.h"

#include <iostream>
#include <string>
#include <memory>

namespace BaseLib
{

class Obj;

namespace Systems
{
class DeviceFamily : public LogicalDevice::IDeviceEventSink, public IEvents
{
public:
	//Event handling
	class IFamilyEventSink : public IEventSinkBase
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
			virtual std::shared_ptr<Database::DataTable> onGetDevices(uint32_t family) = 0;
			virtual void onDeleteDevice(uint64_t id) = 0;
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
			virtual int32_t onIsAddonClient(int32_t clientID) = 0;

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
		virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values) = 0;
	};
	//End event handling

	DeviceFamily(BaseLib::Obj* bl, IFamilyEventSink* eventHandler);
	virtual ~DeviceFamily();

	virtual bool init() = 0;
	virtual void dispose();

	virtual DeviceFamilies getFamily() { return _family; }
	virtual std::shared_ptr<Variable> listBidcosInterfaces() { return std::shared_ptr<Variable>(new Variable(VariableType::tVoid)); }
	virtual std::shared_ptr<Systems::IPhysicalInterface> createPhysicalDevice(std::shared_ptr<Systems::PhysicalInterfaceSettings> settings) { return std::shared_ptr<Systems::IPhysicalInterface>(); }
	virtual void load() = 0;
	virtual void save(bool full);
	virtual void add(std::shared_ptr<LogicalDevice> device);
	virtual void remove(uint64_t id);
	virtual std::shared_ptr<LogicalDevice> get(int32_t address);
	virtual std::shared_ptr<LogicalDevice> get(uint64_t id);
	virtual std::shared_ptr<LogicalDevice> get(std::string serialNumber);
	virtual std::vector<std::shared_ptr<LogicalDevice>> getDevices();
	virtual std::shared_ptr<Central> getCentral() = 0;
	virtual std::string getName() = 0;
	virtual std::shared_ptr<Variable> getPairingMethods() = 0;
	virtual std::string handleCLICommand(std::string& command) = 0;
	virtual bool peerSelected() { if(!_currentDevice) return false; return _currentDevice->peerSelected(); }
	virtual bool deviceSelected() { return (bool)_currentDevice; }
	virtual bool skipFamilyCLI() { return false; }
	virtual bool hasPhysicalInterface() { return true; }

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
	IFamilyEventSink* _eventHandler;
	DeviceFamilies _family = DeviceFamilies::none;
	std::timed_mutex _devicesMutex;
	std::mutex _removeThreadMutex;
	std::thread _removeThread;
	std::vector<std::shared_ptr<LogicalDevice>> _devices;
	std::shared_ptr<BaseLib::Systems::LogicalDevice> _currentDevice;
	bool _disposed = false;

	void removeThread(uint64_t id);

	//Event handling
	//Database
		//General
		virtual void raiseCreateSavepointSynchronous(std::string name);
		virtual void raiseReleaseSavepointSynchronous(std::string name);
		virtual void raiseCreateSavepointAsynchronous(std::string name);
		virtual void raiseReleaseSavepointAsynchronous(std::string name);

		//Metadata
		virtual void raiseDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "");

		//Device
		virtual std::shared_ptr<Database::DataTable> raiseGetDevices();
		virtual void raiseDeleteDevice(uint64_t id);
		virtual uint64_t raiseSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
		virtual void raiseSaveDeviceVariable(Database::DataRow& data);
		virtual void raiseDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeers(uint64_t deviceID);
		virtual std::shared_ptr<Database::DataTable> raiseGetDeviceVariables(uint64_t deviceID);

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
	virtual void raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual int32_t raiseIsAddonClient(int32_t clientID);
	//End event handling

	//Device event handling
	//Database
		//General
		virtual void onCreateSavepointSynchronous(std::string name);
		virtual void onReleaseSavepointSynchronous(std::string name);
		virtual void onCreateSavepointAsynchronous(std::string name);
		virtual void onReleaseSavepointAsynchronous(std::string name);

		//Metadata
		virtual void onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "");

		//Device
		virtual uint64_t onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
		virtual void onSaveDeviceVariable(Database::DataRow& data);
		virtual void onDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeers(uint64_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetDeviceVariables(uint64_t deviceID);

		//Peer
		virtual void onDeletePeer(uint64_t id);
		virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual void onSavePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual void onSavePeerVariable(uint64_t peerID, Database::DataRow& data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerVariables(uint64_t peerID);
		virtual void onDeletePeerParameter(uint64_t peerID, Database::DataRow& data);
		virtual bool onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID);

		//Service messages
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetServiceMessages(uint64_t peerID);
		virtual void onSaveServiceMessage(uint64_t peerID, Database::DataRow& data);
		virtual void onDeleteServiceMessage(uint64_t databaseID);

		//Hooks
		virtual void onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers);
		virtual void onRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers);
	//End database

	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onRPCNewDevices(std::shared_ptr<Variable> deviceDescriptions);
	virtual void onRPCDeleteDevices(std::shared_ptr<Variable> deviceAddresses, std::shared_ptr<Variable> deviceInfo);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values);
	virtual int32_t onIsAddonClient(int32_t clientID);
	//End Device event handling
private:
	DeviceFamily(const DeviceFamily&);
	DeviceFamily& operator=(const DeviceFamily&);
};

}
}
#endif
