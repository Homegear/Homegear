/* Copyright 2013-2015 Sathya Laufer
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

#include "FamilyController.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"

ModuleLoader::ModuleLoader(std::string name, std::string path)
{
	try
	{
		_name = name;
		GD::out.printInfo("Info: Loading family module " + _name);

		void* moduleHandle = dlopen(path.c_str(), RTLD_NOW);
		if(!moduleHandle)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\": " + std::string(dlerror()));
			return;
		}

		std::string (*getVersion)();
		getVersion = (std::string (*)())dlsym(moduleHandle, "getVersion");
		if(!getVersion)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getVersion\" not found.");
			dlclose(moduleHandle);
			return;
		}
		std::string version = getVersion();
		if(GD::bl->version() != version)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Module is compiled for Homegear version " + version);
			dlclose(moduleHandle);
			return;
		}


		BaseLib::Systems::SystemFactory* (*getFactory)();
		getFactory = (BaseLib::Systems::SystemFactory* (*)())dlsym(moduleHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFactory\" not found.");
			dlclose(moduleHandle);
			return;
		}

		_handle = moduleHandle;
		_factory = std::unique_ptr<BaseLib::Systems::SystemFactory>(getFactory());
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

ModuleLoader::~ModuleLoader()
{
	try
	{
		GD::out.printInfo("Info: Disposing family module " + _name);
		GD::out.printDebug("Debug: Deleting factory pointer of module " + _name);
		if(_factory) delete _factory.release();
		GD::out.printDebug("Debug: Closing dynamic library module " + _name);
		if(_handle) dlclose(_handle);
		_handle = nullptr;
		GD::out.printDebug("Debug: Dynamic library " + _name + " disposed");
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::unique_ptr<BaseLib::Systems::DeviceFamily> ModuleLoader::createModule(BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler)
{
	if(!_factory) return std::unique_ptr<BaseLib::Systems::DeviceFamily>();
	return std::unique_ptr<BaseLib::Systems::DeviceFamily>(_factory->createDeviceFamily(GD::bl.get(), eventHandler));
}

FamilyController::FamilyController()
{
}

FamilyController::~FamilyController()
{
	dispose();
}

//Device event handling
void FamilyController::onCreateSavepointSynchronous(std::string name)
{
	GD::db->createSavepointSynchronous(name);
}

void FamilyController::onReleaseSavepointSynchronous(std::string name)
{
	GD::db->releaseSavepointSynchronous(name);
}

void FamilyController::onCreateSavepointAsynchronous(std::string name)
{
	GD::db->createSavepointAsynchronous(name);
}

void FamilyController::onReleaseSavepointAsynchronous(std::string name)
{
	GD::db->releaseSavepointAsynchronous(name);
}

void FamilyController::onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	GD::db->deleteMetadata(peerID, serialNumber, dataID);
}

void FamilyController::onDeletePeer(uint64_t id)
{
	GD::db->deletePeer(id);
}

uint64_t FamilyController::onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	return GD::db->savePeer(id, parentID, address, serialNumber);
}

void FamilyController::onSavePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	GD::db->savePeerParameterAsynchronous(peerID, data);
}

void FamilyController::onSavePeerVariable(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	GD::db->savePeerVariableAsynchronous(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeerParameters(uint64_t peerID)
{
	return GD::db->getPeerParameters(peerID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeerVariables(uint64_t peerID)
{
	return GD::db->getPeerVariables(peerID);
}

void FamilyController::onDeletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	GD::db->deletePeerParameter(peerID, data);
}

bool FamilyController::onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	return GD::db->setPeerID(oldPeerID, newPeerID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetServiceMessages(uint64_t peerID)
{
	return GD::db->getServiceMessages(peerID);
}

void FamilyController::onSaveServiceMessage(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	GD::db->saveServiceMessageAsynchronous(peerID, data);
}

void FamilyController::onDeleteServiceMessage(uint64_t databaseID)
{
	GD::db->deleteServiceMessage(databaseID);
}

void FamilyController::onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, BaseLib::PEventHandler>& eventHandlers)
{
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		eventHandlers[i->first] = i->second.addWebserverEventHandler(eventHandler);
	}
}

void FamilyController::onRemoveWebserverEventHandler(std::map<int32_t, BaseLib::PEventHandler>& eventHandlers)
{
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		i->second.removeWebserverEventHandler(eventHandlers[i->first]);
	}
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetDevices(uint32_t family)
{
	return GD::db->getDevices(family);
}

void FamilyController::onDeleteDevice(uint64_t deviceID)
{
	GD::db->deleteDevice(deviceID);
}

uint64_t FamilyController::onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family)
{
	return GD::db->saveDevice(id, address, serialNumber, type, family);
}

void FamilyController::onSaveDeviceVariable(BaseLib::Database::DataRow& data)
{
	GD::db->saveDeviceVariableAsynchronous(data);
}

void FamilyController::onDeletePeers(int32_t deviceID)
{
	GD::db->deletePeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeers(uint64_t deviceID)
{
	return GD::db->getPeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetDeviceVariables(uint64_t deviceID)
{
	return GD::db->getDeviceVariables(deviceID);
}

void FamilyController::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<BaseLib::PVariable>> values)
{
	try
	{
		GD::rpcClient->broadcastEvent(id, channel, deviceAddress, valueKeys, values);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		GD::rpcClient->broadcastUpdateDevice(id, channel, address, (RPC::Client::Hint::Enum)hint);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCNewDevices(BaseLib::PVariable deviceDescriptions)
{
	try
	{
		GD::rpcClient->broadcastNewDevices(deviceDescriptions);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCDeleteDevices(BaseLib::PVariable deviceAddresses, BaseLib::PVariable deviceInfo)
{
	try
	{
		GD::rpcClient->broadcastDeleteDevices(deviceAddresses, deviceInfo);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values)
{
	try
	{
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, channel, variables, values);
#endif
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRunScript(std::string& script, uint64_t peerId, const std::string& args, bool keepAlive, int32_t interval)
{
	try
	{
#ifdef SCRIPTENGINE
		GD::scriptEngine->executeScript(script, peerId, args, keepAlive, interval);
#endif
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

int32_t FamilyController::onIsAddonClient(int32_t clientID)
{
	return RPC::Server::isAddonClientAll(clientID);
}

void FamilyController::onDecryptDeviceDescription(int32_t moduleId, const std::vector<char>& input, std::vector<char>& output)
{
	try
	{
		std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
		if(i == GD::licensingModules.end() || !i->second)
		{
			GD::out.printError("Error: Could not decrypt device description file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
			return;
		}
		i->second->decryptDeviceDescription(input, output);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
//End Device event handling

void FamilyController::homegearStarted()
{
	try
	{
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(i->second) i->second->homegearStarted();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::homegearShuttingDown()
{
	try
	{
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(i->second) i->second->homegearShuttingDown();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool FamilyController::familyAvailable(int32_t family)
{
	return GD::physicalInterfaces->count(family) > 0 || _familiesWithoutPhysicalInterface.find(family) != _familiesWithoutPhysicalInterface.end();
}

void FamilyController::loadModules()
{
	try
	{
		GD::out.printDebug("Debug: Loading family modules");

		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.modulePath());
		if(files.empty())
		{
			GD::out.printCritical("Critical: No family modules found in \"" + GD::bl->settings.modulePath() + "\".");
			return;
		}
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			std::string licensingPostfix;
			if(i->size() > 15) licensingPostfix = i->substr(i->size() - 12, 9);
			if(extension != ".so" || prefix != "mod_" || licensingPostfix == "licensing") continue;
			std::string path(GD::bl->settings.modulePath() + *i);

			moduleLoaders.insert(std::pair<std::string, std::unique_ptr<ModuleLoader>>(*i, std::unique_ptr<ModuleLoader>(new ModuleLoader(*i, path))));

			std::unique_ptr<BaseLib::Systems::DeviceFamily> family = moduleLoaders.at(*i)->createModule(this);

			if(family)
			{
				if(!family->hasPhysicalInterface()) _familiesWithoutPhysicalInterface.insert(family->getFamily());
				std::string name = family->getName();
				BaseLib::HelperFunctions::toLower(name);
				BaseLib::HelperFunctions::stringReplace(name, " ", "");
				GD::deviceFamiliesByName[name] = family->getFamily();
				GD::deviceFamilies[family->getFamily()].swap(family);
			}
		}
		if(GD::deviceFamilies.empty())
		{
			GD::out.printCritical("Critical: Could not load any family modules from \"" + GD::bl->settings.modulePath() + "\".");
			return;
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::init()
{
	try
	{
		std::vector<int32_t> familiesToRemove;
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(!familyAvailable(i->first) || !i->second->init())
			{
				familiesToRemove.push_back(i->first);
				if(familyAvailable(i->first)) GD::out.printError("Error: Could not initialize device family " + i->second->getName() + ".");
				else GD::out.printInfo("Info: Not initializing device family " + i->second->getName() + ", because no physical interface was found.");
				GD::physicalInterfaces->clear(i->first);
			}
		}
		for(std::vector<int32_t>::iterator i = familiesToRemove.begin(); i != familiesToRemove.end(); ++i)
		{
			GD::deviceFamilies.at(*i)->dispose(); //Calls dispose
			GD::deviceFamilies.erase(*i);
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::load()
{
	try
	{
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(familyAvailable(i->first))
			{
				i->second->load();
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::dispose()
{
	try
	{
		if(_disposed) return;
		_disposed = true;
		_rpcCache.reset();
		if(!GD::deviceFamilies.empty())
		{
			for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				if(!i->second) continue;
				i->second->dispose();
			}
		}
		GD::deviceFamilies.clear();
		moduleLoaders.clear();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::save(bool full)
{
	try
	{
		GD::out.printMessage("(Shutdown) => Saving devices");
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(i->second) i->second->save(full);
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool FamilyController::peerExists(uint64_t peerId)
{
	try
	{
		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(central->logicalDevice()->peerExists(peerId)) return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

std::string FamilyController::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if((command == "unselect" || command == "u") && _currentFamily && !_currentFamily->deviceSelected())
		{
			_currentFamily = nullptr;
			return "Device family unselected.\n";
		}
		else if((command.compare(0, 8, "families") || BaseLib::HelperFunctions::isShortCLICommand(command)) && _currentFamily)
		{
			if((command == "unselect" || command == "u") && _currentFamily && _currentFamily->skipFamilyCLI() && !_currentFamily->peerSelected())
			{
				_currentFamily = nullptr;
				return "Device family unselected.\n";
			}
			return _currentFamily->handleCLICommand(command);
		}
		else if(command == "families help" || command == "fh")
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "families list (ls)\tList all available device families" << std::endl;
			stringStream << "families select (fs)\tSelect a device family" << std::endl;
			return stringStream.str();
		}
		else if(command == "families list" || command == "fl" || (command == "ls" && !_currentFamily))
		{
			std::string bar(" │ ");
			const int32_t idWidth = 5;
			const int32_t nameWidth = 30;
			std::string nameHeader = "Name";
			nameHeader.resize(nameWidth, ' ');
			stringStream << std::setfill(' ')
				<< std::setw(idWidth) << "ID" << bar
				<< nameHeader
				<< std::endl;
			stringStream << "──────┼───────────────────────────────" << std::endl;
			for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				if(i->first == -1 || !familyAvailable(i->first)) continue;
				std::string name = i->second->getName();
				name.resize(nameWidth, ' ');
				stringStream
						<< std::setw(idWidth) << std::setfill(' ') << (int32_t)i->first << bar
						<< name << std::endl;
			}
			stringStream << "──────┴───────────────────────────────" << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 15, "families select") == 0 || command.compare(0, 2, "fs") == 0)
		{
			int32_t family = -1;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 's') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					family = BaseLib::Math::getNumber(element, false);
					if(family == -1) return "Invalid family id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command selects a device family." << std::endl;
				stringStream << "Usage: families select FAMILYID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FAMILYID:\tThe id of the family to select. Example: 1" << std::endl;
				stringStream << "Supported families:" << std::endl;
				for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
				{
					if(i->first == -1 || !familyAvailable(i->first)) continue;
					stringStream << "  FAMILYID: 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)i->first << ":\t" << i->second->getName() << std::endl << std::dec;
				}
				return stringStream.str();
			}
			if(GD::deviceFamilies.find(family) == GD::deviceFamilies.end())
			{
				stringStream << "Device family not found." << std::endl;
				return stringStream.str();
			}

			_currentFamily = GD::deviceFamilies.at(family).get();
			if(!_currentFamily) stringStream << "Device family not found." << std::endl;
			else
			{
				stringStream << "Device family \"" << _currentFamily->getName() << "\" selected." << std::endl;
				stringStream << "For information about the family's commands type: \"help\"" << std::endl;
			}

			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

BaseLib::PVariable FamilyController::listFamilies()
{
	try
	{
		if(_rpcCache) return _rpcCache;

		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));
		if(_disposed) return array;

		for(std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = GD::deviceFamilies.at(i->first)->getCentral();
			if(!central) continue;

			BaseLib::PVariable familyDescription(new BaseLib::Variable(BaseLib::VariableType::tStruct));

			familyDescription->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable((int32_t)i->first))));
			familyDescription->structValue->insert(BaseLib::StructElement("NAME", BaseLib::PVariable(new BaseLib::Variable(i->second->getName()))));
			familyDescription->structValue->insert(BaseLib::StructElement("PAIRING_METHODS", i->second->getPairingMethods()));
			array->arrayValue->push_back(familyDescription);
		}
		_rpcCache = array;
		return array;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
