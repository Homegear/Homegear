#include "RFDevice.h"
#include "../GD.h"
#include "../HelperFunctions.h"
#include "Cul.h"
#include "TICC1100.h"

namespace RF
{

RFDevice::RFDevice()
{

}

RFDevice::~RFDevice()
{

}

std::shared_ptr<RFDevice> RFDevice::create(std::string rfDeviceType)
{
	try
	{
		if(rfDeviceType == "cul")
		{
			return std::shared_ptr<RFDevice>(new Cul());
		}
		else if(rfDeviceType == "cc1100")
		{
			return std::shared_ptr<RFDevice>(new TICC1100());
		}
		else
		{
			HelperFunctions::printError("Error: Unsupported rf device type: " + rfDeviceType);
		}
	}
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RFDevice>();
}

void RFDevice::addHomeMaticDevice(HomeMaticDevice* device)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		_homeMaticDevices.push_back(device);
    }
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _homeMaticDevicesMutex.unlock();
}

void RFDevice::removeHomeMaticDevice(HomeMaticDevice* device)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		_homeMaticDevices.remove(device);
    }
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _homeMaticDevicesMutex.unlock();
}

void RFDevice::callCallback(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		for(std::list<HomeMaticDevice*>::const_iterator i = _homeMaticDevices.begin(); i != _homeMaticDevices.end(); ++i)
		{
			//Don't filter destination addresses here! Some devices need to receive packets not directed to them.
			(*i)->packetReceived(packet);
		}
	}
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _homeMaticDevicesMutex.unlock();
}

}
