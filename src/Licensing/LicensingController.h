/* Copyright 2013-2019 Homegear GmbH
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

#ifndef LICENSINGCONTROLLER_H_
#define LICENSINGCONTROLLER_H_

#include <homegear-base/BaseLib.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <dlfcn.h>

namespace Homegear
{

class LicensingModuleLoader
{
public:
	LicensingModuleLoader(std::string name, std::string path);

	virtual ~LicensingModuleLoader();

	std::unique_ptr<BaseLib::Licensing::Licensing> createModule();

private:
	std::string _name;
	void* _handle = nullptr;
	std::unique_ptr<BaseLib::Licensing::LicensingFactory> _factory;

	LicensingModuleLoader(const LicensingModuleLoader&);

	LicensingModuleLoader& operator=(const LicensingModuleLoader&);
};

class LicensingController
{
public:
	LicensingController();

	virtual ~LicensingController();

	void init();

	void dispose();

	void load();

	void loadModules();

	bool moduleAvailable(int32_t moduleId);

private:
	bool _disposed = false;

	std::map<std::string, std::unique_ptr<LicensingModuleLoader>> moduleLoaders;

	LicensingController(const LicensingController&);

	LicensingController& operator=(const LicensingController&);
};

}

#endif
