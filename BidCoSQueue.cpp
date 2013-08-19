#include "BidCoSQueue.h"
#include "GD.h"
#include "HelperFunctions.h"

BidCoSQueue::BidCoSQueue() : _queueType(BidCoSQueueType::EMPTY)
{
}

BidCoSQueue::BidCoSQueue(BidCoSQueueType queueType) : BidCoSQueue()
{
	_queueType = queueType;
}

BidCoSQueue::BidCoSQueue(std::string serializedObject, HomeMaticDevice* device) : BidCoSQueue()
{
	try
	{
		HelperFunctions::printDebug("Unserializing queue: " + serializedObject);
		uint32_t pos = 0;
		_queueType = (BidCoSQueueType)std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		uint32_t queueSize = std::stol(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		for(uint32_t i = 0; i < queueSize; i++)
		{
			_queueMutex.lock();
			_queue.push_back(BidCoSQueueEntry());
			BidCoSQueueEntry* entry = &_queue.back();
			_queueMutex.unlock();
			entry->setType((QueueEntryType)std::stoi(serializedObject.substr(pos, 1), 0, 16)); pos += 1;
			entry->stealthy = std::stoi(serializedObject.substr(pos, 1)); pos += 1;
			entry->forceResend = std::stoi(serializedObject.substr(pos, 1)); pos += 1;
			int32_t packetExists = std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			if(packetExists)
			{
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				uint32_t packetLength = std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
				std::string packetString(serializedObject.substr(pos, packetLength));
				packet->import(packetString, false); pos += packetLength;
				entry->setPacket(packet, false);
			}
			int32_t messageExists = std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			if(!messageExists) continue;
			int32_t direction = std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			int32_t messageType = std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
			uint32_t subtypeSize = std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
			std::vector<std::pair<uint32_t, int32_t>> subtypes;
			for(uint32_t j = 0; j < subtypeSize; j++)
			{
				subtypes.push_back(std::pair<uint32_t, int32_t>(std::stoi(serializedObject.substr(pos, 2), 0, 16), std::stoi(serializedObject.substr(pos + 2, 2), 0, 16))); pos += 4;
			}
			entry->setMessage(device->getMessages()->find(direction, messageType, subtypes), false);
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	clear();
    	_pendingQueues.reset();
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	clear();
    	_pendingQueues.reset();
    }
}

BidCoSQueue::~BidCoSQueue()
{
	try
	{
		stopResendThread();
		stopPopWaitThread();
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
}

void BidCoSQueue::resend(uint32_t threadId, bool burst)
{
	try
	{
		//Add 100 milliseconds after pushing the packet, otherwise for responses the first resend is 100 ms too early. If the queue is not generated as a response, the resend is 90 ms too late. But so what?
		//Packets that are not sent in response are 110 ms too late. But that doesn't matter.
		int32_t i = 0;
		std::chrono::milliseconds sleepingTime(36);
		if(_resendCounter == 0)
		{
			while(!_stopResendThread && i < 3)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}
		if(_resendCounter < 2)
		{
			//Sleep for 175 ms
			i = 0;
			if(burst)
			{
				longKeepAlive();
				sleepingTime = std::chrono::milliseconds(100);
			}
			else sleepingTime = std::chrono::milliseconds(25);
			while(!_stopResendThread && i < 7)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}
		else if(_resendCounter >= 2 && _resendCounter < 6)
		{
			longKeepAlive();
			//Sleep for 1000 ms
			i = 0;
			sleepingTime = std::chrono::milliseconds(50);
			while(!_stopResendThread && i < 20)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}
		else
		{
			longKeepAlive();
			//Sleep for 5000 ms
			i = 0;
			sleepingTime = std::chrono::milliseconds(200);
			while(!_stopResendThread && i < 25)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}

		if(!_stopResendThread)
		{
			_queueMutex.lock();
			if(!_queue.empty() && !_stopResendThread)
			{
				std::thread send;
				if(!noSending)
				{
					HelperFunctions::printDebug("Sending from resend thread " + std::to_string(threadId) + " of queue " + std::to_string(id) + ".");
					if(_queue.front().getType() == QueueEntryType::MESSAGE)
					{
						HelperFunctions::printDebug("Invoking outgoing message handler from BidCoSQueue.");
						send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
					}
					else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket(), _queue.front().stealthy);
					send.detach();
				}
				_queueMutex.unlock(); //Has to be unlocked before startResendThread
				if(_stopResendThread) return;
				if(_resendCounter < (retries - 2)) //This actually means that the message will be sent three times all together if there is no response
				{
					_resendCounter++;
					_queueMutex.lock();
					send = std::thread(&BidCoSQueue::startResendThread, this, _queue.front().forceResend);
					send.detach();
					_queueMutex.unlock();
				}
				else _resendCounter = 0;
			}
			else _queueMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSPacket> packet, bool stealthy, bool forceResend)
{
	try
	{
		BidCoSQueueEntry entry;
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
				std::thread send(&BidCoSQueue::send, this, entry.getPacket(), entry.stealthy);
				send.detach();
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::push(std::shared_ptr<PendingBidCoSQueues>& pendingQueues)
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	 _queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	 _queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void BidCoSQueue::push(std::shared_ptr<BidCoSQueue> pendingQueue, bool popImmediately, bool clearPendingQueues)
{
	try
	{
		if(!pendingQueue) return;
		_queueMutex.lock();
		if(!_pendingQueues) _pendingQueues.reset(new PendingBidCoSQueues());
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSMessage> message, std::shared_ptr<BidCoSPacket> packet, bool forceResend)
{
	try
	{
		if(message->getDirection() != DIRECTIONOUT) HelperFunctions::printWarning("Warning: Wrong push method used. Packet is not necessary for incoming messages");
		if(message == nullptr) return;
		BidCoSQueueEntry entry;
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
				std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				send.detach();
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSMessage> message, bool forceResend)
{
	try
	{
		if(message->getDirection() == DIRECTIONOUT) HelperFunctions::printCritical("Critical: Wrong push method used. Please provide the received packet for outgoing messages");
		if(message == nullptr) return;
		BidCoSQueueEntry entry;
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
				std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				send.detach();
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::pushFront(std::shared_ptr<BidCoSPacket> packet, bool stealthy, bool popBeforePushing, bool forceResend)
{
	try
	{
		keepAlive();
		if(popBeforePushing)
		{
			HelperFunctions::printDebug("Popping from BidCoSQueue and pushing packet at the front: " + std::to_string(id));
			if(_popWaitThread && _popWaitThread->joinable()) _stopPopWaitThread = true;
			if(_resendThread && _resendThread->joinable()) _stopResendThread = true;
			_queueMutex.lock();
			_queue.pop_front();
			_queueMutex.unlock();
		}
		BidCoSQueueEntry entry;
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
				std::thread send(&BidCoSQueue::send, this, entry.getPacket(), entry.stealthy);
				send.detach();
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
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::stopPopWaitThread()
{
	try
	{
		if(!_popWaitThread) return;
		_stopPopWaitThread = true;
		if(_popWaitThread && _popWaitThread->joinable())
		{
			_popWaitThread->join();
			_popWaitThread.reset();
		}
		_stopPopWaitThread = false;
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
}

void BidCoSQueue::popWait(uint32_t waitingTime)
{
	try
	{
		stopResendThread();
		stopPopWaitThread();
		_popWaitThread.reset(new std::thread(&BidCoSQueue::popWaitThread, this, _popWaitThreadId++, waitingTime));
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
}

void BidCoSQueue::popWaitThread(uint32_t threadId, uint32_t waitingTime)
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
}

void BidCoSQueue::send(std::shared_ptr<BidCoSPacket> packet, bool stealthy)
{
	try
	{
		if(noSending) return;
		if(device) device->sendPacket(packet, stealthy);
		else HelperFunctions::printError("Error: Device pointer of queue " + std::to_string(id) + " is null.");
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
}

void BidCoSQueue::stopResendThread()
{
	try
	{
		if(!_resendThread) return;
		_stopResendThread = true;
		if(_resendThread && _resendThread->joinable())
		{
			_resendThread->join();
			_resendThread.reset();
		}
		_stopResendThread = false;
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
}

void BidCoSQueue::startResendThread(bool force)
{
	try
	{
		if(noSending) return;
		_queueMutex.lock();
		uint8_t controlByte = 0;
		if(_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage())
		{
			controlByte = _queue.front().getMessage()->getControlByte();
		}
		else if(_queue.front().getPacket())
		{
			controlByte = _queue.front().getPacket()->controlByte();
		}
		else throw Exception("Packet or message pointer is empty.");

		_queueMutex.unlock();
		if((!(controlByte & 0x02) && (controlByte & 0x20)) || force) //Resend when no response?
		{
			stopResendThread();
			bool burst = controlByte & 0x10;
			_resendThread.reset(new std::thread(&BidCoSQueue::resend, this, _resendThreadId++, burst));
		}
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::clear()
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
    _queueMutex.unlock();
}

void BidCoSQueue::sleepAndPushPendingQueue()
{
	try
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pushPendingQueue();
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
}

void BidCoSQueue::pushPendingQueue()
{
	try
	{
		_queueMutex.lock();
		if(!_pendingQueues || _pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		while(!_pendingQueues->empty() && _pendingQueues->front()->isEmpty())
		{
			HelperFunctions::printDebug("Debug: Empty queue was pushed.");
			_pendingQueues->pop();
		}
		if(_pendingQueues->empty())
		{
			_queueMutex.unlock();
			return;
		}
		std::shared_ptr<BidCoSQueue> queue;
		queue = _pendingQueues->front();
		_queueMutex.unlock();
		_queueType = queue->getQueueType();
		serviceMessages = queue->serviceMessages;
		queueEmptyCallback = queue->queueEmptyCallback;
		callbackParameter = queue->callbackParameter;
		retries = queue->retries;
		for(std::deque<BidCoSQueueEntry>::iterator i = queue->getQueue()->begin(); i != queue->getQueue()->end(); ++i)
		{
			if(!noSending && i->getType() == QueueEntryType::MESSAGE && i->getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queueMutex.lock();
				_queue.push_back(*i);
				_queueMutex.unlock();
				_resendCounter = 0;
				if(!noSending)
				{
					std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, i->getMessage().get(), i->getPacket());
					send.detach();
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
					std::thread send(&BidCoSQueue::send, this, i->getPacket(), i->stealthy);
					send.detach();
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::keepAlive()
{
	if(lastAction != nullptr) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BidCoSQueue::longKeepAlive()
{
	if(lastAction != nullptr) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 5000;
}

void BidCoSQueue::nextQueueEntry()
{
	try
	{
		_queueMutex.lock();
		if(_queue.empty()) {
			if(queueEmptyCallback && callbackParameter) queueEmptyCallback(callbackParameter);
			if(_workingOnPendingQueue) _pendingQueues->pop();
			if(!_pendingQueues || (_pendingQueues && _pendingQueues->empty()))
			{
				_stopResendThread = true;
				HelperFunctions::printInfo("Info: Queue " + std::to_string(id) + " is empty and there are no pending queues.");
				_workingOnPendingQueue = false;
				_queueMutex.unlock();
				if(serviceMessages && (_queueType == BidCoSQueueType::UNPAIRING || _queueType == BidCoSQueueType::CONFIG))
				{
					serviceMessages->setConfigPending(false);
					serviceMessages.reset();
				}
				return;
			}
			else
			{
				HelperFunctions::printDebug("Queue " + std::to_string(id) + " is empty. Pushing pending queue...");
				_queueMutex.unlock();
				std::thread push(&BidCoSQueue::pushPendingQueue, this);
				push.detach();
				return;
			}
		}
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			_resendCounter = 0;
			std::thread send;
			if(!noSending)
			{
				if(_queue.front().getType() == QueueEntryType::MESSAGE)
				{
					send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
				}
				else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket(), _queue.front().stealthy);
				send.detach();
				bool forceResend = _queue.front().forceResend;
				_queueMutex.unlock(); //Has to be unlocked before startResendThread()
				startResendThread(forceResend);
			}
			else _queueMutex.unlock();
		}
		else _queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSQueue::pop()
{
	try
	{
		keepAlive();
		HelperFunctions::printDebug("Popping from BidCoSQueue: " + std::to_string(id));
		if(_popWaitThread && _popWaitThread->joinable()) _stopPopWaitThread = true;
		if(_resendThread && _resendThread->joinable()) _stopResendThread = true;
		if(_queue.empty()) return;
		_queueMutex.lock();
		_queue.pop_front();
		if(GD::debugLevel >= 5 && !_queue.empty())
		{
			if(_queue.front().getType() == QueueEntryType::PACKET && _queue.front().getPacket()) HelperFunctions::printDebug("Packet now at front of queue: " + _queue.front().getPacket()->hexString());
			else if(_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()) HelperFunctions::printDebug("Message now at front: Message type: 0x" + HelperFunctions::getHexString(_queue.front().getMessage()->getMessageType()) + " Control byte: 0x" + HelperFunctions::getHexString(_queue.front().getMessage()->getControlByte()));
		}
		_queueMutex.unlock();
		nextQueueEntry();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string BidCoSQueue::serialize()
{
	try
	{
		_queueMutex.lock();
		if(_queue.size() == 0)
		{
			_queueMutex.unlock();
			return "";
		}
		std::ostringstream stringstream;
		stringstream << std::hex << std::uppercase << std::setfill('0');
		stringstream << std::setw(2) << (uint32_t)_queueType;
		stringstream << std::setw(4) << _queue.size();
		for(std::deque<BidCoSQueueEntry>::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			stringstream << std::setw(1) << (int32_t)i->getType();
			stringstream << std::setw(1) << (int32_t)i->stealthy;
			stringstream << std::setw(1) << (int32_t)i->forceResend;
			if(i->getPacket() == nullptr)
			{
				stringstream << '0';
			}
			else
			{
				stringstream << '1';
				std::string packetHex = i->getPacket()->hexString();
				stringstream << std::setw(2) << packetHex.size();
				stringstream << packetHex;
			}
			std::shared_ptr<BidCoSMessage> message = i->getMessage();
			if(message == nullptr)
			{
				stringstream << '0';
				continue;
			}
			stringstream << '1';
			stringstream << std::setw(1) << (int32_t)message->getDirection();
			stringstream << std::setw(2) << (int32_t)message->getMessageType();
			std::vector<std::pair<uint32_t, int32_t>>* subtypes = message->getSubtypes();
			stringstream << std::setw(2) << subtypes->size();
			for(std::vector<std::pair<uint32_t, int32_t>>::iterator j = subtypes->begin(); j != subtypes->end(); ++j)
			{
				stringstream << std::setw(2) << j->first;
				stringstream << std::setw(2) << j->second;
			}
		}
		stringstream << std::dec;
		_queueMutex.unlock();
		return stringstream.str();
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
    _queueMutex.unlock();
    return "";
}
