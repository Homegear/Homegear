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

#include "GD.h"

DatabaseController GD::db;
std::string GD::configPath = "/etc/homegear/";
std::string GD::pidfilePath = "";
std::string GD::runDir = "/var/run/homegear/";
std::string GD::socketPath = GD::runDir + "homegear.sock";
std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
FamilyController GD::devices;
std::map<int32_t, RPC::Server> GD::rpcServers;
RPC::Client GD::rpcClient;
CLI::Server GD::cliServer;
CLI::Client GD::cliClient;
int32_t GD::rpcLogLevel = 1;
RPC::ServerSettings GD::serverSettings;
RPC::ClientSettings GD::clientSettings;
EventHandler GD::eventHandler;
PhysicalInterfaces GD::physicalInterfaces;
std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>> GD::deviceFamilies;

/*void GD::init()
{
	loadBaseLibrary();

	output = baseFactory->createOutput();
	try
	{
		helperFunctions = baseFactory->createHelperFunctions();
		fileDescriptorManager = baseFactory->createFileDescriptorManager();
		db = baseFactory->createDatabase();

		bool (*initBase)(std::shared_ptr<FileDescriptorManager> fileDescriptorManager, std::shared_ptr<Database> db);
		initBase = (bool (*)(std::shared_ptr<FileDescriptorManager> fileDescriptorManager, std::shared_ptr<Database> db))dlsym(baseHandle, "init");
		if(!init)
		{
			std::cerr << "Critical: Could not open base library (" + settings.libraryPath() + "mod_base.so). Symbol \"init\" not found." << std::endl;
			exit(1);
		}
		if(!initBase(fileDescriptorManager, db)) exit(1);

		metadata = baseFactory->createMetadata();
		rpcDevices = baseFactory->createRPCDevices();
	}
	catch(const std::exception& ex)
    {
        output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void GD::loadBaseLibrary()
{
	std::string path(settings.libraryPath() + "mod_base.so");
	baseHandle = dlopen(path.c_str(), RTLD_NOW);
	if(!baseHandle)
	{
		std::cerr << "Critical: Could not open base library (" + settings.libraryPath() + "mod_base.so)." << std::endl;
		exit(1);
	}

	BaseFactory* (*create)();
	create = (BaseFactory* (*)())dlsym(baseHandle, "create");
	if(!create)
	{
		std::cerr << "Critical: Could not open base library (" + settings.libraryPath() + "mod_base.so). Symbol \"create\" not found." << std::endl;
		exit(1);
	}
	baseFactory.reset((BaseFactory*)create());
}

void GD::dispose()
{
	if(baseHandle)
	{
		baseFactory.reset();
		dlclose(baseHandle);
		baseHandle = nullptr;
	}
}*/
