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
		_homeMaticDevicesMutex.unlock();
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

void RFDevice::removeHomeMaticDevice(HomeMaticDevice* device)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		_homeMaticDevices.remove(device);
		_homeMaticDevicesMutex.unlock();
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

void RFDevice::callCallback(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		for(std::list<HomeMaticDevice*>::const_iterator i = _homeMaticDevices.begin(); i != _homeMaticDevices.end(); ++i)
		{
			//Don't filter destination addresses here! Some devices need to receive packets not directed to them.
			std::thread received(&HomeMaticDevice::packetReceived, (*i), packet);
			sched_param schedParam;
			int policy;
			pthread_getschedparam(received.native_handle(), &policy, &schedParam);
			schedParam.sched_priority = 70;
			if(!pthread_setschedparam(received.native_handle(), SCHED_FIFO, &schedParam)) throw(Exception("Error: Could not set thread priority."));
			received.detach();
		}
		_homeMaticDevicesMutex.unlock();
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
