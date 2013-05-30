#include "BidCoSQueue.h"

BidCoSQueue::BidCoSQueue() : _queueType(BidCoSQueueType::EMPTY)
{
}

BidCoSQueue::BidCoSQueue(BidCoSQueueType queueType) : _queueType(queueType)
{
}

BidCoSQueue::~BidCoSQueue()
{
	try
	{
		_stopResendThread = true;
		_resendThreadMutex.lock();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Wait for _resendThread to finish. I didn't use thread.join, because for some reason "join" sometimes crashed the program.
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_resendThreadMutex.unlock();
}

void BidCoSQueue::resend()
{
	try
	{
		int32_t i = 0;
		std::chrono::milliseconds sleepingTime(50);
		while(!_stopResendThread && i < 4)
		{
			std::this_thread::sleep_for(sleepingTime);
			i++;
		}
		if(!_stopResendThread)
		{
			_queueMutex.lock();
			if(!_queue.empty())
			{
				std::thread send;
				if(_queue.front().getType() == QueueEntryType::MESSAGE) send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage(), _queue.front().getMessage()->getDevice()->getReceivedPacket());
				else send = std::thread(&BidCoSQueue::send, this, *_queue.front().getPacket());
				send.detach();
				_queueMutex.unlock(); //Has to be unlocked before startResendThread
				if(resendCounter < 1) //This actually means that the message will be sent three times all together if there is no response
				{
					resendCounter++;
					startResendThread();
				}
				else resendCounter = 0;
			}
		}
		else _stopResendThread = false;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_queueMutex.unlock();
}

void BidCoSQueue::push(const BidCoSPacket& packet)
{
	try
	{
		BidCoSQueueEntry entry;
		entry.setPacket(packet);
		if(!noSending && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			resendCounter = 0;
			std::thread send(&BidCoSQueue::send, this, *entry.getPacket());
			send.detach();
			startResendThread();
		}
		else
		{
			_queue.push_back(entry);
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::push(shared_ptr<BidCoSQueue>& pendingBidCoSQueue)
{
	_pendingBidCoSQueue = pendingBidCoSQueue;
	for(std::deque<BidCoSQueueEntry>::iterator i = pendingBidCoSQueue->getQueue()->begin(); i != pendingBidCoSQueue->getQueue()->end(); ++i)
	{
		if(!noSending && i->getType() == QueueEntryType::MESSAGE && i->getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(*i);
			resendCounter = 0;
			std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, i->getMessage(), i->getMessage()->getDevice()->getReceivedPacket());
			send.detach();
			startResendThread();
		}
		else if(!noSending && i->getType() == QueueEntryType::PACKET && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(*i);
			resendCounter = 0;
			std::thread send(&BidCoSQueue::send, this, *(i->getPacket()));
			send.detach();
			startResendThread();
		}
		else _queue.push_back(*i);
	}
}

void BidCoSQueue::push(BidCoSMessage* message)
{
	try
	{
		if(message == nullptr) return;
		BidCoSQueueEntry entry;
		entry.setMessage(message);
		if(!noSending && entry.getMessage()->getDirection() == DIRECTIONOUT && (_queue.size() == 0 || (_queue.size() == 1 && _queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONIN)))
		{
			_queue.push_back(entry);
			resendCounter = 0;
			std::thread send(&BidCoSMessage::invokeMessageHandlerOutgoing, message, message->getDevice()->getReceivedPacket());
			send.detach();
			startResendThread();
		}
		else
		{
			_queue.push_back(entry);
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::send(BidCoSPacket packet)
{
	try
	{
		if(noSending) return;
		if(device != nullptr) device->sendPacket(packet);
		else GD::cul.sendPacket(packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::startResendThread()
{
	try
	{
		_queueMutex.lock();
		uint8_t controlByte = (_queue.front().getType() == QueueEntryType::MESSAGE) ? _queue.front().getMessage()->getControlByte() : _queue.front().getPacket()->controlByte();
		if(!(controlByte & 0x02) && (controlByte & 0x20)) //Resend when no response?
		{
			_resendThreadMutex.lock();
			_resendThread = unique_ptr<std::thread>(new std::thread(&BidCoSQueue::resend, this));
			_resendThread->detach();
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_resendThreadMutex.unlock();
	_queueMutex.unlock();
}

void BidCoSQueue::clear()
{
	std::deque<BidCoSQueueEntry> emptyQueue;
	std::swap(_queue, emptyQueue);
}

void BidCoSQueue::pop()
{
	try
	{
		if(_queue.empty()) return;
		_queueMutex.lock();
		_queue.pop_front();
		if(_queue.empty()) {
			if(_pendingBidCoSQueue.get() != nullptr) _pendingBidCoSQueue->clear();
			_queueMutex.unlock();
			return;
		}
		_resendThreadMutex.lock();
		if(_resendThread.get() != nullptr && _resendThread->joinable()) _stopResendThread = true;
		_resendThreadMutex.unlock();
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			resendCounter = 0;
			std::thread send;
			if(_queue.front().getType() == QueueEntryType::MESSAGE) send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage(), _queue.front().getMessage()->getDevice()->getReceivedPacket());
			else send = std::thread(&BidCoSQueue::send, this, *_queue.front().getPacket());
			send.detach();
			_queueMutex.unlock(); //Has to be unlocked before startResendThread()
			startResendThread();
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_queueMutex.unlock();
}
