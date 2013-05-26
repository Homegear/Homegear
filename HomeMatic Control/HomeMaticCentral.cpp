#include "HomeMaticCentral.h"
#include "GD.h"

void HomeMaticCentral::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times

		GD::cul.setHomeMaticCentral(this);
		_initialized = true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::packetReceived(BidCoSPacket* packet)
{
	try
	{
		_receivedPacketMutex.lock();
		/*_receivedPacket = *packet; //Make a copy of packet
		BidCoSMessage* message = _messages->find(DIRECTIONIN, &_receivedPacket);
		if(message != nullptr && message->checkAccess(&_receivedPacket, _bidCoSQueue.get()))
		{
			std::chrono::milliseconds sleepingTime(90); //Don't respond too fast
			std::this_thread::sleep_for(sleepingTime);
			_messageCounter[_receivedPacket.senderAddress()] = _receivedPacket.messageCounter();
			message->invokeMessageHandlerIncoming(&_receivedPacket);
			_lastReceivedMessage = message;
		}
		if(message == nullptr && _receivedPacket.destinationAddress() == _address) cout << "Could not process message. Unknown message type: " << _receivedPacket.hexString() << endl;*/
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	_receivedPacketMutex.unlock();
}
