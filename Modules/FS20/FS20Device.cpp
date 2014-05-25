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

#include "FS20Device.h"
#include "GD.h"

namespace FS20
{

FS20Device::FS20Device(IDeviceEventSink* eventHandler) : BaseLib::Systems::LogicalDevice(eventHandler)
{
}

FS20Device::FS20Device(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : BaseLib::Systems::LogicalDevice(deviceID, serialNumber, address, eventHandler)
{
}

FS20Device::~FS20Device()
{
	try
	{
		dispose();
		//dispose might have been called by another thread, so wait until dispose is finished
		while(!_disposed) std::this_thread::sleep_for(std::chrono::milliseconds(50)); //Wait for received packets, without this, program sometimes SIGABRTs
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

void FS20Device::dispose(bool wait)
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		BaseLib::Output::printDebug("Removing device " + std::to_string(_deviceID) + " from physical device's event queue...");
		if(GD::physicalDevice) GD::physicalDevice->removeEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalDeviceEventSink*)this);
		int64_t startTime = BaseLib::HelperFunctions::getTime();
		//stopThreads();
		int64_t timeDifference = BaseLib::HelperFunctions::getTime() - startTime;
		//Packets might still arrive, after removing this device from the rfDevice, so sleep a little bit
		//This is not necessary if the rfDevice doesn't listen anymore
		if(wait && timeDifference >= 0 && timeDifference < 2000) std::this_thread::sleep_for(std::chrono::milliseconds(2000 - timeDifference));
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
	_disposed = true;
}

void FS20Device::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times

		if(GD::physicalDevice) GD::physicalDevice->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalDeviceEventSink*)this);

		_initialized = true;
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

void FS20Device::loadVariables()
{
	try
	{
		_databaseMutex.lock();
		BaseLib::DataTable rows = BaseLib::Obj::ins->db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=" + std::to_string(_deviceID));
		for(BaseLib::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{

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
	_databaseMutex.unlock();
}

void FS20Device::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;

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

void FS20Device::sendPacket(std::shared_ptr<FS20Packet> packet)
{
	try
	{

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

bool FS20Device::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;

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
    return false;
}
}
