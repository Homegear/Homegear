/* Copyright 2013-2016 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef DATABASECONTROLLER_H_
#define DATABASECONTROLLER_H_

#include "homegear-base/Database/IDatabaseController.h"
#include "homegear-base/Encoding/RpcEncoder.h"
#include "homegear-base/Encoding/RpcDecoder.h"
#include "../Database/SQLite3.h"

#include <thread>
#include <condition_variable>
#include <atomic>

class DatabaseController : public BaseLib::Database::IDatabaseController
{
public:
	DatabaseController();
	virtual ~DatabaseController();
	virtual void dispose();
	virtual void init();

	// {{{ General
		virtual void open(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath = "");
		virtual void hotBackup();
		virtual bool isOpen() { return _db.isOpen(); }
		virtual void initializeDatabase();
		virtual bool convertDatabase();
		virtual void createSavepointSynchronous(std::string& name);
		virtual void releaseSavepointSynchronous(std::string& name);
		virtual void createSavepointAsynchronous(std::string& name);
		virtual void releaseSavepointAsynchronous(std::string& name);
	// }}}

	// {{{ Homegear variables
		virtual bool getHomegearVariableString(HomegearVariables::Enum id, std::string& value);
		virtual void setHomegearVariableString(HomegearVariables::Enum id, std::string& value);
	// }}}

	// {{{ Metadata
		virtual BaseLib::PVariable setMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID, BaseLib::PVariable& metadata);
		virtual BaseLib::PVariable getMetadata(uint64_t peerID, std::string& dataID);
		virtual BaseLib::PVariable getAllMetadata(uint64_t peerID);
		virtual BaseLib::PVariable deleteMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID);
	// }}}

	// {{{ System variables
		virtual BaseLib::PVariable setSystemVariable(std::string& variableID, BaseLib::PVariable& value);
		virtual BaseLib::PVariable getSystemVariable(std::string& variableID);
		virtual BaseLib::PVariable getAllSystemVariables();
		virtual BaseLib::PVariable deleteSystemVariable(std::string& variableID);
	// }}}

	// {{{ Users
		virtual std::shared_ptr<BaseLib::Database::DataTable> getUsers();
		virtual bool userNameExists(const std::string& name);
		virtual uint64_t getUserID(const std::string& name);
		virtual bool createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt);
		virtual bool updateUser(uint64_t id, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt);
		virtual bool deleteUser(uint64_t id);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPassword(const std::string& name);
	// }}}

	// {{{ Events
		virtual std::shared_ptr<BaseLib::Database::DataTable> getEvents();
		virtual void saveEventAsynchronous(BaseLib::Database::DataRow& event);
		virtual void deleteEvent(std::string& name);
	// }}}

	// {{{ Family
		virtual void deleteFamily(int32_t familyId);
		virtual void saveFamilyVariableAsynchronous(int32_t familyId, BaseLib::Database::DataRow& data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getFamilyVariables(int32_t familyId);
		virtual void deleteFamilyVariable(BaseLib::Database::DataRow& data);
	// }}}

	// {{{ Device
		virtual std::shared_ptr<BaseLib::Database::DataTable> getDevices(uint32_t family);
		virtual void deleteDevice(uint64_t id);
		virtual uint64_t saveDevice(uint64_t id, int32_t address, std::string& serialNumber, uint32_t type, uint32_t family);
		virtual void saveDeviceVariableAsynchronous(BaseLib::Database::DataRow& data);
		virtual void deletePeers(int32_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPeers(uint64_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getDeviceVariables(uint64_t deviceID);
	// }}}

	// {{{ Peer
		virtual void deletePeer(uint64_t id);
		virtual uint64_t savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string& serialNumber, uint32_t type);
		virtual void savePeerParameterAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual void savePeerVariableAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerVariables(uint64_t peerID);
		virtual void deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data);

		/**
		 * {@inheritDoc}
		 */
		virtual bool setPeerID(uint64_t oldPeerID, uint64_t newPeerID);
		//End Peer
	// }}}

	// {{{ Service messages
		virtual std::shared_ptr<BaseLib::Database::DataTable> getServiceMessages(uint64_t peerID);
		virtual void saveServiceMessageAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual void deleteServiceMessage(uint64_t databaseID);
	// }}}

	// {{{ License modules
		virtual std::shared_ptr<BaseLib::Database::DataTable> getLicenseVariables(int32_t moduleId);
		virtual void saveLicenseVariable(int32_t moduleId, BaseLib::Database::DataRow& data);
		virtual void deleteLicenseVariable(int32_t moduleId, uint64_t mapKey);
	// }}}
protected:
	bool _disposing = false;

	BaseLib::Database::SQLite3 _db;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

	std::mutex _systemVariableMutex;
	std::map<std::string, BaseLib::PVariable> _systemVariables;

	std::mutex _metadataMutex;
	std::map<uint64_t, std::map<std::string, BaseLib::PVariable>> _metadata;

	// {{{ Queueing
		static const int32_t _queueSize = 100000;
		std::mutex _queueMutex;
		int32_t _queueHead = 0;
		int32_t _queueTail = 0;
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _queue[_queueSize];
		std::mutex _queueProcessingThreadMutex;
		std::thread _queueProcessingThread;
		bool _queueEntryAvailable = false;
		std::condition_variable _queueConditionVariable;
		std::atomic_bool _stopQueueProcessingThread;

		void bufferedWrite(std::string command, BaseLib::Database::DataRow& data);
		void processQueueEntry();
	// }}}
};

#endif
