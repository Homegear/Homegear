#include "HomeMaticCentral.h"
#include "../GD.h"

HomeMaticCentral::HomeMaticCentral()
{

}

HomeMaticCentral::HomeMaticCentral(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

HomeMaticCentral::HomeMaticCentral(std::string serializedObject) : HomeMaticDevice(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), 0, 0)
{
	init();
}

HomeMaticCentral::~HomeMaticCentral()
{

}

void HomeMaticCentral::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = HMDeviceTypes::HMCENTRAL;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::setUpBidCoSMessages()
{
	//Don't call HomeMaticDevice::setUpBidCoSMessages!
	_messages->add(BidCoSMessage(0x00, this, NOACCESS, FULLACCESS, &HomeMaticDevice::handlePairingRequest));

	_messages->add(BidCoSMessage(0x02, this, NOACCESS, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck));
}

std::string HomeMaticCentral::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	return  stringstream.str();
}

void HomeMaticCentral::packetReceived(BidCoSPacket* packet)
{
	try
	{
		HomeMaticDevice::packetReceived(packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::handleCLICommand(std::string command)
{
	if(command == "pairing mode on")
	{
		_pairing = true;
		cout << "Pairing mode enabled." << endl;
	}
	else if(command == "pairing mode off")
	{
		_pairing = false;
		cout << "Pairing mode disabled." << endl;
	}
}

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(packet->destinationAddress() == 0)
		{
			newBidCoSQueue(BidCoSQueueType::PAIRING);
			if(_peers.find(packet->senderAddress()) != _peers.end())
			{
				//TODO Send config
				return;
			}
			//TODO Check if device type and firmware version is known before pairing

			_bidCoSQueue->peer.address = packet->senderAddress();

			//CONFIG_START
			std::vector<uint8_t> payload;
			payload.push_back(0);
			payload.push_back(0x05);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
			_bidCoSQueue->push(configPacket);
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//CONFIG_WRITE_INDEX
			payload.push_back(0);
			payload.push_back(0x08);
			payload.push_back(0x02);
			payload.push_back(0x01);
			payload.push_back(0x0A);
			payload.push_back(_address >> 16);
			payload.push_back(0x0B);
			payload.push_back((_address >> 8) & 0xFF);
			payload.push_back(0x0C);
			payload.push_back(_address & 0xFF);
			configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
			_bidCoSQueue->push(configPacket);
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//END_CONFIG
			payload.push_back(0);
			payload.push_back(0x06);
			configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
			_bidCoSQueue->push(configPacket);
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//TODO Request peers (known through xml files)
			GD::cul.sendPacket(_bidCoSQueue->front()->getPacket());
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::handleAck(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		_bidCoSQueue->pop(); //Messages are not popped by default.
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRING && _bidCoSQueue->isEmpty())
		{
			cout << "Added device 0x" << std::hex << _bidCoSQueue->peer.address << std::dec << endl;
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

