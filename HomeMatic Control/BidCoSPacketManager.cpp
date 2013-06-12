#include "BidCoSPacketManager.h"
#include "GD.h"

BidCoSPacketInfo::BidCoSPacketInfo()
{
	time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BidCoSPacketManager::~BidCoSPacketManager()
{
	for(std::unordered_map<int32_t, std::shared_ptr<BidCoSPacketInfo>>::iterator i = _packets.begin(); i != _packets.end(); ++i)
	{
		i->second->stopThread = true;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Wait for threads to finish
}

void BidCoSPacketManager::set(int32_t address, std::shared_ptr<BidCoSPacket>& packet)
{
	try
	{
		if(_packets.find(address) != _packets.end() && _packets[address]->thread) _packets[address]->stopThread = true;
		if(_packets.find(address) != _packets.end()) _packets.erase(_packets.find(address));

		std::shared_ptr<BidCoSPacketInfo> info(new BidCoSPacketInfo());
		info->packet = packet;
		info->id = _id++;
		info->thread = std::shared_ptr<std::thread>(new std::thread(&BidCoSPacketManager::deletePacket, this, address, info->id));
		info->thread->detach();
		_packetMutex.lock();
		_packets.insert(std::pair<int32_t, std::shared_ptr<BidCoSPacketInfo>>(address, info));
		_packetMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _packetMutex.unlock();
    }
}

void BidCoSPacketManager::deletePacket(int32_t address, uint32_t id)
{
	try
	{
		std::chrono::milliseconds sleepingTime(50);
		while(_packets.find(address) != _packets.end() && !_packets[address]->stopThread && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _packets[address]->time + 500)
		{
			std::this_thread::sleep_for(sleepingTime);
		}
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end() && !_packets[address]->stopThread && _packets[address]->id == id)
		{
			if(GD::debugLevel >= 8) std::cout << "Deleting packet " << id << " for 0x" << std::hex << address << std::dec << std::endl;
			_packets.erase(_packets.find(address));
		}
		_packetMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _packetMutex.unlock();
    }
}

std::shared_ptr<BidCoSPacket> BidCoSPacketManager::get(int32_t address)
{
	try
	{
		_packetMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSPacket> packet((_packets.find(address) != _packets.end()) ? _packets[address]->packet : nullptr);
		_packetMutex.unlock();
		return packet;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _packetMutex.unlock();
    }
    return std::shared_ptr<BidCoSPacket>();
}

std::shared_ptr<BidCoSPacketInfo> BidCoSPacketManager::getInfo(int32_t address)
{
	try
	{
		_packetMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSPacketInfo> info((_packets.find(address) != _packets.end()) ? _packets[address] : nullptr);
		_packetMutex.unlock();
		return info;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _packetMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _packetMutex.unlock();
    }
    return std::shared_ptr<BidCoSPacketInfo>();
}

void BidCoSPacketManager::keepAlive(int32_t address)
{
	if(_packets.find(address) != _packets.end()) _packets[address]->time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
