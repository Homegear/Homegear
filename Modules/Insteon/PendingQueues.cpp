/* Copyright 2013-2015 Sathya Laufer
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

#include "PendingQueues.h"
#include "GD.h"
#include "InsteonMessage.h"

namespace Insteon
{
PendingQueues::PendingQueues()
{
}

void PendingQueues::serialize(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(GD::bl);
		_queuesMutex.lock();
		encoder.encodeInteger(encodedData, _queues.size());
		for(std::deque<std::shared_ptr<PacketQueue>>::iterator i = _queues.begin(); i != _queues.end(); ++i)
		{
			std::vector<uint8_t> serializedQueue;
			(*i)->serialize(serializedQueue);
			encoder.encodeInteger(encodedData, serializedQueue.size());
			encodedData.insert(encodedData.end(), serializedQueue.begin(), serializedQueue.end());
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_queuesMutex.unlock();
}

void PendingQueues::unserialize(std::shared_ptr<std::vector<char>> serializedData, InsteonPeer* peer, InsteonDevice* device)
{
	try
	{
		BaseLib::BinaryDecoder decoder(GD::bl);
		uint32_t position = 0;
		_queuesMutex.lock();
		uint32_t pendingQueuesSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < pendingQueuesSize; i++)
		{
			uint32_t queueLength = decoder.decodeInteger(*serializedData, position);
			std::shared_ptr<PacketQueue> queue(new PacketQueue());
			queue->unserialize(serializedData, device, position);
			position += queueLength;
			queue->noSending = true;
			queue->pendingQueueID = _currentID++;
			_queues.push_back(queue);
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

bool PendingQueues::empty()
{
	try
	{
		_queuesMutex.lock();
		bool empty = _queues.empty();
		_queuesMutex.unlock();
		return empty;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
    return false;
}

void PendingQueues::push(std::shared_ptr<PacketQueue> queue)
{
	try
	{
		if(!queue || queue->isEmpty()) return;
		_queuesMutex.lock();
		queue->pendingQueueID = _currentID++;
		_queues.push_back(queue);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

void PendingQueues::pop()
{
	try
	{
		_queuesMutex.lock();
		if(!_queues.empty()) _queues.pop_front();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

void PendingQueues::pop(uint32_t id)
{
	try
	{
		_queuesMutex.lock();
		if(!_queues.empty() && _queues.front()->pendingQueueID == id) _queues.pop_front();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

void PendingQueues::clear()
{
	try
	{
		_queuesMutex.lock();
		_queues.clear();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

uint32_t PendingQueues::size()
{
	try
	{
		_queuesMutex.lock();
		uint32_t size = _queues.size();
		_queuesMutex.unlock();
		return size;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
    return 0;
}

std::shared_ptr<PacketQueue> PendingQueues::front()
{
	try
	{
		std::shared_ptr<PacketQueue> queue;
		_queuesMutex.lock();
		if(!_queues.empty()) queue =_queues.front();
		_queuesMutex.unlock();
		return queue;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
    return std::shared_ptr<PacketQueue>();
}

void PendingQueues::remove(std::string parameterName, int32_t channel)
{
	try
	{
		if(parameterName.empty()) return;
		_queuesMutex.lock();
		if(_queues.empty())
		{
			_queuesMutex.unlock();
			return;
		}
		for(int32_t i = _queues.size() - 1; i >= 0; i--)
		{
			if(!_queues.at(i) || (_queues.at(i)->parameterName == parameterName && _queues.at(i)->channel == channel)) _queues.erase(_queues.begin() + i);
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
}

bool PendingQueues::exists(std::string parameterName, int32_t channel)
{
	try
	{
		if(parameterName.empty()) return false;
		_queuesMutex.lock();
		if(_queues.empty())
		{
			_queuesMutex.unlock();
			return false;
		}
		for(int32_t i = _queues.size() - 1; i >= 0; i--)
		{
			if(!_queues.at(i)) continue;
			if(_queues.at(i)->parameterName == parameterName && _queues.at(i)->channel == channel)
			{
				_queuesMutex.unlock();
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queuesMutex.unlock();
    return false;
}

bool PendingQueues::find(PacketQueueType queueType)
{
	try
	{
		_queuesMutex.lock();
		for(std::deque<std::shared_ptr<PacketQueue>>::iterator i = _queues.begin(); i != _queues.end(); ++i)
		{
			if(*i && (*i)->getQueueType() == queueType)
			{
				_queuesMutex.unlock();
				return true;
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_queuesMutex.unlock();
	return false;
}

void PendingQueues::getInfoString(std::ostringstream& stringStream)
{
	try
	{
		_queuesMutex.lock();
		stringStream << "Number of Pending queues: " << _queues.size() << std::endl;
		int32_t j = 1;
		for(std::deque<std::shared_ptr<PacketQueue>>::iterator i = _queues.begin(); i != _queues.end(); ++i)
		{
			stringStream << std::dec << "Queue " << j << ":" << std::endl;
			std::list<PacketQueueEntry>* queue = (*i)->getQueue();
			stringStream << "  Number of packets: " << queue->size() << std::endl;
			int32_t l = 1;
			for(std::list<PacketQueueEntry>::iterator k = queue->begin(); k != queue->end(); ++k)
			{
				stringStream << "  Packet " << l << " (Type: ";
				if(k->getType() == QueueEntryType::PACKET)
				{
					std::shared_ptr<InsteonPacket> packet = k->getPacket();
					stringStream << "Packet): " << (packet ? packet->hexString() : "Nullptr") << std::endl;
				}
				else if(k->getType() == QueueEntryType::MESSAGE)
				{
					stringStream << "Message): ";
					std::shared_ptr<InsteonMessage> message = k->getMessage();
					if(message)
					{
						stringStream << "Type: " << GD::bl->hf.getHexString(message->getMessageType(), 2) << " Subtype: " << GD::bl->hf.getHexString(message->getMessageSubtype(), 2);
						if(!message->getSubtypes()->empty())
						{
							stringStream << " Subtypes: ";
							for(std::vector<std::pair<uint32_t, int32_t>>::iterator m = message->getSubtypes()->begin(); m != message->getSubtypes()->end(); ++m)
							{
								stringStream << "Index " << m->first << ": " << GD::bl->hf.getHexString(m->second, 2) << "; ";
							}
						}
						stringStream << std::endl;
					}
					else
					{
						stringStream << "Nullptr" << std::endl;
					}
				}
				else
				{
					stringStream << "Unknown)" << std::endl;
				}
				l++;
			}
			j++;
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_queuesMutex.unlock();
}
}
