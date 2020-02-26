/* Copyright 2013-2020 Homegear GmbH
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

namespace Homegear
{

ModuleLoader::ModuleLoader(std::string name, std::string path)
{
    try
    {
        _name = name;
        GD::out.printInfo("Info: Loading family module (type " + std::to_string((int32_t)BaseLib::Systems::FamilyType::sharedObject) + ") " + _name);

        void* moduleHandle = dlopen(path.c_str(), RTLD_NOW);
        if(!moduleHandle)
        {
            GD::out.printCritical("Critical: Could not open module \"" + path + "\": " + std::string(dlerror()));
            return;
        }

        int32_t (* getFamilyId)();
        getFamilyId = (int32_t (*)()) dlsym(moduleHandle, "getFamilyId");
        if(!getFamilyId)
        {
            GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFamilyId\" not found.");
            dlclose(moduleHandle);
            return;
        }
        _familyId = getFamilyId();
        if(_familyId < 0 || _familyId > 1023) //Family IDs from 1024 onwards are reserved for dynamic allocation
        {
            GD::out.printCritical("Critical: Could not open module \"" + path + "\". Got invalid family id. Only family IDs between 0 and 1023 are valid.");
            dlclose(moduleHandle);
            return;
        }

        std::string (* getFamilyName)();
        getFamilyName = (std::string (*)()) dlsym(moduleHandle, "getFamilyName");
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

        std::string (* getVersion)();
        getVersion = (std::string (*)()) dlsym(moduleHandle, "getVersion");
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

        BaseLib::Systems::SystemFactory* (* getFactory)();
        getFactory = (BaseLib::Systems::SystemFactory* (*)()) dlsym(moduleHandle, "getFactory");
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

std::unique_ptr<BaseLib::Systems::DeviceFamily> ModuleLoader::createModule(BaseLib::Systems::IFamilyEventSink* eventHandler)
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
    for(auto& server : GD::rpcServers)
    {
        eventHandlers[server.first] = server.second->addWebserverEventHandler(eventHandler);
    }
}

void FamilyController::onRemoveWebserverEventHandler(std::map<int32_t, BaseLib::PEventHandler>& eventHandlers)
{
    for(auto& server : GD::rpcServers)
    {
        server.second->removeWebserverEventHandler(eventHandlers[server.first]);
    }
}

void FamilyController::onRPCEvent(std::string source, uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<BaseLib::PVariable>> values)
{
    try
    {
        GD::rpcClient->broadcastEvent(source, id, channel, deviceAddress, valueKeys, values);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void FamilyController::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
    try
    {
        GD::rpcClient->broadcastUpdateDevice(id, channel, address, (Rpc::Client::Hint::Enum) hint);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void FamilyController::onRPCNewDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceDescriptions)
{
    try
    {
        GD::rpcClient->broadcastNewDevices(ids, deviceDescriptions);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void FamilyController::onRPCDeleteDevices(std::vector<uint64_t>& ids, BaseLib::PVariable deviceAddresses, BaseLib::PVariable deviceInfo)
{
    try
    {
        GD::rpcClient->broadcastDeleteDevices(ids, deviceAddresses, deviceInfo);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void FamilyController::onEvent(std::string source, uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values)
{
    try
    {
        if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(source, peerID, channel, variables, values);
#ifdef EVENTHANDLER
        GD::eventHandler->trigger(peerID, channel, variables, values);
#endif
#ifndef NO_SCRIPTENGINE
        GD::scriptEngineServer->broadcastEvent(source, peerID, channel, variables, values);
#endif
        if(GD::ipcServer) GD::ipcServer->broadcastEvent(source, peerID, channel, variables, values);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
}

BaseLib::PVariable FamilyController::onInvokeRpc(std::string& methodName, BaseLib::PArray& parameters)
{
    try
    {
        auto parameterVariable = std::make_shared<BaseLib::Variable>(parameters);
        return GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, methodName, parameterVariable);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Homegear is compiled without script engine.");
}

int32_t FamilyController::onCheckLicense(int32_t moduleId, int32_t familyId, int32_t deviceId, const std::string& licenseKey)
{
    try
    {
        auto licensingModulesIterator = GD::licensingModules.find(moduleId);
        if(licensingModulesIterator == GD::licensingModules.end() || !licensingModulesIterator->second)
        {
            GD::out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
            return -2;
        }
        return licensingModulesIterator->second->checkLicense(familyId, deviceId, licenseKey);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
    return 0;
}

void FamilyController::onDecryptDeviceDescription(int32_t moduleId, const std::vector<char>& input, std::vector<char>& output)
{
    try
    {
        auto licensingModulesIterator = GD::licensingModules.find(moduleId);
        if(licensingModulesIterator == GD::licensingModules.end() || !licensingModulesIterator->second)
        {
            GD::out.printError("Error: Could not decrypt device description file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
            return;
        }
        licensingModulesIterator->second->decryptDeviceDescription(input, output);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}
//End Device event handling

void FamilyController::homegearStarted()
{
    try
    {
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
        for(auto& family : families)
        {
            family.second->homegearStarted();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void FamilyController::homegearShuttingDown()
{
    try
    {
        GD::out.printInfo("Info: Shutting down shared object family modules...");
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
        for(auto& family : families)
        {
            GD::out.printInfo("Info: Shutting down shared object family module \"" + family.second->getName() + "\"...");
            family.second->homegearShuttingDown();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

std::vector<std::shared_ptr<FamilyModuleInfo>> FamilyController::getModuleInfo()
{
    std::vector<std::shared_ptr<FamilyModuleInfo>> moduleInfoVector;
    try
    {
        std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.modulePath());
        if(files.empty()) return moduleInfoVector;
        moduleInfoVector.reserve(files.size());
        for(auto& filename : files)
        {
            if(filename.size() < 9) continue; //mod_?*.so
            std::string prefix = filename.substr(0, 4);
            std::string extension = filename.substr(filename.size() - 3, 3);
            std::string licensingPostfix;
            if(filename.size() > 15) licensingPostfix = filename.substr(filename.size() - 12, 9);
            if(extension != ".so" || prefix != "mod_" || licensingPostfix == "licensing") continue;
            auto moduleInfo = std::make_shared<FamilyModuleInfo>();
            {
                std::lock_guard<std::mutex> moduleLoadersGuard(_moduleLoadersMutex);
                std::map<std::string, std::unique_ptr<ModuleLoader>>::const_iterator moduleIterator = _moduleLoaders.find(filename);
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
            std::string path(GD::bl->settings.modulePath() + filename);
            std::unique_ptr<ModuleLoader> moduleLoader(new ModuleLoader(filename, path));
            moduleInfo->baselibVersion = moduleLoader->getVersion();
            moduleInfo->loaded = false;
            moduleInfo->filename = filename;
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
    return moduleInfoVector;
}

int32_t FamilyController::loadModule(const std::string& filename)
{
    _moduleLoadersMutex.lock();
    try
    {
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
            {
                std::unique_lock<std::mutex> familiesGuard(_familiesMutex);
                auto familyIterator = _families.find(family->getFamily());
                if(familyIterator != _families.end() && familyIterator->second)
                {
                    GD::out.printError("Error: Could not load family " + family->getName() + ", because a family with ID " + std::to_string(family->getFamily()) + " is already loaded.");
                    familiesGuard.unlock();
                    family->dispose();
                    family.reset();
                    _moduleLoaders.at(filename)->dispose();
                    _moduleLoaders.erase(filename);
                    _moduleLoadersMutex.unlock();
                    return -3;
                }
            }

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
            return -5;
        }
        _moduleLoadersMutex.unlock();
        return 0;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    _moduleLoadersMutex.unlock();
    return -1;
}

int32_t FamilyController::unloadModule(const std::string& filename)
{
    _moduleLoadersMutex.lock();
    try
    {
        GD::out.printInfo("Info: Unloading " + filename + "...");
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

        _moduleLoadersMutex.unlock();
        GD::out.printInfo("Info: " + filename + " unloaded.");
        return 0;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    _moduleLoadersMutex.unlock();
    return -1;
}

int32_t FamilyController::reloadModule(const std::string& filename)
{
    try
    {
        int32_t result = unloadModule(filename);
        if(result < 0) return result;
        return loadModule(filename);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
        std::vector<uint64_t> groups{7};
        _dummyClientInfo->acls->fromGroups(groups);
        _dummyClientInfo->user = "SYSTEM (7)";
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
    _moduleLoadersMutex.unlock();
}

void FamilyController::load()
{
    try
    {
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
        for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
        {
            if(i->second->type() == BaseLib::Systems::FamilyType::sharedObject)
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
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
}

void FamilyController::dispose()
{
    try
    {
        _familiesMutex.lock();
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
}

std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> FamilyController::getFamilies()
{
    std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families;
    try
    {
        std::lock_guard<std::mutex> familiesGuard(_familiesMutex);
        for(auto& family : _families)
        {
            if(!family.second || family.second->locked()) continue;
            families.insert(family);
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
    return std::shared_ptr<BaseLib::Systems::DeviceFamily>();
}

bool FamilyController::peerExists(uint64_t peerId)
{
    try
    {
        std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
        for(auto& family : families)
        {
            if(family.second->locked()) continue;
            std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
            if(central && central->peerExists(peerId)) return true;
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
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
}

BaseLib::PVariable FamilyController::listInterfaces(int32_t familyId)
{
    auto interfaces = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    try
    {
        if(familyId != -1)
        {
            auto family = getFamily(familyId);
            if(!family) return interfaces;
            auto tempArray = family->physicalInterfaces()->listInterfaces();
            if(!tempArray->arrayValue->empty()) interfaces->arrayValue->insert(interfaces->arrayValue->end(), tempArray->arrayValue->begin(), tempArray->arrayValue->end());
            return interfaces;
        }
        else
        {
            std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = getFamilies();
            for(auto& family : families)
            {
                auto tempArray = family.second->physicalInterfaces()->listInterfaces();
                if(!tempArray->arrayValue->empty()) interfaces->arrayValue->insert(interfaces->arrayValue->end(), tempArray->arrayValue->begin(), tempArray->arrayValue->end());
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return interfaces;
}

BaseLib::PVariable FamilyController::listFamilies(int32_t familyId)
{
    try
    {
        auto array = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

        auto families = getFamilies();
        for(auto& family : families)
        {
            if(familyId != -1 && family.first != familyId) continue;

            auto familyDescription = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

            familyDescription->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable((int32_t) family.first))));
            familyDescription->structValue->insert(BaseLib::StructElement("NAME", BaseLib::PVariable(new BaseLib::Variable(family.second->getName()))));
            array->arrayValue->emplace_back(std::move(familyDescription));
        }

        return array;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}