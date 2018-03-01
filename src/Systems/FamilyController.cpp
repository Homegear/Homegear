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

#include "FamilyController.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

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

		int32_t (*getFamilyId)();
		getFamilyId = (int32_t (*)())dlsym(moduleHandle, "getFamilyId");
		if(!getFamilyId)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFamilyId\" not found.");
			dlclose(moduleHandle);
			return;
		}
		_familyId = getFamilyId();
		if(_familyId < 0)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Got invalid family id.");
			dlclose(moduleHandle);
			return;
		}

		std::string (*getFamilyName)();
		getFamilyName = (std::string (*)())dlsym(moduleHandle, "getFamilyName");
		if(!getFamilyName)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFamilyName\" not found.");
			dlclose(moduleHandle);
			return;
		}
		_familyName = getFamilyName();
		if(_familyName.empty())
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Got empty family name.");
			dlclose(moduleHandle);
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
		_version = getVersion();
		if(GD::bl->version() != _version)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Module is compiled for Homegear version " + _version);
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
	dispose();
}

void ModuleLoader::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
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

int32_t ModuleLoader::getFamilyId()
{
	return _familyId;
}

std::string ModuleLoader::getFamilyName()
{
	return _familyName;
}

std::string ModuleLoader::getVersion()
{
	return _version;
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

void FamilyController::onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, BaseLib::PEventHandler>& eventHandlers)
{
	for(std::map<int32_t, Rpc::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		eventHandlers[i->first] = i->second.addWebserverEventHandler(eventHandler);
	}
}

void FamilyController::onRemoveWebserverEventHandler(std::map<int32_t, BaseLib::PEventHandler>& eventHandlers)
{
	for(std::map<int32_t, Rpc::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		i->second.removeWebserverEventHandler(eventHandlers[i->first]);
	}
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
		GD::rpcClient->broadcastUpdateDevice(id, channel, address, (Rpc::Client::Hint::Enum)hint);
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
		if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(peerID, channel, variables, values);
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, channel, variables, values);
#endif
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(peerID, channel, variables, values);
#endif
		if(GD::ipcServer) GD::ipcServer->broadcastEvent(peerID, channel, variables, values);
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

