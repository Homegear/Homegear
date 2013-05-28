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
	_messages->add(BidCoSMessage(0x00, this, ACCESSPAIREDTOSENDER, FULLACCESS, &HomeMaticDevice::handlePairingRequest));

	_messages->add(BidCoSMessage(0x02, this, NOACCESS, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck));

	_messages->add(BidCoSMessage(0x10, this, NOACCESS, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse));
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

Peer HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter)
{
	Peer peer;
	peer.address = address;
	peer.firmwareVersion = firmwareVersion;
	peer.deviceType = deviceType;
	peer.serialNumber = serialNumber;
	peer.remoteChannel = remoteChannel;
	peer.messageCounter = messageCounter;
    return peer;
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
			std::vector<uint8_t> payload;
			XMLRPC::Device* device;
			if(_pairing)
			{
				newBidCoSQueue(BidCoSQueueType::PAIRING);

				device = GD::xmlrpcDevices.find(packet);
				if(device == nullptr)
				{
					if(GD::debugLevel >= 3) cout << "Warning: Device type not supported. Sender address 0x" << std::hex << packet->senderAddress() << std::dec << "." << std::endl;
					return;
				}

				std::string serialNumber;
				for(int i = 3; i < 13; i++)
					serialNumber += std::to_string(packet->payload()->at(i));
				_bidCoSQueue->peer = createPeer(packet->senderAddress(), packet->payload()->at(9), (HMDeviceTypes)((packet->payload()->at(10) << 8) + packet->payload()->at(11)), serialNumber, 0, 0);
				_bidCoSQueue->peer.xmlrpcDevice = device;

				//CONFIG_START
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
			}
			else
			{
				newBidCoSQueue(BidCoSQueueType::DEFAULT);
				if(_peers.find(packet->senderAddress()) == _peers.end()) return;
				device = _peers[packet->senderAddress()].xmlrpcDevice;
			}

			for(std::vector<XMLRPC::DeviceChannel>::iterator i = device->channels.begin(); i != device->channels.end(); ++i)
			{
				//Channel 0 - MASTER is the devices MASTER parameter set.
				if(i->index == 0 || i->parameterSetsByType[XMLRPC::ParameterSet::Type::master]->parameters.size() > 0)
				{

				}
				if(i->parameterSetsByType[XMLRPC::ParameterSet::Type::values]->parameters.size() > 0)
				{

				}
				if(i->parameterSetsByType.find(XMLRPC::ParameterSet::Type::link) != i->parameterSetsByType.end())
				{
					payload.push_back(i->index);
					payload.push_back(0x03);
					BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
					_bidCoSQueue->push(configPacket);
					_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<int32_t, int32_t>>()));
					payload.clear();
					_messageCounter[0]++;
				}
			}
			sendPacket(_bidCoSQueue->front()->getPacket());
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

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(_sentPacket.payload()->at(1) == 0x03) //Peer request
			{
				for(uint32_t i = 1; i < packet->payload()->size(); i += 4)
				{
					int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
					int32_t peerChannel = packet->payload()->at(i + 3);
					PairedPeer peer;
					peer.address = peerAddress;
					peer.channel = peerChannel;
					_peers[packet->senderAddress()].peers.push_back(peer);
				}
			}
		}
		_bidCoSQueue->pop(); //Messages are not popped by default.
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
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(_sentPacket.messageType() == 0x01 && _sentPacket.payload()->at(0) == 0x00 && _sentPacket.payload()->at(1) == 0x06)
			{
				_peers[_bidCoSQueue->peer.address] = _bidCoSQueue->peer;
				cout << "Added device 0x" << std::hex << _bidCoSQueue->peer.address << std::dec << endl;
			}
		}
		_bidCoSQueue->pop(); //Messages are not popped by default.
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

