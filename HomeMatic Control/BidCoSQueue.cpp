#include "BidCoSQueue.h"
#include "GD.h"

BidCoSQueue::BidCoSQueue() : _queueType(BidCoSQueueType::EMPTY)
{
	peer = shared_ptr<Peer>(new Peer());
}

BidCoSQueue::BidCoSQueue(BidCoSQueueType queueType) : BidCoSQueue()
{
	_queueType = queueType;
}

BidCoSQueue::BidCoSQueue(std::string serializedObject, HomeMaticDevice* device) : BidCoSQueue()
{
	if(GD::debugLevel >= 5) cout << "Unserializing queue: " << serializedObject << endl;
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
			shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
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
				if(!noSending)
				{
					if(_queue.front().getType() == QueueEntryType::MESSAGE)
					{
						if(GD::debugLevel >= 5) cout << "Invoking outgoing message handler from BidCoSQueue." << endl;
						send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
					}
					else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket());
					send.detach();
				}
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

void BidCoSQueue::push(shared_ptr<BidCoSPacket> packet)
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
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::push(shared_ptr<std::queue<shared_ptr<BidCoSQueue>>>& pendingQueues)
{
	_pendingQueues = pendingQueues;
	if(_queue.empty()) pushPendingQueue();
}

void BidCoSQueue::push(shared_ptr<BidCoSMessage> message, shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(message->getDirection() != DIRECTIONOUT && GD::debugLevel >= 3) cout << "Warning: Wrong push method used. Packet is not necessary for incoming messages" << endl;
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
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::push(shared_ptr<BidCoSMessage> message)
{
	try
	{
		if(message->getDirection() == DIRECTIONOUT && GD::debugLevel >= 1) cout << "Critical: Wrong push method used. Please provide the received packet for outgoing messages" << endl;
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
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSQueue::send(shared_ptr<BidCoSPacket> packet)
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

void BidCoSQueue::sleepAndPushPendingQueue()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(90));
	pushPendingQueue();
}

void BidCoSQueue::pushPendingQueue()
{
	if(_pendingQueues.get() == nullptr || _pendingQueues->empty()) return;
	_queueType = _pendingQueues->front()->getQueueType();
	while(!_pendingQueues->empty() && _pendingQueues->front()->isEmpty())
	{
		_pendingQueues->pop();
	}
	if(_pendingQueues->empty()) return;
	for(std::deque<BidCoSQueueEntry>::iterator i = _pendingQueues->front()->getQueue()->begin(); i != _pendingQueues->front()->getQueue()->end(); ++i)
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

void BidCoSQueue::keepAlive()
{
	if(lastAction != nullptr) *lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BidCoSQueue::pop()
{
	try
	{
		keepAlive();
		if(GD::debugLevel >= 5) cout << "Popping from BidCoSQueue." << endl;
		if(_queue.empty()) return;
		_queueMutex.lock();
		_queue.pop_front();
		if(_queue.empty()) {
			if(_workingOnPendingQueue) _pendingQueues->pop();
			if(_pendingQueues.get() != nullptr && _pendingQueues->size() == 0)
			{
				if(GD::debugLevel >= 5) cout << "Queue is empty and there are no pending queues." << endl;
				_workingOnPendingQueue = false;
				_queueMutex.unlock();
				return;
			}
			else
			{
				if(GD::debugLevel >= 5) cout << "Queue is empty. Pushing pending queue..." << endl;
				_queueMutex.unlock();
				std::thread push(&BidCoSQueue::sleepAndPushPendingQueue, this);
				push.detach();
				return;
			}
		}
		_resendThreadMutex.lock();
		if(_resendThread.get() != nullptr && _resendThread->joinable()) _stopResendThread = true;
		_resendThreadMutex.unlock();
		if((_queue.front().getType() == QueueEntryType::MESSAGE && _queue.front().getMessage()->getDirection() == DIRECTIONOUT) || _queue.front().getType() == QueueEntryType::PACKET)
		{
			resendCounter = 0;
			std::thread send;
			if(!noSending)
			{
				if(_queue.front().getType() == QueueEntryType::MESSAGE) send = std::thread(&BidCoSMessage::invokeMessageHandlerOutgoing, _queue.front().getMessage().get(), _queue.front().getPacket());
				else send = std::thread(&BidCoSQueue::send, this, _queue.front().getPacket());
				send.detach();
				_queueMutex.unlock(); //Has to be unlocked before startResendThread()
				startResendThread();
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_queueMutex.unlock();
}

std::string BidCoSQueue::serialize()
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
		shared_ptr<BidCoSMessage> message = i->getMessage();
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
