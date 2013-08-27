#include "BidCoSPacketManager.h"
#include "GD.h"
#include "HelperFunctions.h"

BidCoSPacketInfo::BidCoSPacketInfo()
{
	time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BidCoSPacketManager::dispose(bool wait)
{
	_disposing = true;
	if(wait) std::this_thread::sleep_for(std::chrono::milliseconds(1500)); //Wait for threads to finish
}

void BidCoSPacketManager::set(int32_t address, std::shared_ptr<BidCoSPacket>& packet, int64_t time)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets.erase(_packets.find(address));
		_packetMutex.unlock();

		std::shared_ptr<BidCoSPacketInfo> info(new BidCoSPacketInfo());
		info->packet = packet;
		info->id = _id++;
		if(time > 0) info->time = time;
		std::thread t(&BidCoSPacketManager::deletePacket, this, address, info->id);
		t.detach();
		_packetMutex.lock();
		_packets.insert(std::pair<int32_t, std::shared_ptr<BidCoSPacketInfo>>(address, info));
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
    _packetMutex.unlock();
}

void BidCoSPacketManager::deletePacket(int32_t address, uint32_t id)
{
	try
	{
		std::chrono::milliseconds sleepingTime(400);
		while(true)
		{
			if(_disposing) return;
			_packetMutex.lock();
			if(_packets.find(address) != _packets.end() && _packets.at(address) && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _packets.at(address)->time + 1000)
			{
				_packetMutex.unlock();
				std::this_thread::sleep_for(sleepingTime);
				if(_disposing) return;
			}
			else if(_disposing)
			{
				_packetMutex.unlock();
				return;
			}
			else
			{
				_packetMutex.unlock();
				break;
			}
		}
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end() && _packets.at(address) && _packets.at(address)->id == id)
		{
			_packets.erase(address);
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
    _packetMutex.unlock();
}

std::shared_ptr<BidCoSPacket> BidCoSPacketManager::get(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSPacket>();
		_packetMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSPacket> packet((_packets.find(address) != _packets.end()) ? _packets[address]->packet : nullptr);
		_packetMutex.unlock();
		return packet;
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
    _packetMutex.unlock();
    return std::shared_ptr<BidCoSPacket>();
}

std::shared_ptr<BidCoSPacketInfo> BidCoSPacketManager::getInfo(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSPacketInfo>();
		//Make a copy to make sure, the element exists
		_packetMutex.lock();
		std::shared_ptr<BidCoSPacketInfo> info((_packets.find(address) != _packets.end()) ? _packets[address] : nullptr);
		_packetMutex.unlock();
		return info;
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
    _packetMutex.unlock();
    return std::shared_ptr<BidCoSPacketInfo>();
}

void BidCoSPacketManager::keepAlive(int32_t address)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets[address]->time = HelperFunctions::getTime();
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
    _packetMutex.unlock();
}
