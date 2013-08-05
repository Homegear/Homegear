#include "BidCoSQueue.h"
#include "GD.h"

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
		if(GD::debugLevel >= 5) std::cout << "Unserializing queue: " << serializedObject << std::endl;
		uint32_t pos = 0;
		_queueType = (BidCoSQueueType)std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		uint32_t queueSize = std::stol(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		for(uint32_t i = 0; i < queueSize; i++)
		{
			_queue.push_back(BidCoSQueueEntry());
			BidCoSQueueEntry* entry = &_queue.back();
			entry->setType((QueueEntryType)std::stoi(serializedObject.substr(pos, 1), 0, 16)); pos += 1;
			int32_t packetExists = std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			if(packetExists)
			{
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				uint32_t packetLength = std::stoi(serializedObject.substr(pos, 2), 0, 16); pos += 2;
				packet->import(serializedObject.substr(pos, packetLength), false); pos += packetLength;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    	clear();
    	_pendingQueues.reset();
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    	clear();
    	_pendingQueues.reset();
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    	clear();
    	_pendingQueues.reset();
    }
}

BidCoSQueue::~BidCoSQueue()
{
	try
	{
		_stopResendThread = true;
		if(_resendThread && _resendThread->joinable()) _resendThread->join();
		stopPopWaitThread();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::resend(uint32_t threadId)
{
	try
	{
		//Add 90 milliseconds after pushing the packet, otherwise for responses the first resend is 90 ms too early. If the queue is not generated as a response, the resend is 90 ms too late. But so what?
		int32_t i = 0;
		std::chrono::milliseconds sleepingTime(30);
		if(resendCounter == 0)
		{
			while(!_stopResendThread && i < 3)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}
		if(resendCounter < 2)
		{
			//Sleep for 175 ms
			i = 0;
			sleepingTime = std::chrono::milliseconds(25);
			while(!_stopResendThread && i < 7)
			{
				std::this_thread::sleep_for(sleepingTime);
				i++;
			}
		}
		//only when burst = true
		else if(resendCounter >= 2 && resendCounter < 6)
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
		//only when burst = true
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
					if(GD::debugLevel >= 5) std::cout << "Sending from resend thread " << threadId << " of queue " << id << "." << std::endl;
					if(_queue.front().getType() == QueueEntryType::MESSAGE)
					{
						if(GD::debugLevel >= 5) std::cout << "Invoking outgoing message handler from BidCoSQueue." << std::endl;
						send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
					}
					else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket());
					send.detach();
				}
				_queueMutex.unlock(); //Has to be unlocked before startResendThread
				if(_stopResendThread) return;
				if(resendCounter < 1) //This actually means that the message will be sent three times all together if there is no response
				{
					resendCounter++;
					send = std::thread(&BidCoSQueue::startResendThread, this);
					send.detach();
				}
				else if(burst && resendCounter < 10)
				{
					resendCounter++;
					send = std::thread(&BidCoSQueue::startResendThread, this);
					send.detach();
				}
				else resendCounter = 0;
			}
			else _queueMutex.unlock();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		BidCoSQueueEntry entry;
		entry.setPacket(packet, true);
		if(!noSending && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			resendCounter = 0;
			if(!noSending)
			{
				std::thread send(&BidCoSQueue::send, this, entry.getPacket());
				send.detach();
				startResendThread();
			}
		}
		else
		{
			_queue.push_back(entry);
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push(std::shared_ptr<PendingBidCoSQueues>& pendingQueues)
{
	try
	{
		_pendingQueues = pendingQueues;
		if(_queue.empty()) pushPendingQueue();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSQueue> pendingQueue, bool popImmediately, bool clearPendingQueues)
{
	try
	{
		if(!_pendingQueues) _pendingQueues.reset(new PendingBidCoSQueues());
		if(clearPendingQueues) _pendingQueues->clear();
		if(!pendingQueue) return;
		_pendingQueues->push(pendingQueue);
		pushPendingQueue();
		if(popImmediately)
		{
			_pendingQueues->pop();
			_workingOnPendingQueue = false;
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSMessage> message, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(message->getDirection() != DIRECTIONOUT && GD::debugLevel >= 3) std::cout << "Warning: Wrong push method used. Packet is not necessary for incoming messages" << std::endl;
		if(message == nullptr) return;
		BidCoSQueueEntry entry;
		entry.setMessage(message, true);
		entry.setPacket(packet, false);
		if(!noSending && entry.getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			resendCounter = 0;
			if(!noSending)
			{
				std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				send.detach();
				startResendThread();
			}
		}
		else
		{
			_queue.push_back(entry);
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push(std::shared_ptr<BidCoSMessage> message)
{
	try
	{
		if(message->getDirection() == DIRECTIONOUT && GD::debugLevel >= 1) std::cout << "Critical: Wrong push method used. Please provide the received packet for outgoing messages" << std::endl;
		if(message == nullptr) return;
		BidCoSQueueEntry entry;
		entry.setMessage(message, true);
		if(!noSending && entry.getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			resendCounter = 0;
			if(!noSending)
			{
				std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, message.get(), entry.getPacket());
				send.detach();
				startResendThread();
			}
		}
		else
		{
			_queue.push_back(entry);
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::push_front(std::shared_ptr<BidCoSPacket> packet)
{
        try
        {
                BidCoSQueueEntry entry;
                entry.setPacket(packet, true);
                if(!noSending)
                {
                        _queue.push_front(entry);
                        resendCounter = 0;
                        if(!noSending)
                        {
                                std::thread send(&BidCoSQueue::send, this, entry.getPacket());
                                send.detach();
                                startResendThread();
                        }
                }
                else
                {
                        _queue.push_front(entry);
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
        std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::stopPopWaitThread()
{
	try
	{
		if(!_popWaitThread) return;
		_stopPopWaitThread = true;
		if(_popWaitThread && _popWaitThread->joinable()) _popWaitThread->join();
		_stopPopWaitThread = false;
		_popWaitThread.reset();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::send(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(noSending) return;
		if(device != nullptr) device->sendPacket(packet);
		else if(GD::debugLevel >= 2) std::cout << "Error: Device pointer of queue " << id << " is null." << std::endl;
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::stopResendThread()
{
	try
	{
		if(!_resendThread) return;
		_stopResendThread = true;
		if(_resendThread && _resendThread->joinable()) _resendThread->join();
		_stopResendThread = false;
		_resendThread.reset();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::startResendThread()
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
		else
		{
			if(GD::debugLevel >= 2) std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << "Packet or message pointer is empty." << std::endl;
		}
		_queueMutex.unlock();
		if(!(controlByte & 0x02) && (controlByte & 0x20)) //Resend when no response?
		{
			stopResendThread();
			_resendThread.reset(new std::thread(&BidCoSQueue::resend, this, _resendThreadId++));
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::clear()
{
	try
	{
		std::deque<BidCoSQueueEntry> emptyQueue;
		std::swap(_queue, emptyQueue);
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::sleepAndPushPendingQueue()
{
	try
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(90));
		pushPendingQueue();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void BidCoSQueue::pushPendingQueue()
{
	try
	{
		if(!_pendingQueues || _pendingQueues->empty()) return;
		while(!_pendingQueues->empty() && _pendingQueues->front()->isEmpty())
		{
			if(GD::debugLevel >= 5) std::cout << "Debug: Empty queue was pushed." << std::endl;
			_pendingQueues->pop();
		}
		if(_pendingQueues->empty()) return;
		std::shared_ptr<BidCoSQueue> queue;
		queue = _pendingQueues->front();
		_queueType = queue->getQueueType();
		serviceMessages = queue->serviceMessages;
		queueEmptyCallback = queue->queueEmptyCallback;
		callbackParameter = queue->callbackParameter;
		burst = queue->burst;
		for(std::deque<BidCoSQueueEntry>::iterator i = queue->getQueue()->begin(); i != queue->getQueue()->end(); ++i)
		{
			if(!noSending && i->getType() == QueueEntryType::MESSAGE && i->getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queue.push_back(*i);
				resendCounter = 0;
				if(!noSending)
				{
					std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, i->getMessage().get(), i->getPacket());
					send.detach();
					startResendThread();
				}
			}
			else if(!noSending && i->getType() == QueueEntryType::PACKET && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
			{
				_queue.push_back(*i);
				resendCounter = 0;
				if(!noSending)
				{
					std::thread send(&BidCoSQueue::send, this, i->getPacket());
					send.detach();
					startResendThread();
				}
			}
			else _queue.push_back(*i);
		}
		_workingOnPendingQueue = true;
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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

void BidCoSQueue::pop()
{
	try
	{
		keepAlive();
		if(GD::debugLevel >= 5) std::cout << "Popping from BidCoSQueue: " << id << std::endl;
		if(_popWaitThread && _popWaitThread->joinable()) _stopPopWaitThread = true;
		if(_resendThread && _resendThread->joinable()) _stopResendThread = true;
		if(_queue.empty()) return;
		_queueMutex.lock();
		_queue.pop_front();
		if(_queue.empty()) {
			if(queueEmptyCallback && callbackParameter) queueEmptyCallback(callbackParameter);
			if(_workingOnPendingQueue) _pendingQueues->pop();
			if(!_pendingQueues || (_pendingQueues && _pendingQueues->empty()))
			{
				_stopResendThread = true;
				if(GD::debugLevel >= 5) std::cout << "Queue " << id << " is empty and there are no pending queues." << std::endl;
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
				if(GD::debugLevel >= 5) std::cout << "Queue " << id << " is empty. Pushing pending queue..." << std::endl;
				_queueMutex.unlock();
				std::thread push(&BidCoSQueue::pushPendingQueue, this);
				push.detach();
				return;
			}
		}
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			resendCounter = 0;
			std::thread send;
			if(!noSending)
			{
				if(_queue.front().getType() == QueueEntryType::MESSAGE)
				{
					send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
				}
				else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket());
				send.detach();
				_queueMutex.unlock(); //Has to be unlocked before startResendThread()
				startResendThread();
			}
			else _queueMutex.unlock();
		}
		else _queueMutex.unlock();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::string BidCoSQueue::serialize()
{
	try
	{
		if(_queue.size() == 0) return "";
		std::ostringstream stringstream;
		stringstream << std::hex << std::uppercase << std::setfill('0');
		stringstream << std::setw(2) << (uint32_t)_queueType;
		stringstream << std::setw(4) << _queue.size();
		for(std::deque<BidCoSQueueEntry>::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			stringstream << std::setw(1) << (int32_t)i->getType();
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
		return stringstream.str();
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return "";
}
