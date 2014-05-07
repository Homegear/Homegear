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

#ifndef BASEFACTORY_H_
#define BASEFACTORY_H_

#include "Systems/PhysicalDevice.h"
#include "Output/Output.h"
#include "HelperFunctions/HelperFunctions.h"
#include "FileDescriptorManager/FileDescriptorManager.h"
#include "Database/Database.h"
#include "Metadata/Metadata.h"
#include "RPC/Device.h"
#include "RPC/Devices.h"

class BaseFactory
{
public:
	BaseFactory();
	virtual ~BaseFactory();

	virtual bool init(std::shared_ptr<FileDescriptorManager> fileDescriptorManager, std::shared_ptr<Database> db);

	virtual bool setDatabase(std::shared_ptr<Database> db);

	virtual std::shared_ptr<PhysicalDevices::PhysicalDevice> createPhysicalDevice(std::shared_ptr<Output> output, std::shared_ptr<HelperFunctions> helperFunctions, std::shared_ptr<FileDescriptorManager> fileDescriptorManager, std::string gpioPath)
	{
		return std::shared_ptr<PhysicalDevices::PhysicalDevice>(new PhysicalDevices::PhysicalDevice(gpioPath));
	}

	virtual std::shared_ptr<PhysicalDevices::PhysicalDevice> createPhysicalDevice(std::shared_ptr<Output> output, std::shared_ptr<HelperFunctions> helperFunctions, std::shared_ptr<FileDescriptorManager> fileDescriptorManager, std::string gpioPath, std::shared_ptr<PhysicalDevices::PhysicalDeviceSettings> settings)
	{
		return std::shared_ptr<PhysicalDevices::PhysicalDevice>(new PhysicalDevices::PhysicalDevice(gpioPath, settings));
	}

	virtual std::shared_ptr<Output> createOutput()
	{
		return std::shared_ptr<Output>(new Output);
	}

	virtual std::shared_ptr<HelperFunctions> createHelperFunctions()
	{
		return std::shared_ptr<HelperFunctions>(new HelperFunctions);
	}

	virtual std::shared_ptr<FileDescriptorManager> createFileDescriptorManager()
	{
		return std::shared_ptr<FileDescriptorManager>(new FileDescriptorManager);
	}

	virtual std::shared_ptr<Database> createDatabase()
	{
		return std::shared_ptr<Database>(new Database);
	}

	virtual std::shared_ptr<Metadata> createMetadata()
	{
		return std::shared_ptr<Metadata>(new Metadata);
	}

	virtual std::unique_ptr<RPC::ParameterSet> createParameterSet()
	{
		return std::unique_ptr<RPC::ParameterSet>(new RPC::ParameterSet);
	}

	virtual std::unique_ptr<RPC::Devices> createRPCDevices()
	{
		return std::unique_ptr<RPC::Devices>(new RPC::Devices);
	}
};

extern "C"
{
	BaseFactory* create()
	{
		return new BaseFactory;
	}
}

#endif /* BASEFACTORY_H_ */
