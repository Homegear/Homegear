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

#ifndef DATABASECONTROLLER_H_
#define DATABASECONTROLLER_H_

#include "../../Modules/Base/Encoding/RPCEncoder.h"
#include "../../Modules/Base/Encoding/RPCDecoder.h"
#include "../../Modules/Base/Database/SQLite3.h"

class DatabaseController
{
public:
	DatabaseController();
	virtual ~DatabaseController();

	//General
	virtual void open(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal, std::string backupPath = "");
	virtual bool isOpen() { return db.isOpen(); }
	virtual void initializeDatabase();
	virtual void convertDatabase();
	virtual void createSavepoint(std::string name);
	virtual void releaseSavepoint(std::string name);
	//End general

	//Metadata
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setMetadata(std::string objectID, std::string dataID, std::shared_ptr<BaseLib::RPC::RPCVariable> metadata);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getMetadata(std::string objectID, std::string dataID);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getAllMetadata(std::string objectID);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> deleteMetadata(std::string objectID, std::string dataID = "");
	//End metadata

	//Users
	virtual BaseLib::Database::DataTable getUsers();
	virtual bool userNameExists(std::string name);
	virtual uint64_t getUserID(std::string name);
	virtual bool createUser(std::string name, std::vector<uint8_t>& passwordHash, std::vector<uint8_t>& salt);
	virtual bool updateUser(uint64_t id, std::vector<uint8_t>& passwordHash, std::vector<uint8_t>& salt);
	virtual bool deleteUser(uint64_t id);
	virtual BaseLib::Database::DataTable getPassword(std::string name);
	//End users

	//Events
	virtual BaseLib::Database::DataTable getEvents();
	virtual uint64_t saveEvent(BaseLib::Database::DataRow event);
	virtual void deleteEvent(std::string name);
	//End events

	//Device
	virtual BaseLib::Database::DataTable getDevices(uint32_t family);
	virtual void deleteDevice(uint64_t id);
	virtual uint64_t saveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
	virtual uint64_t saveDeviceVariable(BaseLib::Database::DataRow data);
	virtual void deletePeers(int32_t deviceID);
	virtual BaseLib::Database::DataTable getPeers(uint64_t deviceID);
	virtual BaseLib::Database::DataTable getDeviceVariables(uint64_t deviceID);
	//End device

	//Peer
	virtual void deletePeer(uint64_t id);
	virtual uint64_t savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
	virtual uint64_t savePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data);
	virtual uint64_t savePeerVariable(uint64_t peerID, BaseLib::Database::DataRow data);
	virtual BaseLib::Database::DataTable getPeerParameters(uint64_t peerID);
	virtual BaseLib::Database::DataTable getPeerVariables(uint64_t peerID);
	virtual void deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data);
	//End Peer
protected:
	BaseLib::Database::SQLite3 db;
	std::mutex _databaseMutex;

	BaseLib::RPC::RPCEncoder _rpcEncoder;
	BaseLib::RPC::RPCDecoder _rpcDecoder;
};

#endif /* DATABASECONTROLLER_H_ */
