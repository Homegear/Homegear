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

#include "HMWiredQueue.h"
#include "../../GD/GD.h"
#include "HMWiredMessage.h"
#include "HMWiredMessages.h"
#include "PendingHMWiredQueues.h"

namespace HMWired
{
HMWiredQueue::HMWiredQueue() : _queueType(HMWiredQueueType::EMPTY)
{
	_lastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

HMWiredQueue::HMWiredQueue(HMWiredQueueType queueType) : HMWiredQueue()
{
	_queueType = queueType;
}

HMWiredQueue::~HMWiredQueue()
{
	try
	{
		dispose();
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::serialize(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		_queueMutex.lock();
		if(_queue.size() == 0)
		{
			_queueMutex.unlock();
			return;
		}
		encoder.encodeByte(encodedData, (int32_t)_queueType);
		encoder.encodeInteger(encodedData, _queue.size());
		for(std::list<HMWiredQueueEntry>::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			encoder.encodeByte(encodedData, (uint8_t)i->getType());
			encoder.encodeBoolean(encodedData, i->stealthy);
			encoder.encodeBoolean(encodedData, i->forceResend);
			if(!i->getPacket()) encoder.encodeBoolean(encodedData, false);
			else
			{
				encoder.encodeBoolean(encodedData, true);
				std::vector<uint8_t> packet = i->getPacket()->byteArray();
				encoder.encodeByte(encodedData, packet.size());
				encodedData.insert(encodedData.end(), packet.begin(), packet.end());
			}
			std::shared_ptr<HMWiredMessage> message = i->getMessage();
			if(!message)
			{
				encoder.encodeBoolean(encodedData, false);
				continue;
			}
			encoder.encodeBoolean(encodedData, true);
			encoder.encodeByte(encodedData, message->getDirection());
			encoder.encodeByte(encodedData, message->getMessageType());
			std::vector<std::pair<uint32_t, int32_t>>* subtypes = message->getSubtypes();
			encoder.encodeByte(encodedData, subtypes->size());
			for(std::vector<std::pair<uint32_t, int32_t>>::iterator j = subtypes->begin(); j != subtypes->end(); ++j)
			{
				encoder.encodeByte(encodedData, j->first);
				encoder.encodeByte(encodedData, j->second);
			}
		}
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_queueMutex.unlock();
}

void HMWiredQueue::unserialize(std::shared_ptr<std::vector<char>> serializedData, HMWiredDevice* device, uint32_t position)
{
	try
	{
		BinaryDecoder decoder;
		_queueMutex.lock();
		_queueType = (HMWiredQueueType)decoder.decodeByte(serializedData, position);
		uint32_t queueSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < queueSize; i++)
		{
			_queue.push_back(HMWiredQueueEntry());
			HMWiredQueueEntry* entry = &_queue.back();
			entry->setType((QueueEntryType)decoder.decodeByte(serializedData, position));
			entry->stealthy = decoder.decodeBoolean(serializedData, position);
			entry->forceResend = decoder.decodeBoolean(serializedData, position);
			int32_t packetExists = decoder.decodeBoolean(serializedData, position);
			if(packetExists)
			{
				std::vector<uint8_t> packetData;
				uint32_t dataSize = decoder.decodeByte(serializedData, position);
				if(position + dataSize <= serializedData->size()) packetData.insert(packetData.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
				std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(packetData, false));
				entry->setPacket(packet, false);
			}
			int32_t messageExists = decoder.decodeBoolean(serializedData, position);
			if(!messageExists) continue;
			int32_t direction = decoder.decodeByte(serializedData, position);
			int32_t messageType = decoder.decodeByte(serializedData, position);
			uint32_t subtypeSize = decoder.decodeByte(serializedData, position);
			std::vector<std::pair<uint32_t, int32_t>> subtypes;
			for(uint32_t j = 0; j < subtypeSize; j++)
			{
				subtypes.push_back(std::pair<uint32_t, int32_t>(decoder.decodeByte(serializedData, position), decoder.decodeByte(serializedData, position)));
			}
			entry->setMessage(device->getMessages()->find(direction, messageType, subtypes), false);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	clear();
    	_pendingQueues.reset();
    }
    _queueMutex.unlock();
}

void HMWiredQueue::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		if(_startResendThread.joinable()) _startResendThread.join();
		if(_pushPendingQueueThread.joinable()) _pushPendingQueueThread.join();
        if(_sendThread.joinable()) _sendThread.join();
		stopResendThread();
		stopPopWaitThread();
		_queueMutex.lock();
		_queue.clear();
		_pendingQueues.reset();
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queueMutex.unlock();
}

bool HMWiredQueue::isEmpty()
{
	return _queue.empty() && (!_pendingQueues || _pendingQueues->empty());
}

bool HMWiredQueue::pendingQueuesEmpty()
{
	 return (!_pendingQueues || _pendingQueues->empty());
}

void HMWiredQueue::resend(uint32_t threadId)
{
	try
	{
		//Add ~4 milliseconds after popping, otherwise the first resend is 4 ms too early.
		int64_t timeSinceLastPop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - _lastPop;
		int32_t i = 0;
		std::chrono::milliseconds sleepingTime;
		uint32_t responseDelay = GD::physicalDevices.get(DeviceFamilies::HomeMaticWired)->responseDelay();
		if(timeSinceLastPop < responseDelay && _resendCounter == 0) std::this_thread::sleep_for(std::chrono::milliseconds(responseDelay - timeSinceLastPop));
		if(_stopResendThread) return;

		//Sleep for 200 ms
		i = 0;
		keepAlive();
		sleepingTime = std::chrono::milliseconds(25);
		while(!_stopResendThread && i < 8)
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
				Output::printDebug("Sending from resend thread " + std::to_string(threadId) + " of queue " + std::to_string(id) + ".");
				std::shared_ptr<HMWiredPacket> packet = _queue.front().getPacket();
				if(!packet) return;
				if(_queue.front().getType() == QueueEntryType::MESSAGE)
				{
					Output::printDebug("Invoking outgoing message handler from HomeMatic Wired queue.");
					HMWiredMessage* message = _queue.front().getMessage().get();
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					if(_stopResendThread)
					{
						_sendThreadMutex.unlock();
						return;
					}
					_sendThread = std::thread(&HMWiredMessage::invokeMessageHandlerOutgoing, message, packet);
					Threads::setThreadPriority(_sendThread.native_handle(), 45);
					_sendThreadMutex.unlock();
				}
				else
				{
					bool stealthy = _queue.front().stealthy;
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					if(_stopResendThread)
					{
						_sendThreadMutex.unlock();
						return;
					}
					_sendThread = std::thread(&HMWiredQueue::send, this, packet, stealthy);
					Threads::setThreadPriority(_sendThread.native_handle(), 45);
					_sendThreadMutex.unlock();
				}
			}
			else _queueMutex.unlock(); //Has to be unlocked before startResendThread
			if(_stopResendThread) return;
			if(_resendCounter < (retries - 2)) //This actually means that the message will be sent three times all together if there is no response
			{
				_resendCounter++;
				if(_startResendThread.joinable()) _startResendThread.join();
				_startResendThread = std::thread(&HMWiredQueue::startResendThread, this, forceResend);
			}
			else _resendCounter = 0;
		}
		else _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::push(std::shared_ptr<HMWiredPacket> packet, bool stealthy, bool forceResend)
{
	try
	{
		HMWiredQueueEntry entry;
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
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&HMWiredQueue::send, this, entry.getPacket(), entry.stealthy);
				Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::push(std::shared_ptr<PendingHMWiredQueues>& pendingQueues)
{
	try
	{
		_queueMutex.lock();
		_pendingQueues = pendingQueues;
		if(_queue.empty())
		{
			 _queueMutex.unlock();
			pushPendingQueue();
		}
		else  _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		 _queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	 _queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	 _queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void HMWiredQueue::push(std::shared_ptr<HMWiredQueue> pendingQueue, bool popImmediately, bool clearPendingQueues)
{
	try
	{
		if(!pendingQueue) return;
		_queueMutex.lock();
		if(!_pendingQueues) _pendingQueues.reset(new PendingHMWiredQueues());
		if(clearPendingQueues) _pendingQueues->clear();
		_pendingQueues->push(pendingQueue);
		_queueMutex.unlock();
		pushPendingQueue();
		_queueMutex.lock();
		if(popImmediately)
		{
			_pendingQueues->pop();
			_workingOnPendingQueue = false;
		}
		_queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::push(std::shared_ptr<HMWiredMessage> message, std::shared_ptr<HMWiredPacket> packet, bool forceResend)
{
	try
	{
		if(message->getDirection() != DIRECTIONOUT) Output::printWarning("Warning: Wrong push method used. Packet is not necessary for incoming messages");
		if(message == nullptr) return;
		HMWiredQueueEntry entry;
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
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&HMWiredMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::push(std::shared_ptr<HMWiredMessage> message, bool forceResend)
{
	try
	{
		if(message->getDirection() == DIRECTIONOUT) Output::printCritical("Critical: Wrong push method used. Please provide the received packet for outgoing messages");
		if(message == nullptr) return;
		HMWiredQueueEntry entry;
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
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&HMWiredMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::pushFront(std::shared_ptr<HMWiredPacket> packet, bool stealthy, bool popBeforePushing, bool forceResend)
{
	try
	{
		keepAlive();
		if(popBeforePushing)
		{
			Output::printDebug("Popping from HomeMatic Wired queue and pushing packet at the front: " + std::to_string(id));
			if(_popWaitThread.joinable()) _stopPopWaitThread = true;
			if(_resendThread.joinable()) _stopResendThread = true;
			_queueMutex.lock();
			_queue.pop_front();
			_queueMutex.unlock();
		}
		HMWiredQueueEntry entry;
		entry.setPacket(packet, true);
		entry.stealthy = stealthy;
		entry.forceResend = forceResend;
		if(!noSending)
		{
			_queueMutex.lock();
			_queue.push_front(entry);
			_queueMutex.unlock();
			_resendCounter = 0;
			if(!noSending)
			{
				_sendThreadMutex.lock();
				if(_sendThread.joinable()) _sendThread.join();
				_sendThread = std::thread(&HMWiredQueue::send, this, entry.getPacket(), entry.stealthy);
				Threads::setThreadPriority(_sendThread.native_handle(), 45);
				_sendThreadMutex.unlock();
				startResendThread(forceResend);
			}
		}
		else
		{
			_queueMutex.lock();
			_queue.push_front(entry);
			_queueMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
		_sendThreadMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::stopPopWaitThread()
{
	try
	{
		_stopPopWaitThread = true;
		if(_popWaitThread.joinable()) _popWaitThread.join();
		_stopPopWaitThread = false;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::popWait(uint32_t waitingTime)
{
	try
	{
		stopResendThread();
		stopPopWaitThread();
		_popWaitThread = std::thread(&HMWiredQueue::popWaitThread, this, _popWaitThreadId++, waitingTime);
		Threads::setThreadPriority(_popWaitThread.native_handle(), 45);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::popWaitThread(uint32_t threadId, uint32_t waitingTime)
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::send(std::shared_ptr<HMWiredPacket> packet, bool stealthy)
{
	try
	{
		if(noSending || _disposing) return;
		if(device) device->sendPacket(packet, stealthy);
		else Output::printError("Error: Device pointer of queue " + std::to_string(id) + " is null.");
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::stopResendThread()
{
	try
	{
		_stopResendThread = true;
		if(_resendThread.joinable()) _resendThread.join();
		_stopResendThread = false;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::startResendThread(bool force)
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
		uint8_t controlByte = 0;
		if(_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage())
		{
			controlByte = _queue.front().getMessage()->getControlByte();
		}
		else if(_queue.front().getPacket())
		{
			controlByte = _queue.front().getPacket()->controlByte();
		}
		else throw Exception("Packet or message pointer of HomeMatic Wired queue is empty.");

		_queueMutex.unlock();
		if((controlByte & 1) || force) //Resend when no response?
		{
			stopResendThread();
			_resendThread = std::thread(&HMWiredQueue::resend, this, _resendThreadId++);
			Threads::setThreadPriority(_resendThread.native_handle(), 45);
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::clear()
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _queueMutex.unlock();
}

void HMWiredQueue::sleepAndPushPendingQueue()
{
	try
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(GD::physicalDevices.get(DeviceFamilies::HomeMaticWired)->responseDelay()));
		pushPendingQueue();
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::pushPendingQueue()
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		if(!_pendingQueues || _pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		while(!_pendingQueues->empty() && (!_pendingQueues->front() || _pendingQueues->front()->isEmpty()))
		{
			Output::printDebug("Debug: Empty queue was pushed.");
			_pendingQueues->pop();
		}
		if(_pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		std::shared_ptr<HMWiredQueue> queue = _pendingQueues->front();
		_queueMutex.unlock();
		if(!queue) return; //Not really necessary, as the mutex is locked, but I had a segmentation fault in this function, so just to make
		_queueType = queue->getQueueType();
		queueEmptyCallback = queue->queueEmptyCallback;
		callbackParameter = queue->callbackParameter;
		retries = queue->retries;
		for(std::list<HMWiredQueueEntry>::iterator i = queue->getQueue()->begin(); i != queue->getQueue()->end(); ++i)
		{
			if(!noSending && i->getType() == QueueEntryType::MESSAGE && i->getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queueMutex.lock();
				_queue.push_back(*i);
				_queueMutex.unlock();
				_resendCounter = 0;
				if(!noSending)
				{
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					_sendThread = std::thread(&HMWiredMessage::invokeMessageHandlerOutgoing, i->getMessage().get(), i->getPacket());
					_sendThreadMutex.unlock();
					Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					_sendThread = std::thread(&HMWiredQueue::send, this, i->getPacket(), i->stealthy);
					_sendThreadMutex.unlock();
					Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::keepAlive()
{
	if(_disposing) return;
	if(lastAction) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void HMWiredQueue::longKeepAlive()
{
	if(_disposing) return;
	if(lastAction) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 5000;
}

void HMWiredQueue::nextQueueEntry()
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		if(_queue.empty()) {
			if(queueEmptyCallback && callbackParameter) queueEmptyCallback(callbackParameter);
			if(_workingOnPendingQueue) _pendingQueues->pop();
			if(!_pendingQueues || (_pendingQueues && _pendingQueues->empty()))
			{
				_stopResendThread = true;
				Output::printInfo("Info: Queue " + std::to_string(id) + " is empty and there are no pending queues.");
				_workingOnPendingQueue = false;
				_queueMutex.unlock();
				return;
			}
			else
			{
				_queueMutex.unlock();
				Output::printDebug("Queue " + std::to_string(id) + " is empty. Pushing pending queue...");
				if(_pushPendingQueueThread.joinable()) _pushPendingQueueThread.join();
				_pushPendingQueueThread = std::thread(&HMWiredQueue::pushPendingQueue, this);
				Threads::setThreadPriority(_pushPendingQueueThread.native_handle(), 45);
				return;
			}
		}
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			_resendCounter = 0;
			if(!noSending)
			{
				bool forceResend = _queue.front().forceResend;
				std::shared_ptr<HMWiredPacket> packet = _queue.front().getPacket();
				if(_queue.front().getType() == QueueEntryType::MESSAGE)
				{
					HMWiredMessage* message = _queue.front().getMessage().get();
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					_sendThread = std::thread(&HMWiredMessage::invokeMessageHandlerOutgoing, message, packet);
					_sendThreadMutex.unlock();
				}
				else
				{
					bool stealthy = _queue.front().stealthy;
					_queueMutex.unlock();
					_sendThreadMutex.lock();
					if(_sendThread.joinable()) _sendThread.join();
					_sendThread = std::thread(&HMWiredQueue::send, this, packet, stealthy);
					_sendThreadMutex.unlock();
				}
				Threads::setThreadPriority(_sendThread.native_handle(), 45);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	_sendThreadMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredQueue::pop()
{
	try
	{
		if(_disposing) return;
		keepAlive();
		Output::printDebug("Popping from HomeMatic Wired queue: " + std::to_string(id));
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
		if(GD::debugLevel >= 5 && !_queue.empty())
		{
			if(_queue.front().getType() == QueueEntryType::PACKET && _queue.front().getPacket()) Output::printDebug("Packet now at front of queue: " + _queue.front().getPacket()->hexString());
			else if(_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()) Output::printDebug("Message now at front: Message type: 0x" + HelperFunctions::getHexString(_queue.front().getMessage()->getMessageType()) + " Control byte: 0x" + HelperFunctions::getHexString(_queue.front().getMessage()->getControlByte()));
		}
		_queueMutex.unlock();
		nextQueueEntry();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
}
