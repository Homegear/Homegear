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

#ifndef DEVICEFAMILY_H_
#define DEVICEFAMILY_H_

#include "../Database/DatabaseTypes.h"
#include "Central.h"
#include "LogicalDevice.h"
#include "PhysicalInterfaceSettings.h"
#include "IPhysicalInterface.h"
#include "DeviceFamilies.h"
#include "../RPC/RPCVariable.h"

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
			virtual void onCreateSavepoint(std::string name) {}
			virtual void onReleaseSavepoint(std::string name) {}

			//Metadata
			virtual void onDeleteMetadata(std::string objectID, std::string dataID = "") {}

			//Device
			virtual std::shared_ptr<Database::DataTable> onGetDevices(uint32_t family) { return std::shared_ptr<Database::DataTable>(); }
			virtual void onDeleteDevice(uint64_t id) {};
			virtual uint64_t onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family) { return 0; }
			virtual uint64_t onSaveDeviceVariable(Database::DataRow data) { return 0; }
			virtual void onDeletePeers(int32_t deviceID) {};
			virtual std::shared_ptr<Database::DataTable> onGetPeers(uint64_t deviceID) { return std::shared_ptr<Database::DataTable>(); }
			virtual std::shared_ptr<Database::DataTable> onGetDeviceVariables(uint64_t deviceID) { return std::shared_ptr<Database::DataTable>(); }

			//Peer
			virtual void onDeletePeer(uint64_t id) {};
			virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber) { return 0; }
			virtual uint64_t onSavePeerParameter(uint64_t peerID, Database::DataRow data) { return 0; }
			virtual uint64_t onSavePeerVariable(uint64_t peerID, Database::DataRow data) { return 0; }
			virtual std::shared_ptr<Database::DataTable> onGetPeerParameters(uint64_t peerID) { return std::shared_ptr<Database::DataTable>(); }
			virtual std::shared_ptr<Database::DataTable> onGetPeerVariables(uint64_t peerID) { return std::shared_ptr<Database::DataTable>(); }
			virtual void onDeletePeerParameter(uint64_t peerID, Database::DataRow data) { }
		//End database

		virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values) {}
		virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint) {}
		virtual void onRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions) {}
		virtual void onRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo) {}
		virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values) {}
	};
	//End event handling

	DeviceFamily(BaseLib::Obj* bl, IFamilyEventSink* eventHandler);
	virtual ~DeviceFamily();

	virtual bool init() = 0;
	virtual void dispose();

	virtual DeviceFamilies getFamily() { return _family; }
	virtual std::shared_ptr<RPC::RPCVariable> listBidcosInterfaces() { return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid)); }
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
	virtual std::string handleCLICommand(std::string& command) = 0;
	virtual bool deviceSelected() { return (bool)_currentDevice; }
protected:
	BaseLib::Obj* _bl = nullptr;
	IFamilyEventSink* _eventHandler;
	DeviceFamilies _family = DeviceFamilies::none;
	std::timed_mutex _devicesMutex;
	std::thread _removeThread;
	std::vector<std::shared_ptr<LogicalDevice>> _devices;
	std::shared_ptr<BaseLib::Systems::LogicalDevice> _currentDevice;
	bool _disposed = false;

	void removeThread(uint64_t id);

	//Event handling
	//Database
		//General
		virtual void raiseCreateSavepoint(std::string name);
		virtual void raiseReleaseSavepoint(std::string name);

		//Metadata
		virtual void raiseDeleteMetadata(std::string objectID, std::string dataID = "");

		//Device
		virtual std::shared_ptr<Database::DataTable> raiseGetDevices();
		virtual void raiseDeleteDevice(uint64_t id);
		virtual uint64_t raiseSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
		virtual uint64_t raiseSaveDeviceVariable(Database::DataRow data);
		virtual void raiseDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeers(uint64_t deviceID);
		virtual std::shared_ptr<Database::DataTable> raiseGetDeviceVariables(uint64_t deviceID);

		//Peer
		virtual void raiseDeletePeer(uint64_t id);
		virtual uint64_t raiseSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual uint64_t raiseSavePeerParameter(uint64_t peerID, Database::DataRow data);
		virtual uint64_t raiseSavePeerVariable(uint64_t peerID, Database::DataRow data);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<Database::DataTable> raiseGetPeerVariables(uint64_t peerID);
		virtual void raiseDeletePeerParameter(uint64_t peerID, Database::DataRow data);
	//End database

	virtual void raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void raiseRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions);
	virtual void raiseRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo);
	virtual void raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End event handling

	//Device event handling
	//Database
		//General
		virtual void onCreateSavepoint(std::string name);
		virtual void onReleaseSavepoint(std::string name);

		//Metadata
		virtual void onDeleteMetadata(std::string objectID, std::string dataID = "");

		//Device
		virtual uint64_t onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
		virtual uint64_t onSaveDeviceVariable(Database::DataRow data);
		virtual void onDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeers(uint64_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetDeviceVariables(uint64_t deviceID);

		//Peer
		virtual void onDeletePeer(uint64_t id);
		virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual uint64_t onSavePeerParameter(uint64_t peerID, Database::DataRow data);
		virtual uint64_t onSavePeerVariable(uint64_t peerID, Database::DataRow data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerVariables(uint64_t peerID);
		virtual void onDeletePeerParameter(uint64_t peerID, Database::DataRow data);
	//End database

	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions);
	virtual void onRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End Device event handling
};

}
}
#endif
