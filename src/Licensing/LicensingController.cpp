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

#include "LicensingController.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Homegear
{

LicensingModuleLoader::LicensingModuleLoader(std::string name, std::string path)
{
	try
	{
		_name = name;
		GD::out.printInfo("Info: Loading licensing module " + _name);

		void* moduleHandle = dlopen(path.c_str(), RTLD_NOW);
		if(!moduleHandle)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\": " + std::string(dlerror()));
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
		std::string version = getVersion();
		if(GD::bl->version() != version)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Module is compiled for Homegear version " + version);
			dlclose(moduleHandle);
			return;
		}


		BaseLib::Licensing::LicensingFactory* (* getFactory)();
		getFactory = (BaseLib::Licensing::LicensingFactory* (*)()) dlsym(moduleHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFactory\" not found.");
			dlclose(moduleHandle);
			return;
		}

		_handle = moduleHandle;
		_factory = std::unique_ptr<BaseLib::Licensing::LicensingFactory>(getFactory());
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

LicensingModuleLoader::~LicensingModuleLoader()
{
	try
	{
		GD::out.printInfo("Info: Disposing licensing module " + _name);
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

std::unique_ptr<BaseLib::Licensing::Licensing> LicensingModuleLoader::createModule()
{
	if(!_factory) return std::unique_ptr<BaseLib::Licensing::Licensing>();
	return std::unique_ptr<BaseLib::Licensing::Licensing>(_factory->createLicensing(GD::bl.get()));
}

LicensingController::LicensingController()
{
}

LicensingController::~LicensingController()
{
	dispose();
}

bool LicensingController::moduleAvailable(int32_t moduleId)
{
	return GD::licensingModules.find(moduleId) != GD::licensingModules.end();
}

void LicensingController::loadModules()
{
	try
	{
		GD::out.printDebug("Debug: Loading licensing modules");

		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.modulePath());
		if(files.empty())
		{
			GD::out.printDebug("Debug: No licensing modules found in \"" + GD::bl->settings.modulePath() + "\".");
			return;
		}
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			std::string licensingPostfix;
			if(i->size() > 15) licensingPostfix = i->substr(i->size() - 12, 9);
			if(extension != ".so" || prefix != "mod_" || licensingPostfix != "licensing") continue;
			std::string path(GD::bl->settings.modulePath() + *i);

			moduleLoaders.insert(std::pair<std::string, std::unique_ptr<LicensingModuleLoader>>(*i, std::unique_ptr<LicensingModuleLoader>(new LicensingModuleLoader(*i, path))));

			std::unique_ptr<BaseLib::Licensing::Licensing> licensingModule = moduleLoaders.at(*i)->createModule();

			if(licensingModule) GD::licensingModules[licensingModule->getModuleId()].swap(licensingModule);
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

void LicensingController::init()
{
	try
	{
		std::vector<int32_t> modulesToRemove;
		for(std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.begin(); i != GD::licensingModules.end(); ++i)
		{
			if(!i->second->init()) modulesToRemove.push_back(i->first);
		}
		for(std::vector<int32_t>::iterator i = modulesToRemove.begin(); i != modulesToRemove.end(); ++i)
		{
			GD::licensingModules.at(*i)->dispose(); //Calls dispose
			GD::licensingModules.erase(*i);
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

void LicensingController::load()
{
	try
	{
		for(std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.begin(); i != GD::licensingModules.end(); ++i)
		{
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


void LicensingController::dispose()
{
	try
	{
		if(_disposed) return;
		_disposed = true;
		if(!GD::licensingModules.empty())
		{
			for(std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.begin(); i != GD::licensingModules.end(); ++i)
			{
				if(!i->second) continue;
				i->second->dispose();
			}
		}
		GD::licensingModules.clear();
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

}