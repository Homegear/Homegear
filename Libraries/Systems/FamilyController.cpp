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

#include "FamilyController.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

ModuleLoader::ModuleLoader(std::string name, std::string path)
{
	try
	{
		_name = name;
		BaseLib::Output::printInfo("Info: Loading family module " + _name);

		void* moduleHandle = dlopen(path.c_str(), RTLD_NOW);
		if(!moduleHandle)
		{
			BaseLib::Output::printCritical("Critical: Could not open module \"" + path + "\": " + std::string(dlerror()));
			return;
		}

		BaseLib::Systems::SystemFactory* (*getFactory)();
		getFactory = (BaseLib::Systems::SystemFactory* (*)())dlsym(moduleHandle, "getFactory");
		if(!getFactory)
		{
			BaseLib::Output::printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFactory\" not found.");
			exit(4);
		}

		_handle = moduleHandle;
		_factory = std::unique_ptr<BaseLib::Systems::SystemFactory>(getFactory());
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

ModuleLoader::~ModuleLoader()
{
	try
	{
		BaseLib::Output::printInfo("Info: Disposing family module " + _name);
		BaseLib::Output::printDebug("Debug: Deleting factory pointer of module " + _name);
		if(_factory) delete _factory.release();
		BaseLib::Output::printDebug("Debug: Closing dynamic library module " + _name);
		if(_handle) dlclose(_handle);
		_handle = nullptr;
		BaseLib::Output::printDebug("Debug: Dynamic library " + _name + " disposed");
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::unique_ptr<BaseLib::Systems::DeviceFamily> ModuleLoader::createModule(BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler)
{
	if(!_factory) return std::unique_ptr<BaseLib::Systems::DeviceFamily>();
	return std::unique_ptr<BaseLib::Systems::DeviceFamily>(_factory->createDeviceFamily(BaseLib::Obj::ins, eventHandler));
}

FamilyController::FamilyController()
{
}

FamilyController::~FamilyController()
{
}

//Device event handling
void FamilyController::onCreateSavepoint(std::string name)
{
	GD::db.createSavepoint(name);
}

void FamilyController::onReleaseSavepoint(std::string name)
{
	GD::db.releaseSavepoint(name);
}

void FamilyController::onDeleteMetadata(std::string objectID, std::string dataID)
{
	GD::db.deleteMetadata(objectID, dataID);
}

void FamilyController::onDeletePeer(uint64_t id)
{
	GD::db.deletePeer(id);
}

uint64_t FamilyController::onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	return GD::db.savePeer(id, parentID, address, serialNumber);
}

uint64_t FamilyController::onSavePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data)
{
	return GD::db.savePeerParameter(peerID, data);
}

uint64_t FamilyController::onSavePeerVariable(uint64_t peerID, BaseLib::Database::DataRow data)
{
	return GD::db.savePeerVariable(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeerParameters(uint64_t peerID)
{
	return GD::db.getPeerParameters(peerID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeerVariables(uint64_t peerID)
{
	return GD::db.getPeerVariables(peerID);
}

void FamilyController::onDeletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data)
{
	GD::db.deletePeerParameter(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetDevices(uint32_t family)
{
	return GD::db.getDevices(family);
}

void FamilyController::onDeleteDevice(uint64_t deviceID)
{
	GD::db.deleteDevice(deviceID);
}

uint64_t FamilyController::onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family)
{
	return GD::db.saveDevice(id, address, serialNumber, type, family);
}

uint64_t FamilyController::onSaveDeviceVariable(BaseLib::Database::DataRow data)
{
	return GD::db.saveDeviceVariable(data);
}

void FamilyController::onDeletePeers(int32_t deviceID)
{
	GD::db.deletePeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetPeers(uint64_t deviceID)
{
	return GD::db.getPeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> FamilyController::onGetDeviceVariables(uint64_t deviceID)
{
	return GD::db.getDeviceVariables(deviceID);
}

void FamilyController::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		GD::rpcClient.broadcastEvent(id, channel, deviceAddress, valueKeys, values);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		GD::rpcClient.broadcastUpdateDevice(id, channel, address, (RPC::Client::Hint::Enum)hint);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCNewDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		GD::rpcClient.broadcastNewDevices(deviceDescriptions);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onRPCDeleteDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceAddresses, std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo)
{
	try
	{
		GD::rpcClient.broadcastDeleteDevices(deviceAddresses, deviceInfo);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void FamilyController::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		GD::eventHandler.trigger(peerID, channel, variables, values);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
//End Device event handling

bool FamilyController::familyAvailable(BaseLib::Systems::DeviceFamilies family)
{
	return GD::physicalInterfaces.count(family) > 0;
}

void FamilyController::loadModules()
{
	try
	{
		BaseLib::Output::printDebug("Debug: Loading family modules");

		std::vector<std::string> files = BaseLib::HelperFunctions::getFiles(BaseLib::Obj::ins->settings.modulePath());
		if(files.empty())
		{
			BaseLib::Output::printCritical("Critical: No family modules found in \"" + BaseLib::Obj::ins->settings.modulePath() + "\".");
			exit(3);
		}
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			if(extension != ".so" || prefix != "mod_") continue;
			std::string path(BaseLib::Obj::ins->settings.modulePath() + *i);

			moduleLoaders.insert(std::pair<std::string, std::unique_ptr<ModuleLoader>>(*i, std::unique_ptr<ModuleLoader>(new ModuleLoader(*i, path))));

			std::unique_ptr<BaseLib::Systems::DeviceFamily> family = moduleLoaders.at(*i)->createModule(this);

			if(family) GD::deviceFamilies[family->getFamily()].swap(family);
		}
		if(GD::deviceFamilies.empty())
		{
			BaseLib::Output::printCritical("Critical: Could not load any family modules from \"" + BaseLib::Obj::ins->settings.modulePath() + "\".");
			exit(3);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::load()
{
	try
	{
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(familyAvailable(i->first)) i->second->load();
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::dispose()
{
	try
	{
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(!i->second)
			{
				BaseLib::Output::printError("Error: Disposing of device family with index " + std::to_string((int32_t)i->first) + " failed, because the pointer was empty.");
				continue;
			}
			i->second->dispose();
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FamilyController::save(bool full, bool crash)
{
	try
	{
		if(!crash)
		{
			BaseLib::Output::printMessage("(Shutdown) => Waiting for threads");
			//The disposing is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
			//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of the program.
			dispose();
		}
		BaseLib::Output::printMessage("(Shutdown) => Saving devices");
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->save(full);
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string FamilyController::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "unselect" && _currentFamily && !_currentFamily->deviceSelected())
		{
			_currentFamily = nullptr;
			return "Device family unselected.\n";
		}
		else if(command.compare(0, 8, "families") != 0 && _currentFamily)
		{
			if(!_currentFamily) return "No device family selected.\n";
			return _currentFamily->handleCLICommand(command);
		}
		else if(command == "families help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "families list\t\tList all available device families" << std::endl;
			stringStream << "families select\t\tSelect a device family" << std::endl;
			return stringStream.str();
		}
		else if(command == "families list")
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
			for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				if(i->first == BaseLib::Systems::DeviceFamilies::none || !familyAvailable(i->first)) continue;
				std::string name = i->second->getName();
				name.resize(nameWidth, ' ');
				stringStream
						<< std::setw(idWidth) << std::setfill(' ') << (int32_t)i->first << bar
						<< name << std::endl;
			}
			stringStream << "──────┴───────────────────────────────" << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 15, "families select") == 0)
		{
			BaseLib::Systems::DeviceFamilies family = BaseLib::Systems::DeviceFamilies::none;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			bool central = false;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					family = (BaseLib::Systems::DeviceFamilies)BaseLib::HelperFunctions::getNumber(element, false);
					if(family == BaseLib::Systems::DeviceFamilies::none) return "Invalid family id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a device family." << std::endl;
				stringStream << "Usage: families select FAMILYID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FAMILYID:\tThe id of the family to select. Example: 1" << std::endl;
				stringStream << "Supported families:" << std::endl;
				for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
				{
					if(i->first == BaseLib::Systems::DeviceFamilies::none || !familyAvailable(i->first)) continue;
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
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<BaseLib::RPC::RPCVariable> FamilyController::listFamilies()
{
	try
	{
		if(_rpcCache) return _rpcCache;

		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = GD::deviceFamilies.at(i->first)->getCentral();
			if(!central) continue;

			std::shared_ptr<BaseLib::RPC::RPCVariable> familyDescription(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

			familyDescription->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)i->first))));
			familyDescription->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->second->getName()))));
			array->arrayValue->push_back(familyDescription);
		}
		_rpcCache = array;
		return array;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
