#include "RFDevice.h"
#include "../GD.h"
#include "Cul.h"
#ifdef TI_CC1100
#include "TICC1100.h"
#endif

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
#ifdef TI_CC1100
		else if(rfDeviceType == "cc1100")
		{
			return std::shared_ptr<RFDevice>(new TICC1100());
		}
#endif
		else
		{
			if(GD::debugLevel >= 1) std::cerr << "Error: Unsupported rf device type: " << rfDeviceType << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
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
			received.detach();
		}
		_homeMaticDevicesMutex.unlock();
	}
    catch(const std::exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_homeMaticDevicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

}
