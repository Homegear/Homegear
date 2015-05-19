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

#ifndef DATABASECONTROLLER_H_
#define DATABASECONTROLLER_H_

#include "../../Modules/Base/Encoding/RPCEncoder.h"
#include "../../Modules/Base/Encoding/RPCDecoder.h"
#include "../Database/SQLite3.h"

#include <thread>
#include <condition_variable>

class DatabaseController
{
public:
	struct HomegearVariables
	{
		enum Enum { version = 0, upnpusn = 1 };
	};

	DatabaseController();
	virtual ~DatabaseController();
	void dispose();
	void init();

	//General
	virtual void open(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal, std::string backupPath = "");
	virtual bool isOpen() { return _db.isOpen(); }
	virtual void initializeDatabase();
	virtual void convertDatabase();
	virtual void createSavepointSynchronous(std::string& name);
	virtual void releaseSavepointSynchronous(std::string& name);
	virtual void createSavepointAsynchronous(std::string& name);
	virtual void releaseSavepointAsynchronous(std::string& name);
	//End general

	//Homegear variables
	virtual bool getHomegearVariableString(HomegearVariables::Enum id, std::string& value);
	virtual void setHomegearVariableString(HomegearVariables::Enum id, std::string& value);
	//End Homegear variables

	//Metadata
	virtual std::shared_ptr<BaseLib::RPC::Variable> setMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID, std::shared_ptr<BaseLib::RPC::Variable>& metadata);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getMetadata(uint64_t peerID, std::string& dataID);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getAllMetadata(uint64_t peerID);
	virtual std::shared_ptr<BaseLib::RPC::Variable> deleteMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID);
	//End metadata

	//System variables
	virtual std::shared_ptr<BaseLib::RPC::Variable> setSystemVariable(std::string& variableID, std::shared_ptr<BaseLib::RPC::Variable>& value);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getSystemVariable(std::string& variableID);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getAllSystemVariables();
	virtual std::shared_ptr<BaseLib::RPC::Variable> deleteSystemVariable(std::string& variableID);
	//End system variables

	//Users
	virtual std::shared_ptr<BaseLib::Database::DataTable> getUsers();
	virtual bool userNameExists(const std::string& name);
	virtual uint64_t getUserID(const std::string& name);
	virtual bool createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt);
	virtual bool updateUser(uint64_t id, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt);
	virtual bool deleteUser(uint64_t id);
	virtual std::shared_ptr<BaseLib::Database::DataTable> getPassword(const std::string& name);
	//End users

	//Events
	virtual std::shared_ptr<BaseLib::Database::DataTable> getEvents();
	virtual void saveEventAsynchronous(BaseLib::Database::DataRow& event);
	virtual void deleteEvent(std::string& name);
	//End events

	//Device
	virtual std::shared_ptr<BaseLib::Database::DataTable> getDevices(uint32_t family);
	virtual void deleteDevice(uint64_t id);
	virtual uint64_t saveDevice(uint64_t id, int32_t address, std::string& serialNumber, uint32_t type, uint32_t family);
	virtual void saveDeviceVariableAsynchronous(BaseLib::Database::DataRow& data);
	virtual void deletePeers(int32_t deviceID);
	virtual std::shared_ptr<BaseLib::Database::DataTable> getPeers(uint64_t deviceID);
	virtual std::shared_ptr<BaseLib::Database::DataTable> getDeviceVariables(uint64_t deviceID);
	//End device

	//Peer
	virtual void deletePeer(uint64_t id);
	virtual uint64_t savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string& serialNumber);
	virtual void savePeerParameterAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
	virtual void savePeerVariableAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
	virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerParameters(uint64_t peerID);
	virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerVariables(uint64_t peerID);
	virtual void deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data);

	/**
	 * Changes the ID of a peer.
	 *
	 * @param oldPeerID The old ID of the peer.
	 * @param newPeerID The new ID of the peer.
	 * @return Returns "true" on success or "false" when the new ID is already in use.
	 */
	virtual bool setPeerID(uint64_t oldPeerID, uint64_t newPeerID);
	//End Peer

	//Service messages
	virtual std::shared_ptr<BaseLib::Database::DataTable> getServiceMessages(uint64_t peerID);
	virtual void saveServiceMessageAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
	virtual void deleteServiceMessage(uint64_t databaseID);
	//End service messages
protected:
	bool _disposing = false;

	BaseLib::Database::SQLite3 _db;

	std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;

	std::mutex _systemVariableMutex;
	std::map<std::string, std::shared_ptr<BaseLib::RPC::Variable>> _systemVariables;

	std::mutex _metadataMutex;
	std::map<uint64_t, std::map<std::string, std::shared_ptr<BaseLib::RPC::Variable>>> _metadata;

	/* Queueing */
	static const int32_t _queueSize = 100000;
	std::mutex _queueMutex;
	int32_t _queueHead = 0;
	int32_t _queueTail = 0;
	std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _queue[_queueSize];
	std::mutex _queueProcessingThreadMutex;
	std::thread _queueProcessingThread;
	bool _queueEntryAvailable = false;
	std::condition_variable _queueConditionVariable;
	bool _stopQueueProcessingThread = false;

	void bufferedWrite(std::string command, BaseLib::Database::DataRow& data);
	void processQueueEntry();
	/* Queueing End */
};

#endif /* DATABASECONTROLLER_H_ */
