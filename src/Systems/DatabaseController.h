/* Copyright 2013-2017 Sathya Laufer
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

#include <homegear-base/BaseLib.h>
#include "../Database/SQLite3.h"

#include <thread>
#include <condition_variable>
#include <atomic>

class DatabaseController : public BaseLib::Database::IDatabaseController, public BaseLib::IQueue
{
public:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry(std::string command, BaseLib::Database::DataRow& data) { _entry = std::make_shared<std::pair<std::string, BaseLib::Database::DataRow>>(command, data); };
		virtual ~QueueEntry() {};
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>& getEntry() { return _entry; }
	private:
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _entry;
	};

	DatabaseController();
	virtual ~DatabaseController();
	virtual void dispose();
	virtual void init();

	// {{{ General
		virtual void open(std::string databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath = "", std::string backupFilename = "");
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

	// {{{ Data
		virtual BaseLib::PVariable setData(std::string& component, std::string& key, BaseLib::PVariable& value);
		virtual BaseLib::PVariable getData(std::string& component, std::string& key);
		virtual BaseLib::PVariable deleteData(std::string& component, std::string& key);
	// }}}

	// {{{ Stories
        virtual BaseLib::PVariable addRoomToStory(uint64_t storyId, uint64_t roomId);
        virtual BaseLib::PVariable createStory(BaseLib::PVariable translations, BaseLib::PVariable metadata);
        virtual BaseLib::PVariable deleteStory(uint64_t storyId);
        virtual BaseLib::PVariable getRoomsInStory(BaseLib::PRpcClientInfo clientInfo, uint64_t storyId, bool checkAcls);
        virtual BaseLib::PVariable getStoryMetadata(uint64_t storyId);
        virtual BaseLib::PVariable getStories(std::string languageCode);
        virtual BaseLib::PVariable removeRoomFromStories(uint64_t roomId);
        virtual BaseLib::PVariable removeRoomFromStory(uint64_t storyId, uint64_t roomId);
        virtual bool storyExists(uint64_t storyId);
        virtual BaseLib::PVariable setStoryMetadata(uint64_t storyId, BaseLib::PVariable metadata);
        virtual BaseLib::PVariable updateStory(uint64_t storyId, BaseLib::PVariable translations, BaseLib::PVariable metadata);
	// }}}

	// {{{ Rooms
		virtual BaseLib::PVariable createRoom(BaseLib::PVariable translations, BaseLib::PVariable metadata);
		virtual BaseLib::PVariable deleteRoom(uint64_t roomId);
		virtual BaseLib::PVariable getRoomMetadata(uint64_t roomId);
		virtual BaseLib::PVariable getRooms(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls);
		virtual bool roomExists(uint64_t roomId);
		virtual BaseLib::PVariable setRoomMetadata(uint64_t roomId, BaseLib::PVariable metadata);
		virtual BaseLib::PVariable updateRoom(uint64_t roomId, BaseLib::PVariable translations, BaseLib::PVariable metadata);
	// }}}

	// {{{ Categories
		virtual BaseLib::PVariable createCategory(BaseLib::PVariable translations, BaseLib::PVariable metadata);
		virtual BaseLib::PVariable deleteCategory(uint64_t categoryId);
		virtual BaseLib::PVariable getCategories(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls);
		virtual BaseLib::PVariable getCategoryMetadata(uint64_t categoryId);
		virtual bool categoryExists(uint64_t categoryId);
		virtual BaseLib::PVariable setCategoryMetadata(uint64_t categoryId, BaseLib::PVariable metadata);
		virtual BaseLib::PVariable updateCategory(uint64_t categoryId, BaseLib::PVariable translations, BaseLib::PVariable metadata);
	// }}}

	// {{{ Node data
		virtual BaseLib::PVariable setNodeData(std::string& node, std::string& key, BaseLib::PVariable& value);
		virtual BaseLib::PVariable getNodeData(std::string& node, std::string& key, bool requestFromTrustedServer = false);
		virtual std::set<std::string> getAllNodeDataNodes();
		virtual BaseLib::PVariable deleteNodeData(std::string& node, std::string& key);
	// }}}

	// {{{ Metadata
		virtual BaseLib::PVariable setMetadata(uint64_t peerId, std::string& serialNumber, std::string& dataId, BaseLib::PVariable& metadata);
		virtual BaseLib::PVariable getMetadata(uint64_t peerId, std::string& dataId);
		virtual BaseLib::PVariable getAllMetadata(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<BaseLib::Systems::Peer> peer, bool checkAcls);
		virtual BaseLib::PVariable deleteMetadata(uint64_t peerId, std::string& serialNumber, std::string& dataId);
	// }}}

	// {{{ System variables
        virtual BaseLib::PVariable deleteSystemVariable(std::string& variableId);
        virtual BaseLib::PVariable getSystemVariable(std::string& variableId);
		virtual BaseLib::Database::PSystemVariable getSystemVariableInternal(std::string& variableId);
        virtual BaseLib::PVariable getSystemVariableCategories(std::string& variableId);
        virtual std::set<uint64_t> getSystemVariableCategoriesInternal(std::string& variableId);
        virtual BaseLib::PVariable getSystemVariableRoom(std::string& variableId);
		virtual BaseLib::PVariable getSystemVariablesInCategory(BaseLib::PRpcClientInfo clientInfo, uint64_t categoryId, bool checkAcls);
		virtual BaseLib::PVariable getSystemVariablesInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, bool checkAcls);
        virtual uint64_t getSystemVariableRoomInternal(std::string& variableId);
        virtual BaseLib::PVariable getAllSystemVariables(BaseLib::PRpcClientInfo clientInfo, bool returnRoomsAndCategories, bool checkAcls);
        virtual void removeCategoryFromSystemVariables(uint64_t categoryId);
        virtual void removeRoomFromSystemVariables(uint64_t roomId);
        virtual BaseLib::PVariable setSystemVariable(std::string& variableId, BaseLib::PVariable& value);
        virtual BaseLib::PVariable setSystemVariableCategories(std::string& variableId, std::set<uint64_t>& categories);
        virtual BaseLib::PVariable setSystemVariableRoom(std::string& variableId, uint64_t room);
        virtual bool systemVariableHasCategory(std::string& variableId, uint64_t categoryId);
	// }}}

	// {{{ Users
		virtual bool createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups);
		virtual bool deleteUser(uint64_t userId);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPassword(const std::string& name);
		virtual uint64_t getUserId(const std::string& name);
        virtual BaseLib::PVariable getUserMetadata(uint64_t userId);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getUsers();
		virtual std::vector<uint64_t> getUsersGroups(uint64_t userId);
		virtual bool updateUser(uint64_t userId, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups);
        virtual BaseLib::PVariable setUserMetadata(uint64_t userId, BaseLib::PVariable metadata);
		virtual bool userNameExists(const std::string& name);
	// }}}

	// {{{ Groups
		virtual BaseLib::PVariable createGroup(BaseLib::PVariable translations, BaseLib::PVariable acl);
		virtual BaseLib::PVariable deleteGroup(uint64_t groupId);
		virtual BaseLib::PVariable getAcl(uint64_t groupId);
		virtual BaseLib::PVariable getGroup(uint64_t groupId, std::string languageCode);
		virtual BaseLib::PVariable getGroups(std::string languageCode);
		virtual bool groupExists(uint64_t groupId);
		virtual BaseLib::PVariable updateGroup(uint64_t groupId, BaseLib::PVariable translations, BaseLib::PVariable acl);
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
		virtual void savePeerParameterAsynchronous(BaseLib::Database::DataRow& data);
		virtual void savePeerParameterRoomAsynchronous(BaseLib::Database::DataRow& data);
		virtual void savePeerParameterCategoriesAsynchronous(BaseLib::Database::DataRow& data);
		virtual void savePeerVariableAsynchronous(BaseLib::Database::DataRow& data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> getPeerVariables(uint64_t peerID);
		virtual void deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual bool peerExists(uint64_t peerId);

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
	std::atomic_bool _disposing;

	BaseLib::Database::SQLite3 _db;

	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;

	std::mutex _systemVariableMutex;
	std::map<std::string, BaseLib::Database::PSystemVariable> _systemVariables;

	std::mutex _dataMutex;
	std::map<std::string, std::map<std::string, BaseLib::PVariable>> _data;

	std::mutex _nodeDataMutex;
	std::map<std::string, std::map<std::string, BaseLib::PVariable>> _nodeData;

	std::mutex _metadataMutex;
	std::map<uint64_t, std::map<std::string, BaseLib::PVariable>> _metadata;

	virtual void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
};

#endif
