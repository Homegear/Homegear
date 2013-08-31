#include "PendingBidCoSQueues.h"
#include "GD.h"
#include "HelperFunctions.h"

PendingBidCoSQueues::PendingBidCoSQueues()
{
}

void PendingBidCoSQueues::serialize(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		_queuesMutex.lock();
		encoder.encodeInteger(encodedData, _queues.size());
		for(std::deque<std::shared_ptr<BidCoSQueue>>::iterator i = _queues.begin(); i != _queues.end(); ++i)
		{
			std::vector<uint8_t> serializedQueue;
			(*i)->serialize(serializedQueue);
			encoder.encodeInteger(encodedData, serializedQueue.size());
			encodedData.insert(encodedData.end(), serializedQueue.begin(), serializedQueue.end());
			bool hasCallbackFunction = ((*i)->callbackParameter && (*i)->callbackParameter->integers.size() == 3 && (*i)->callbackParameter->strings.size() == 1);
			encoder.encodeBoolean(encodedData, hasCallbackFunction);
			if(hasCallbackFunction)
			{
				encoder.encodeInteger(encodedData, (*i)->callbackParameter->integers.at(0));
				encoder.encodeString(encodedData, (*i)->callbackParameter->strings.at(0));
				encoder.encodeInteger(encodedData, (*i)->callbackParameter->integers.at(1));
				encoder.encodeInteger(encodedData, (*i)->callbackParameter->integers.at(2) / 1000);
			}
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
	_queuesMutex.unlock();
}

void PendingBidCoSQueues::unserialize(std::shared_ptr<std::vector<char>> serializedData, Peer* peer, HomeMaticDevice* device)
{
	try
	{
		BinaryDecoder decoder;
		uint32_t position = 0;
		_queuesMutex.lock();
		uint32_t pos = 0;
		uint32_t pendingQueuesSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < pendingQueuesSize; i++)
		{
			uint32_t queueLength = decoder.decodeInteger(serializedData, position);
			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue());
			queue->unserialize(serializedData, device, position);
			position += queueLength;
			queue->noSending = true;
			bool hasCallbackFunction = decoder.decodeBoolean(serializedData, position);
			if(hasCallbackFunction)
			{
				std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
				parameters->integers.push_back(decoder.decodeInteger(serializedData, position));
				parameters->strings.push_back(decoder.decodeString(serializedData, position));
				parameters->integers.push_back(decoder.decodeInteger(serializedData, position));
				parameters->integers.push_back(decoder.decodeInteger(serializedData, position) * 1000);
				queue->callbackParameter = parameters;
				queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<Peer, &Peer::addVariableToResetCallback>(peer);
			}
			_queues.push_back(queue);
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
    _queuesMutex.unlock();
}

void PendingBidCoSQueues::unserialize_0_0_6(std::string serializedObject, Peer* peer, HomeMaticDevice* device)
{
	try
	{
		HelperFunctions::printDebug( "Unserializing pending BidCoS queues: " + serializedObject);
		if(serializedObject.empty()) return;

		_queuesMutex.lock();
		std::istringstream stringstream(serializedObject);
		uint32_t pos = 0;
		uint32_t pendingQueuesSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
		for(uint32_t i = 0; i < pendingQueuesSize; i++)
		{
			uint32_t queueLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(queueLength > 6)
			{
				std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue());
				queue->unserialize_0_0_6(serializedObject.substr(pos, queueLength), device);
				pos += queueLength;
				queue->noSending = true;
				bool hasCallbackFunction = std::stol(serializedObject.substr(pos, 1)); pos += 1;
				if(hasCallbackFunction)
				{
					std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 4), 0, 16)); pos += 4;
					uint32_t keyLength = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
					parameters->strings.push_back(serializedObject.substr(pos, keyLength)); pos += keyLength;
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 8), 0, 16) * 1000); pos += 8;
					queue->callbackParameter = parameters;
					queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<Peer, &Peer::addVariableToResetCallback>(peer);
				}
				_queues.push_back(queue);
			}
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
    _queuesMutex.unlock();
}

bool PendingBidCoSQueues::empty()
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
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void PendingBidCoSQueues::push(std::shared_ptr<BidCoSQueue> queue)
{
	try
	{
		if(!queue || queue->isEmpty()) return;
		_queuesMutex.lock();
		_queues.push_back(queue);
		_queuesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PendingBidCoSQueues::pop()
{
	try
	{
		_queuesMutex.lock();
		if(!_queues.empty()) _queues.pop_front();
		_queuesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PendingBidCoSQueues::clear()
{
	try
	{
		_queuesMutex.lock();
		_queues.clear();
		_queuesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint32_t PendingBidCoSQueues::size()
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
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

std::shared_ptr<BidCoSQueue> PendingBidCoSQueues::front()
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue;
		_queuesMutex.lock();
		if(!_queues.empty()) queue =_queues.front();
		_queuesMutex.unlock();
		return queue;
	}
	catch(const std::exception& ex)
    {
		_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queuesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<BidCoSQueue>();
}
