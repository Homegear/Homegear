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

#include "PacketQueue.h"
#include "InsteonMessage.h"
#include "PendingQueues.h"
#include "../Base/BaseLib.h"
#include "GD.h"

namespace Insteon
{
PacketQueue::PacketQueue()
{
	_queueType = PacketQueueType::EMPTY;
	_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	_physicalInterface = GD::defaultPhysicalInterface;
}

PacketQueue::PacketQueue(std::shared_ptr<BaseLib::Systems::IPhysicalInterface> physicalInterface)
{
	_queueType = PacketQueueType::EMPTY;
	_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if(physicalInterface) _physicalInterface = physicalInterface;
	else _physicalInterface = GD::defaultPhysicalInterface;
}

PacketQueue::PacketQueue(std::shared_ptr<BaseLib::Systems::IPhysicalInterface> physicalInterface, PacketQueueType queueType) : PacketQueue(physicalInterface)
{
	_queueType = queueType;
}

PacketQueue::~PacketQueue()
{
	try
	{
		dispose();
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
}

void PacketQueue::serialize(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(GD::bl);
		_queueMutex.lock();
		if(_queue.size() == 0)
		{
			_queueMutex.unlock();
			return;
		}
		encoder.encodeByte(encodedData, (int32_t)_queueType);
		encoder.encodeInteger(encodedData, _queue.size());
		for(std::list<PacketQueueEntry>::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			encoder.encodeByte(encodedData, (uint8_t)i->getType());
			encoder.encodeBoolean(encodedData, i->stealthy);
			encoder.encodeBoolean(encodedData, i->forceResend);
			if(!i->getPacket()) encoder.encodeBoolean(encodedData, false);
			else
			{
				encoder.encodeBoolean(encodedData, true);
				std::vector<char> packet = i->getPacket()->byteArray();
				encoder.encodeByte(encodedData, packet.size());
				encodedData.insert(encodedData.end(), packet.begin(), packet.end());
			}
			std::shared_ptr<InsteonMessage> message = i->getMessage();
			if(!message) encoder.encodeBoolean(encodedData, false);
			else
			{
				encoder.encodeBoolean(encodedData, true);
				encoder.encodeByte(encodedData, message->getDirection());
				encoder.encodeByte(encodedData, message->getMessageType());
				encoder.encodeByte(encodedData, message->getMessageSubtype());
				encoder.encodeByte(encodedData, (uint8_t)message->getMessageFlags());
				std::vector<std::pair<uint32_t, int32_t>>* subtypes = message->getSubtypes();
				encoder.encodeByte(encodedData, subtypes->size());
				for(std::vector<std::pair<uint32_t, int32_t>>::iterator j = subtypes->begin(); j != subtypes->end(); ++j)
				{
					encoder.encodeByte(encodedData, j->first);
					encoder.encodeByte(encodedData, j->second);
				}
			}
			encoder.encodeString(encodedData, parameterName);
			encoder.encodeInteger(encodedData, channel);
			std::string id = _physicalInterface->getID();
			encoder.encodeString(encodedData, id);
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
	_queueMutex.unlock();
}

void PacketQueue::unserialize(std::shared_ptr<std::vector<char>> serializedData, InsteonDevice* device, uint32_t position)
{
	try
	{
		BaseLib::BinaryDecoder decoder(GD::bl);
		_queueMutex.lock();
		_queueType = (PacketQueueType)decoder.decodeByte(*serializedData, position);
		uint32_t queueSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < queueSize; i++)
		{
			_queue.push_back(PacketQueueEntry());
			PacketQueueEntry* entry = &_queue.back();
			entry->setType((QueueEntryType)decoder.decodeByte(*serializedData, position));
			entry->stealthy = decoder.decodeBoolean(*serializedData, position);
			entry->forceResend = decoder.decodeBoolean(*serializedData, position);
			int32_t packetExists = decoder.decodeBoolean(*serializedData, position);
			if(packetExists)
			{
				std::vector<uint8_t> packetData;
				uint32_t dataSize = decoder.decodeByte(*serializedData, position);
				if(position + dataSize <= serializedData->size()) packetData.insert(packetData.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
				std::shared_ptr<InsteonPacket> packet(new InsteonPacket(packetData));
				entry->setPacket(packet, false);
			}
			int32_t messageExists = decoder.decodeBoolean(*serializedData, position);
			if(messageExists)
			{
				int32_t direction = decoder.decodeByte(*serializedData, position);
				int32_t messageType = decoder.decodeByte(*serializedData, position);
				int32_t messageSubtype = decoder.decodeByte(*serializedData, position);
				InsteonPacketFlags flags = (InsteonPacketFlags)decoder.decodeByte(*serializedData, position);
				uint32_t subtypeSize = decoder.decodeByte(*serializedData, position);
				std::vector<std::pair<uint32_t, int32_t>> subtypes;
				for(uint32_t j = 0; j < subtypeSize; j++)
				{
					subtypes.push_back(std::pair<uint32_t, int32_t>(decoder.decodeByte(*serializedData, position), decoder.decodeByte(*serializedData, position)));
				}
				entry->setMessage(device->getMessages()->find(direction, messageType, messageSubtype, flags, subtypes), false);
			}
			parameterName = decoder.decodeString(*serializedData, position);
			channel = decoder.decodeInteger(*serializedData, position);
			std::string physicalInterfaceID = decoder.decodeString(*serializedData, position);
			if(GD::physicalInterfaces.find(physicalInterfaceID) != GD::physicalInterfaces.end()) _physicalInterface = GD::physicalInterfaces.at(physicalInterfaceID);
			else _physicalInterface = GD::defaultPhysicalInterface;
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	clear();
    	_pendingQueues.reset();
    }
    if(!_physicalInterface) _physicalInterface = GD::defaultPhysicalInterface;
    _queueMutex.unlock();
}

void PacketQueue::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		_startResendThreadMutex.lock();
		if(_startResendThread.joinable()) _startResendThread.join();
		_startResendThreadMutex.unlock();
		_pushPendingQueueThreadMutex.lock();
		if(_pushPendingQueueThread.joinable()) _pushPendingQueueThread.join();
		_pushPendingQueueThreadMutex.unlock();
		_sendThreadMutex.lock();
        if(_sendThread.joinable()) _sendThread.join();
        _sendThreadMutex.unlock();
		stopResendThread();
		stopPopWaitThread();
		_queueMutex.lock();
		_queue.clear();
		_pendingQueues.reset();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_sendThreadMutex.unlock();
    	_pushPendingQueueThreadMutex.unlock();
    	_startResendThreadMutex.unlock();
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	_sendThreadMutex.unlock();
    	_pushPendingQueueThreadMutex.unlock();
    	_startResendThreadMutex.unlock();
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	_sendThreadMutex.unlock();
    	_pushPendingQueueThreadMutex.unlock();
    	_startResendThreadMutex.unlock();
    }
    _queueMutex.unlock();
}

bool PacketQueue::isEmpty()
{
	return _queue.empty() && (!_pendingQueues || _pendingQueues->empty());
}

bool PacketQueue::pendingQueuesEmpty()
{
	 return (!_pendingQueues || _pendingQueues->empty());
}

void PacketQueue::resend(uint32_t threadId)
{
	try
	{
		//Add ~100 milliseconds after popping, otherwise the first resend is 100 ms too early.
		int64_t timeSinceLastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - _lastPop;
		int32_t i = 0;
		std::chrono::milliseconds sleepingTime;
		uint32_t responseDelay = _physicalInterface->responseDelay();
		if(timeSinceLastPop < responseDelay)
		{
			sleepingTime = std::chrono::milliseconds((responseDelay - timeSinceLastPop) / 3);
			if(_resendCounter == 0)
			{
				while(!_stopResendThread && i < 3)
				{
					std::this_thread::sleep_for(sleepingTime);
					i++;
				}
			}
		}
		if(_stopResendThread) return;
		i = 0;
		keepAlive();
		sleepingTime = std::chrono::milliseconds(100);
		while(!_stopResendThread && i < ((signed)_resendSleepingTime / 100))
		{
			std::this_thread::sleep_for(sleepingTime);
			i++;
		}
		if(_stopResendThread) return;

		_queueMutex.lock();
		if(!_queue.empty() && !_stopResendThread)
		{
			if(_stopResendThread)
			{
				_queueMutex.unlock();
				return;
			}
			bool forceResend = _queue.front().forceResend;
			if(!noSending)
			{
				GD::out.printDebug("Sending from resend thread " + std::to_string(threadId) + " of queue " + std::to_string(id) + ".");
				std::shared_ptr<InsteonPacket> packet = _queue.front().getPacket();
				if(!packet) return;
				packet->setHopsLeft(3);
				packet->setHopsMax(3);
				if(_queue.front().getType() == QueueEntryType::MESSAGE)
				{
					GD::out.printDebug("Invoking outgoing message handler from queue.");
					InsteonMessage* message = _queue.front().getMessage().get();
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					if(_stopResendThread || _disposing)
					{
						_sendThreadMutex.unlock();
						return;
					}
					_sendThread = std::thread(&InsteonMessage::invokeMessageHandlerOutgoing, message, packet);
					BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
					_sendThreadMutex.unlock();
				}
				else
				{
					bool stealthy = _queue.front().stealthy;
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					if(_stopResendThread || _disposing)
					{
						_sendThreadMutex.unlock();
						return;
					}
					_sendThread = std::thread(&PacketQueue::send, this, packet, stealthy);
					BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
					_sendThreadMutex.unlock();
				}
			}
			else _queueMutex.unlock(); //Has to be unlocked before startResendThread
			if(_stopResendThread) return;
			if(_resendCounter < ((signed)retries - 2)) //This actually means that the message will be sent three times all together if there is no response
			{
				_resendCounter++;
				_startResendThreadMutex.lock();
				if(_disposing)
				{
					_startResendThreadMutex.unlock();
					return;
				}
				if(_startResendThread.joinable()) _startResendThread.join();
				_startResendThread = std::thread(&PacketQueue::startResendThread, this, forceResend);
				_startResendThreadMutex.unlock();
			}
			else _resendCounter = 0;
		}
		else _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
		_startResendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	_startResendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	_startResendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::push(std::shared_ptr<InsteonPacket> packet, bool stealthy, bool forceResend)
{
	try
	{
		if(_disposing) return;
		PacketQueueEntry entry;
		entry.setPacket(packet, true);
		entry.stealthy = stealthy;
		entry.forceResend = forceResend;
		_queueMutex.lock();
		if(!noSending && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
			_resendCounter = 0;
			if(!noSending)
			{
				_sendThreadMutex.lock();
				if(_disposing)
				{
					_sendThreadMutex.unlock();
					return;
				}
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&PacketQueue::send, this, entry.getPacket(), entry.stealthy);
				BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
				_sendThreadMutex.unlock();
				startResendThread(forceResend);
			}
		}
		else
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::push(std::shared_ptr<PendingQueues>& pendingQueues)
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		_pendingQueues = pendingQueues;
		if(_queue.empty())
		{
			 _queueMutex.unlock();
			pushPendingQueue(true);
		}
		else  _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		 _queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	 _queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	 _queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void PacketQueue::push(std::shared_ptr<PacketQueue> pendingQueue, bool popImmediately, bool clearPendingQueues)
{
	try
	{
		if(_disposing) return;
		if(!pendingQueue) return;
		_queueMutex.lock();
		if(!_pendingQueues) _pendingQueues.reset(new PendingQueues());
		if(clearPendingQueues) _pendingQueues->clear();
		_pendingQueues->push(pendingQueue);
		_queueMutex.unlock();
		pushPendingQueue(true);
		_queueMutex.lock();
		if(popImmediately)
		{
			if(!_pendingQueues->empty()) _pendingQueues->pop(pendingQueueID);
			_workingOnPendingQueue = false;
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
    _queueMutex.unlock();
}

void PacketQueue::push(std::shared_ptr<InsteonMessage> message, std::shared_ptr<InsteonPacket> packet, bool forceResend)
{
	try
	{
		if(_disposing) return;
		if(!message || !packet) return;
		if(message->getDirection() != DIRECTIONOUT) GD::out.printWarning("Warning: Wrong push method used. Packet is not necessary for incoming messages");
		PacketQueueEntry entry;
		entry.setMessage(message, true);
		entry.setPacket(packet, false);
		entry.forceResend = forceResend;
		_queueMutex.lock();
		if(!noSending && entry.getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
			_resendCounter = 0;
			if(!noSending)
			{
				_sendThreadMutex.lock();
				if(_disposing)
				{
					_sendThreadMutex.unlock();
					return;
				}
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&InsteonMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
				_sendThreadMutex.unlock();
				startResendThread(forceResend);
			}
		}
		else
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::push(std::shared_ptr<InsteonMessage> message, bool forceResend)
{
	try
	{
		if(_disposing) return;
		if(!message) return;
		if(message->getDirection() == DIRECTIONOUT) GD::out.printCritical("Critical: Wrong push method used. Please provide the received packet for outgoing messages");
		PacketQueueEntry entry;
		entry.setMessage(message, true);
		entry.forceResend = forceResend;
		_queueMutex.lock();
		if(!noSending && entry.getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
			_resendCounter = 0;
			if(!noSending)
			{
				_sendThreadMutex.lock();
				if(_disposing)
				{
					_sendThreadMutex.unlock();
					return;
				}
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&InsteonMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
				_sendThreadMutex.unlock();
				startResendThread(forceResend);
			}
		}
		else
		{
			_queue.push_back(entry);
			_queueMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::pushFront(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(_disposing) return;
		keepAlive();
		PacketQueueEntry entry;
		entry.setPacket(packet, true);

		_queueMutex.lock();
		_queue.push_front(entry);
		_queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::processCurrentQueueEntry(bool startResendOnly)
{
	try
	{
		nextQueueEntry(!startResendOnly);
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
}

void PacketQueue::stopPopWaitThread()
{
	try
	{
		_stopPopWaitThread = true;
		if(_popWaitThread.joinable()) _popWaitThread.join();
		_stopPopWaitThread = false;
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
}

void PacketQueue::popWait(uint32_t waitingTime)
{
	try
	{
		if(_disposing) return;
		stopResendThread();
		stopPopWaitThread();
		_popWaitThread = std::thread(&PacketQueue::popWaitThread, this, _popWaitThreadId++, waitingTime);
		BaseLib::Threads::setThreadPriority(GD::bl, _popWaitThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
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
}

void PacketQueue::popWaitThread(uint32_t threadId, uint32_t waitingTime)
{
	try
	{
		std::chrono::milliseconds sleepingTime(25);
		uint32_t i = 0;
		while(!_stopPopWaitThread && i < waitingTime)
		{
			std::this_thread::sleep_for(sleepingTime);
			i += 25;
		}
		if(!_stopPopWaitThread)
		{
			pop();
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
}

void PacketQueue::send(std::shared_ptr<InsteonPacket> packet, bool stealthy)
{
	try
	{
		if(noSending || _disposing) return;
		if(device) device->sendPacket(_physicalInterface, packet, stealthy);
		else GD::out.printError("Error: Device pointer of queue " + std::to_string(id) + " is null.");
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
}

void PacketQueue::stopResendThread()
{
	try
	{
		_stopResendThread = true;
		if(_resendThread.joinable()) _resendThread.join();
		_stopResendThread = false;
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
}

void PacketQueue::startResendThread(bool force)
{
	try
	{
		if(noSending || _disposing) return;
		_queueMutex.lock();
		if(_queue.empty())
		{
			_queueMutex.unlock();
			return;
		}
		int32_t destinationAddress = 0;
		if(_queue.front().getPacket())
		{
			destinationAddress = _queue.front().getPacket()->destinationAddress();
		}

		_queueMutex.unlock();
		if(destinationAddress != 0 || force) //Resend when no response?
		{
			stopResendThread();
			_resendThread = std::thread(&PacketQueue::resend, this, _resendThreadId++);
			BaseLib::Threads::setThreadPriority(GD::bl, _resendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::clear()
{
	try
	{
		stopResendThread();
		_queueMutex.lock();
		if(_pendingQueues) _pendingQueues->clear();
		_queue.clear();
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
    _queueMutex.unlock();
}

void PacketQueue::sleepAndPushPendingQueue()
{
	try
	{
		if(_disposing) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(_physicalInterface->responseDelay()));
		pushPendingQueue(true);
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
}

void PacketQueue::pushPendingQueue(bool sendImmediately)
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		if(_disposing)
		{
			_queueMutex.unlock();
			return;
		}
		if(!_pendingQueues || _pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		while(!_pendingQueues->empty() && (!_pendingQueues->front() || _pendingQueues->front()->isEmpty()))
		{
			GD::out.printDebug("Debug: Empty queue was pushed.");
			_pendingQueues->pop();
		}
		if(_pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		std::shared_ptr<PacketQueue> queue = _pendingQueues->front();
		_queueMutex.unlock();
		if(!queue) return; //Not really necessary, as the mutex is locked, but I had a segmentation fault in this function, so just to make
		_queueType = queue->getQueueType();
		retries = queue->retries;
		_resendSleepingTime = queue->getResendSleepingTime();
		pendingQueueID = queue->pendingQueueID;
		for(std::list<PacketQueueEntry>::iterator i = queue->getQueue()->begin(); i != queue->getQueue()->end(); ++i)
		{
			if(!noSending && i->getType() == QueueEntryType::MESSAGE && i->getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queueMutex.lock();
				_queue.push_back(*i);
				_queueMutex.unlock();
				_resendCounter = 0;
				if(!noSending)
				{
					if(sendImmediately)
					{
						_sendThreadMutex.lock();
						if(_disposing)
						{
							_sendThreadMutex.unlock();
							return;
						}
						if(_sendThread.joinable()) _sendThread.join();
						_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
						_sendThread = std::thread(&InsteonMessage::invokeMessageHandlerOutgoing, i->getMessage().get(), i->getPacket());
						_sendThreadMutex.unlock();
						BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
					}
					startResendThread(i->forceResend);
				}
			}
			else if(!noSending && i->getType() == QueueEntryType::PACKET && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queueMutex.lock();
				_queue.push_back(*i);
				_queueMutex.unlock();
				_resendCounter = 0;
				if(!noSending)
				{
					if(sendImmediately)
					{
						_sendThreadMutex.lock();
						if(_disposing)
						{
							_sendThreadMutex.unlock();
							return;
						}
						if(_sendThread.joinable()) _sendThread.join();
						_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
						_sendThread = std::thread(&PacketQueue::send, this, i->getPacket(), i->stealthy);
						_sendThreadMutex.unlock();
						BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
					}
					startResendThread(i->forceResend);
				}
			}
			else
			{
				_queueMutex.lock();
				_queue.push_back(*i);
				_queueMutex.unlock();
			}
		}
		_workingOnPendingQueue = true;
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::keepAlive()
{
	if(_disposing) return;
	if(lastAction) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void PacketQueue::longKeepAlive()
{
	if(_disposing) return;
	if(lastAction) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 15000;
}

void PacketQueue::nextQueueEntry(bool sendImmediately)
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		if(_queue.empty()) {
			if(_workingOnPendingQueue && !_pendingQueues->empty()) _pendingQueues->pop(pendingQueueID);
			if(!_pendingQueues || (_pendingQueues && _pendingQueues->empty()))
			{
				_stopResendThread = true;
				GD::out.printInfo("Info: Queue " + std::to_string(id) + " is empty and there are no pending queues.");
				_workingOnPendingQueue = false;
				_queueMutex.unlock();
				return;
			}
			else
			{
				_queueMutex.unlock();
				GD::out.printDebug("Queue " + std::to_string(id) + " is empty. Pushing pending queue...");
				_pushPendingQueueThreadMutex.lock();
				if(_disposing)
				{
					_pushPendingQueueThreadMutex.unlock();
					return;
				}
				if(_pushPendingQueueThread.joinable()) _pushPendingQueueThread.join();
				_pushPendingQueueThread = std::thread(&PacketQueue::pushPendingQueue, this, sendImmediately);
				BaseLib::Threads::setThreadPriority(GD::bl, _pushPendingQueueThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
				_pushPendingQueueThreadMutex.unlock();
				return;
			}
		}
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			_resendCounter = 0;
			if(!noSending)
			{
				bool forceResend = _queue.front().forceResend;
				if(sendImmediately)
				{
					std::shared_ptr<InsteonPacket> packet = _queue.front().getPacket();
					if(_queue.front().getType() == QueueEntryType::MESSAGE)
					{
						InsteonMessage* message = _queue.front().getMessage().get();
						_queueMutex.unlock();
						_sendThreadMutex.lock();
						if(_disposing)
						{
							_sendThreadMutex.unlock();
							return;
						}
						if(_sendThread.joinable()) _sendThread.join();
						_sendThread = std::thread(&InsteonMessage::invokeMessageHandlerOutgoing, message, packet);
						_sendThreadMutex.unlock();
					}
					else
					{
						bool stealthy = _queue.front().stealthy;
						_queueMutex.unlock();
						_sendThreadMutex.lock();
						if(_disposing)
						{
							_sendThreadMutex.unlock();
							return;
						}
						if(_sendThread.joinable()) _sendThread.join();
						_sendThread = std::thread(&PacketQueue::send, this, packet, stealthy);
						_sendThreadMutex.unlock();
					}
					BaseLib::Threads::setThreadPriority(GD::bl, _sendThread.native_handle(), GD::bl->settings.packetQueueThreadPriority(), GD::bl->settings.packetQueueThreadPolicy());
				}
				else _queueMutex.unlock();
				startResendThread(forceResend);
			}
			else _queueMutex.unlock();
		}
		else _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
		_pushPendingQueueThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	_pushPendingQueueThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	_pushPendingQueueThreadMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PacketQueue::pop(bool silently)
{
	try
	{
		if(_disposing) return;
		keepAlive();
		if(silently) GD::out.printDebug("Popping silently from queue: " + std::to_string(id));
		else GD::out.printDebug("Popping from queue: " + std::to_string(id));
		if(_popWaitThread.joinable()) _stopPopWaitThread = true;
		if(_resendThread.joinable()) _stopResendThread = true;
		_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		_queueMutex.lock();
		if(_queue.empty())
		{
			_queueMutex.unlock();
			return;
		}
		_queue.pop_front();
		if(GD::bl->debugLevel >= 5 && !_queue.empty())
		{
			if(_queue.front().getType() == QueueEntryType::PACKET && _queue.front().getPacket()) GD::out.printDebug("Packet now at front of queue: " + _queue.front().getPacket()->hexString());
			else if(_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()) GD::out.printDebug("Message now at front: Message type: 0x" + BaseLib::HelperFunctions::getHexString(_queue.front().getMessage()->getMessageType(), 2) + " Message subtype: 0x" + BaseLib::HelperFunctions::getHexString(_queue.front().getMessage()->getMessageSubtype(), 2) + " Message flags: 0x" + BaseLib::HelperFunctions::getHexString((int32_t)_queue.front().getMessage()->getMessageFlags()));
		}
		_queueMutex.unlock();
		if(!silently) nextQueueEntry(true);
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
}
