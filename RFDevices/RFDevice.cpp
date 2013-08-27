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
		std::vector<HomeMaticDevice*> devices;
		_homeMaticDevicesMutex.lock();
		//We need to copy all elements. In packetReceived so much can happen, that _homeMaticDevicesMutex might deadlock
		for(std::list<HomeMaticDevice*>::const_iterator i = _homeMaticDevices.begin(); i != _homeMaticDevices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_homeMaticDevicesMutex.unlock();
		for(std::vector<HomeMaticDevice*>::iterator i = devices.begin(); i != devices.end(); ++i)
		{
			(*i)->packetReceived(packet);
		}
	}
    catch(const std::exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

}
