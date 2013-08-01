#include "BidCoSPacket.h"
#include "GD.h"

//Properties
uint8_t BidCoSPacket::length()
{
    return _length;
}

uint8_t BidCoSPacket::messageCounter()
{
    return _messageCounter;
}

uint8_t BidCoSPacket::controlByte()
{
    return _controlByte;
}

uint8_t BidCoSPacket::messageType()
{
    return _messageType;
}

int32_t BidCoSPacket::messageSubtype()
{
	try
	{
		switch(_messageType)
		{
			case 0x01:
				return _payload.at(1);
			case 0x11:
				return _payload.at(0);
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return -1;
}

int32_t BidCoSPacket::senderAddress()
{
    return _senderAddress;
}

int32_t BidCoSPacket::destinationAddress()
{
    return _destinationAddress;
}

int32_t BidCoSPacket::channel()
{
    switch(_messageType)
    {
        case 0x01:
            return _payload.at(0);
        default:
            return -1;
    }
}

std::vector<uint8_t>* BidCoSPacket::payload()
{
    return &_payload;
}

std::string BidCoSPacket::hexString()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
		stringStream << std::setw(2) << (9 + _payload.size());
		stringStream << std::setw(2) << (int32_t)_messageCounter;
		stringStream << std::setw(2) << (int32_t)_controlByte;
		stringStream << std::setw(2) << (int32_t)_messageType;
		stringStream << std::setw(6) << _senderAddress;
		stringStream << std::setw(6) << _destinationAddress;
		std::for_each(_payload.begin(), _payload.end(), [&](uint8_t element) {stringStream << std::setw(2) << (int32_t)element;});
		return stringStream.str();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return "";
}
//End of properties

BidCoSPacket::BidCoSPacket()
{
}

BidCoSPacket::BidCoSPacket(std::string packet)
{
    import(packet);
}

BidCoSPacket::BidCoSPacket(uint8_t messageCounter, uint8_t controlByte, uint8_t messageType, int32_t senderAddress, int32_t destinationAddress, std::vector<uint8_t> payload)
     : _messageCounter(messageCounter), _controlByte(controlByte), _messageType(messageType), _senderAddress(senderAddress), _destinationAddress(destinationAddress), _payload(payload)
{
    _length = 9 + _payload.size();
}

BidCoSPacket::~BidCoSPacket()
{
}

//Packet looks like A...DATA...\r\n
void BidCoSPacket::import(std::string packet, bool removeFirstCharacter)
{
	try
	{
		uint32_t startIndex = removeFirstCharacter ? 1 : 0;
		_length = getByte(packet.substr(startIndex, 2));
		_messageCounter = getByte(packet.substr(startIndex + 2, 2));
		_controlByte = getByte(packet.substr(startIndex + 4, 2));
		_messageType = getByte(packet.substr(startIndex + 6, 2));
		_senderAddress = getInt(packet.substr(startIndex + 8, 6));
		_destinationAddress = getInt(packet.substr(startIndex + 14, 6));

		uint32_t tailLength = 0;
		if(packet.back() == '\n') tailLength = 2;
		_payload.clear();
		for(uint32_t i = startIndex + 20; i < packet.length() - tailLength; i+=2)
		{
			_payload.push_back(getByte(packet.substr(i, 2)));
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

uint8_t BidCoSPacket::getByte(std::string hexString)
{
	try
	{
		uint8_t value = 0;
		try	{ value = std::stoi(hexString, 0, 16); } catch(...) {}
		return value;
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
	return 0;
}

int32_t BidCoSPacket::getInt(std::string hexString)
{
	try
	{
		int32_t value = 0;
		try	{ value = std::stoll(hexString, 0, 16); } catch(...) {}
		return value;
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
	return 0;
}

void BidCoSPacket::setPosition(double index, double size, std::vector<uint8_t>& value)
{
	try
	{
		if(size < 0)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Negative size not allowed." << std::endl;
			return;
		}
		if(index < 9)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Packet index < 9 requested." << std::endl;
			return;
		}
		index -= 9;
		double byteIndex = std::floor(index);
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(value.empty()) value.push_back(0);
			int32_t intByteIndex = byteIndex;
			if(size > 1)
			{
				if(GD::debugLevel >= 2) std::cout << "Error: Can't set partial byte index > 1." << std::endl;
				return;
			}
			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			while((signed)_payload.size() - 1 < intByteIndex)
			{
				_payload.push_back(0);
			}
			_payload.at(intByteIndex) |= value.at(value.size() - 1) << (std::lround(index * 10) % 10);
		}
		else
		{
			uint32_t intByteIndex = byteIndex;
			uint32_t bytes = (uint32_t)std::ceil(size);
			while(_payload.size() < intByteIndex + bytes)
			{
				_payload.push_back(0);
			}
			while(value.size() < bytes)
			{
				value.push_back(0);
			}
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			_payload.at(intByteIndex) = value.at(0) & _bitmask[bitSize];
			for(uint32_t i = 1; i < bytes; i++)
			{
				_payload.at(intByteIndex + i) = value.at(i);
			}
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
    _length = 9 + _payload.size();
}

std::vector<uint8_t> BidCoSPacket::getPosition(double index, double size)
{
	std::vector<uint8_t> result;
	try
	{
		if(size < 0)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Negative negative size not allowed." << std::endl;
			return result;
		}
		if(index < 9)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Packet index < 9 requested." << std::endl;
			return result;
		}
		index -= 9;
		double byteIndex = std::floor(index);
		if(byteIndex >= _payload.size()) return result;
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(size > 1)
			{
				if(GD::debugLevel >= 2) std::cout << "Error: Partial byte index > 1 requested." << std::endl;
				return result;
			}
			//The round is necessary, because for example (uint32_t)(0.2 * 10) is 1
			uint32_t bitSize = std::lround(size * 10);
			result.push_back((_payload.at(byteIndex) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
		}
		else
		{
			uint32_t bytes = (uint32_t)std::ceil(size);
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			result.push_back(_payload.at(index) & _bitmask[bitSize]);
			for(uint32_t i = 1; i < bytes; i++)
			{
				result.push_back(_payload.at(index + i));
			}
		}
		return result;
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
    return result;
}