void FamilyController::onRunScript(BaseLib::ScriptEngine::PScriptInfo& scriptInfo, bool wait)
{
	try
	{
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->executeScript(scriptInfo, wait);
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

BaseLib::PVariable FamilyController::onInvokeRpc(std::string& methodName, BaseLib::PArray& parameters)
{
	try
	{
		return GD::rpcServers.begin()->second.callMethod(_dummyClientInfo, methodName, std::make_shared<BaseLib::Variable>(parameters));
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
    return BaseLib::Variable::createError(-32500, "Homegear is compiled without script engine.");
}

int32_t FamilyController::onCheckLicense(int32_t moduleId, int32_t familyId, int32_t deviceId, const std::string& licenseKey)
{
	try
	{
		std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
		if(i == GD::licensingModules.end() || !i->second)
		{
			GD::out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
			return -2;
		}
		return i->second->checkLicense(familyId, deviceId, licenseKey);
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
    return -1;
}

uint64_t FamilyController::onGetRoomIdByName(std::string& name)
{
	try
	{
		BaseLib::PVariable rooms = GD::bl->db->getRooms(_dummyClientInfo, "", _dummyClientInfo->acls->roomsReadSet());
		for(auto& room : *rooms->arrayValue)
		{
			auto idIterator = room->structValue->find("ID");
			if(idIterator == room->structValue->end()) continue;
			auto translationsIterator = room->structValue->find("TRANSLATIONS");
			if(translationsIterator == room->structValue->end()) continue;
			for(auto& translation : *translationsIterator->second->structValue)
			{
				if(translation.second->stringValue == name) return idIterator->second->integerValue64;
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
    return 0;
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
		onEvent(0, -1, std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>{"HOMEGEAR_STARTED"}), BaseLib::PArray(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(true))}));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			i->second->homegearStarted();
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
		onEvent(0, -1, std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>{"HOMEGEAR_SHUTTINGDOWN"}), BaseLib::PArray(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(true))}));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			i->second->homegearShuttingDown();
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

std::vector<std::shared_ptr<FamilyController::ModuleInfo>> FamilyController::getModuleInfo()
{
	std::vector<std::shared_ptr<FamilyController::ModuleInfo>> moduleInfoVector;
	try
	{
		if(_disposed) return moduleInfoVector;
		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.modulePath());
		if(files.empty()) return moduleInfoVector;
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			std::string licensingPostfix;
			if(i->size() > 15) licensingPostfix = i->substr(i->size() - 12, 9);
			if(extension != ".so" || prefix != "mod_" || licensingPostfix == "licensing") continue;
			std::shared_ptr<FamilyController::ModuleInfo> moduleInfo(new FamilyController::ModuleInfo());
			{
				std::lock_guard<std::mutex> moduleLoadersGuard(_moduleLoadersMutex);
				std::map<std::string, std::unique_ptr<ModuleLoader>>::const_iterator moduleIterator = _moduleLoaders.find(*i);
				if(moduleIterator != _moduleLoaders.end() && moduleIterator->second)
				{
					moduleInfo->baselibVersion = moduleIterator->second->getVersion();
					moduleInfo->loaded = true;
					moduleInfo->filename = moduleIterator->first;
					moduleInfo->familyId = moduleIterator->second->getFamilyId();
					moduleInfo->familyName = moduleIterator->second->getFamilyName();
					moduleInfoVector.push_back(moduleInfo);
					continue;
				}
			}
			std::string path(GD::bl->settings.modulePath() + *i);
			std::unique_ptr<ModuleLoader> moduleLoader(new ModuleLoader(*i, path));
			moduleInfo->baselibVersion = moduleLoader->getVersion();
			moduleInfo->loaded = false;
			moduleInfo->filename = *i;
			moduleInfo->familyId = moduleLoader->getFamilyId();
			moduleInfo->familyName = moduleLoader->getFamilyName();
			moduleInfoVector.push_back(moduleInfo);
			moduleLoader->dispose();
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
    return moduleInfoVector;
}

int32_t FamilyController::loadModule(std::string filename)
{
	_moduleLoadersMutex.lock();
	try
	{
		if(_disposed)
		{
			_moduleLoadersMutex.unlock();
			return -1;
		}
		std::string path(GD::bl->settings.modulePath() + filename);
		if(!BaseLib::Io::fileExists(path))
		{
			_moduleLoadersMutex.unlock();
			return -2;
		}
		if(_moduleLoaders.find(filename) != _moduleLoaders.end())
		{
			_moduleLoadersMutex.unlock();
			return 1;
		}
		_moduleLoaders.insert(std::pair<std::string, std::unique_ptr<ModuleLoader>>(filename, std::unique_ptr<ModuleLoader>(new ModuleLoader(filename, path))));

		std::shared_ptr<BaseLib::Systems::DeviceFamily> family = _moduleLoaders.at(filename)->createModule(this);

		if(family)
		{
			_moduleFilenames[family->getFamily()] = filename;
			std::string name = family->getName();
			BaseLib::HelperFunctions::toLower(name);
			BaseLib::HelperFunctions::stringReplace(name, " ", "");
			{
				std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
				_families[family->getFamily()] = family;
			}
			if(!family->enabled() || !family->init())
			{
				if(!family->enabled()) GD::out.printInfo("Info: Not initializing device family " + family->getName() + ", because it is disabled in it's configuration file.");
				else GD::out.printError("Error: Could not initialize device family " + family->getName() + ".");
				int32_t familyId = family->getFamily();
				family->dispose();
				family.reset();
				{
					std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
					_families[familyId].reset();
				}
				_moduleLoaders.at(filename)->dispose();
				_moduleLoaders.erase(filename);
				_moduleLoadersMutex.unlock();
				return -4;
			}
			family->load();
			family->physicalInterfaces()->startListening();
			family->homegearStarted();
		}
		else
		{
			_moduleLoaders.at(filename)->dispose();
			_moduleLoaders.erase(filename);
			_moduleLoadersMutex.unlock();
			return -3;
		}
		_rpcCache.reset();
		_moduleLoadersMutex.unlock();
		return 0;
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
    _moduleLoadersMutex.unlock();
    return -1;
}

int32_t FamilyController::unloadModule(std::string filename)
{
	_moduleLoadersMutex.lock();
	try
	{
		GD::out.printInfo("Info: Unloading " + filename + "...");
		if(_disposed)
		{
			_moduleLoadersMutex.unlock();
			return -1;
		}
		std::string path(GD::bl->settings.modulePath() + filename);
		if(!BaseLib::Io::fileExists(path))
		{
			_moduleLoadersMutex.unlock();
			return -2;
		}

		std::map<std::string, std::unique_ptr<ModuleLoader>>::iterator moduleLoaderIterator = _moduleLoaders.find(filename);
		if(moduleLoaderIterator == _moduleLoaders.end())
		{
			_moduleLoadersMutex.unlock();
			return 1;
		}

		std::shared_ptr<BaseLib::Systems::DeviceFamily> family = getFamily(moduleLoaderIterator->second->getFamilyId());
		if(family)
		{
			{
				std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
				family->lock();
			}

			if(_currentFamily && family->getFamily() == _currentFamily->getFamily()) _currentFamily.reset();

			family->homegearShuttingDown();
			family->physicalInterfaces()->stopListening();
			while(family.use_count() > 2) //At this point, the node is used by "_families" and "family".
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			family->save(false);
			int32_t familyId = family->getFamily();
			family->dispose();
			family.reset();
			std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
			_families[familyId].reset();
		}

		moduleLoaderIterator->second->dispose();
		moduleLoaderIterator->second.reset();
		_moduleLoaders.erase(moduleLoaderIterator);

		_rpcCache.reset();
		_moduleLoadersMutex.unlock();
		GD::out.printInfo("Info: " + filename + " unloaded.");
		return 0;
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
    _moduleLoadersMutex.unlock();
    return -1;
}

int32_t FamilyController::reloadModule(std::string filename)
{
	try
	{
		if(_disposed) return -1;
		int32_t result = unloadModule(filename);
		if(result < 0) return result;
		return loadModule(filename);
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
    return -1;
}

void FamilyController::init()
{
	try
	{
		_dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
		_dummyClientInfo->familyModule = true;
		_dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
		std::vector<uint64_t> groups{ 7 };
		_dummyClientInfo->acls->fromGroups(groups);
		_dummyClientInfo->user = "SYSTEM (7)";
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

void FamilyController::loadModules()
{
	_moduleLoadersMutex.lock();
	try
	{
		GD::out.printDebug("Debug: Loading family modules");

		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.modulePath());
		if(files.empty())
		{
			GD::out.printCritical("Critical: No family modules found in \"" + GD::bl->settings.modulePath() + "\".");
			_moduleLoadersMutex.unlock();
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

			_moduleLoaders.insert(std::pair<std::string, std::unique_ptr<ModuleLoader>>(*i, std::unique_ptr<ModuleLoader>(new ModuleLoader(*i, path))));

			std::shared_ptr<BaseLib::Systems::DeviceFamily> family = _moduleLoaders.at(*i)->createModule(this);

			if(family)
			{
				_moduleFilenames[family->getFamily()] = *i;
				std::string name = family->getName();
				BaseLib::HelperFunctions::toLower(name);
				BaseLib::HelperFunctions::stringReplace(name, " ", "");
				std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
				_families[family->getFamily()] = family;
			}
			else
			{
				_moduleLoaders.at(*i)->dispose();
				_moduleLoaders.erase(*i);
			}
		}
		if(_families.empty())
		{
			GD::out.printCritical("Critical: Could not load any family modules from \"" + GD::bl->settings.modulePath() + "\".");
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
    _moduleLoadersMutex.unlock();
}

void FamilyController::load()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(!i->second->enabled() || !i->second->init())
			{
				if(!i->second->enabled()) GD::out.printInfo("Info: Not initializing device family " + i->second->getName() + ", because it is disabled in it's configuration file.");
				else GD::out.printError("Error: Could not initialize device family " + i->second->getName() + ".");
				i->second->dispose();
				i->second.reset();
				_families[i->first].reset();
				_moduleLoadersMutex.lock();
				std::map<std::string, std::unique_ptr<ModuleLoader>>::iterator moduleIterator = _moduleLoaders.find(_moduleFilenames[i->first]);
				if(moduleIterator != _moduleLoaders.end())
				{
					moduleIterator->second->dispose();
					_moduleLoaders.erase(moduleIterator);
				}
				_moduleLoadersMutex.unlock();
				continue;
			}
			i->second->load();
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

void FamilyController::disposeDeviceFamilies()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(!i->second) continue;
			i->second->dispose();
			i->second.reset();
			_families[i->first].reset();
		}
		families.clear();
		_families.clear();
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
		_familiesMutex.lock();
		_currentFamily.reset();
		_families.clear();
		_familiesMutex.unlock();
		_moduleLoadersMutex.lock();
		_moduleLoaders.clear();
		_moduleLoadersMutex.unlock();
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

bool FamilyController::lifetick()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(!i->second->lifetick()) return false;
		}
		return true;
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

void FamilyController::save(bool full)
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(i->second && !i->second->locked()) i->second->save(full);
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

std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> FamilyController::getFamilies()
{
	std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families;
	try
	{
		std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = _families.begin(); i != _families.end(); ++i)
		{
			if(!i->second || i->second->locked()) continue;
			families.insert(*i);
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
    return families;
}

std::shared_ptr<BaseLib::Systems::DeviceFamily> FamilyController::getFamily(int32_t familyId)
{
	try
	{
		std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
		auto familyIterator = _families.find(familyId);
		std::shared_ptr<BaseLib::Systems::DeviceFamily> family;
		if(familyIterator != _families.end()) family = familyIterator->second;
		if(family && !family->locked()) return family;
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
    return std::shared_ptr<BaseLib::Systems::DeviceFamily>();
}

bool FamilyController::peerExists(uint64_t peerId)
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(central->peerExists(peerId))
				{
					if(i->second->locked()) return false;
					return true;
				}
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

std::string FamilyController::handleCliCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if((command == "unselect" || command == "u") && _currentFamily && !_currentFamily->peerSelected())
		{
			_currentFamily.reset();
			return "Device family unselected.\n";
		}
		else if((command.compare(0, 8, "families") || BaseLib::HelperFunctions::isShortCliCommand(command)) && _currentFamily)
		{
			std::string result = _currentFamily->handleCliCommand(command);
			return result;
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
			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
			for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
			{
				if(i->first == -1) continue;
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
					if(family == -1)
					{
						return "Invalid family id.\n";
					}
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
				std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
				for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
				{
					if(i->first == -1) continue;
					stringStream << "  FAMILYID: 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)i->first << ":\t" << i->second->getName() << std::endl << std::dec;
				}
				return stringStream.str();
			}
			_currentFamily = getFamily(family);
			if(!_currentFamily)
			{
				stringStream << "Device family not found." << std::endl;
				return stringStream.str();
			}

			stringStream << "Device family \"" << _currentFamily->getName() << "\" selected." << std::endl;
			stringStream << "For information about the family's commands type: \"help\"" << std::endl;

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

BaseLib::PVariable FamilyController::listFamilies(int32_t familyId)
{
	try
	{
		if(_rpcCache) return _rpcCache;

		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));
		if(_disposed) return array;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
            if(familyId != -1 && i->first != familyId) continue;
			std::shared_ptr<BaseLib::Systems::ICentral> central = families.at(i->first)->getCentral();
			if(!central) continue;

			BaseLib::PVariable familyDescription(new BaseLib::Variable(BaseLib::VariableType::tStruct));

			familyDescription->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable((int32_t)i->first))));
			familyDescription->structValue->insert(BaseLib::StructElement("NAME", BaseLib::PVariable(new BaseLib::Variable(i->second->getName()))));
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

uint32_t FamilyController::physicalInterfaceCount(int32_t family)
{
	uint32_t size = 0;
	try
	{
		std::shared_ptr<BaseLib::Systems::DeviceFamily> pFamily = getFamily(family);
		if(pFamily) size = pFamily->physicalInterfaces()->count();
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
    return size;
}

bool FamilyController::physicalInterfaceIsOpen()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(!i->second->hasPhysicalInterface()) continue;
			std::shared_ptr<BaseLib::Systems::PhysicalInterfaces> interfaces = i->second->physicalInterfaces();
			if(!interfaces->isOpen()) return false;
		}
		return true;
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

void FamilyController::physicalInterfaceStartListening()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			i->second->physicalInterfaces()->startListening();
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

void FamilyController::physicalInterfaceStopListening()
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			i->second->physicalInterfaces()->stopListening();
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

void FamilyController::physicalInterfaceSetup(int32_t userID, int32_t groupID, bool setPermissions)
{
	try
	{
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			i->second->physicalInterfaces()->setup(userID, groupID, setPermissions);
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

BaseLib::PVariable FamilyController::listInterfaces(int32_t familyId)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));

		if(familyId != -1)
		{
			auto family = getFamily(familyId);
			if(!family) return array;
			return family->physicalInterfaces()->listInterfaces();
		}
		else
		{
			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
			for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
			{
				BaseLib::PVariable tempArray = i->second->physicalInterfaces()->listInterfaces();
				array->arrayValue->insert(array->arrayValue->end(), tempArray->arrayValue->begin(), tempArray->arrayValue->end());
			}
		}

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
