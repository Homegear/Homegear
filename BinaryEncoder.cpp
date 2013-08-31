#include "BinaryEncoder.h"

void BinaryEncoder::encodeInteger(std::shared_ptr<std::vector<char>>& encodedData, int32_t integer)
{
	try
	{
		char result[4];
		HelperFunctions::memcpyBigEndian(result, (char*)&integer, 4);
		encodedData->insert(encodedData->end(), result, result + 4);
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

void BinaryEncoder::encodeInteger(std::vector<uint8_t>& encodedData, int32_t integer)
{
	try
	{
		uint8_t result[4];
		HelperFunctions::memcpyBigEndian(result, (uint8_t*)&integer, 4);
		encodedData.insert(encodedData.end(), result, result + 4);
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

void BinaryEncoder::encodeByte(std::shared_ptr<std::vector<char>>& encodedData, uint8_t byte)
{
	try
	{
		encodedData->push_back(byte);
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

void BinaryEncoder::encodeByte(std::vector<uint8_t>& encodedData, uint8_t byte)
{
	try
	{
		encodedData.push_back(byte);
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

void BinaryEncoder::encodeString(std::shared_ptr<std::vector<char>>& encodedData, std::string& string)
{
	try
	{
		encodeInteger(encodedData, string.size());
		if(string.size() > 0) encodedData->insert(encodedData->end(), string.begin(), string.end());
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

void BinaryEncoder::encodeString(std::vector<uint8_t>& encodedData, std::string& string)
{
	try
	{
		encodeInteger(encodedData, string.size());
		if(string.size() > 0) encodedData.insert(encodedData.end(), string.begin(), string.end());
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

void BinaryEncoder::encodeBoolean(std::shared_ptr<std::vector<char>>& encodedData, bool boolean)
{
	try
	{
		encodedData->push_back((char)boolean);
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

void BinaryEncoder::encodeBoolean(std::vector<uint8_t>& encodedData, bool boolean)
{
	try
	{
		encodedData.push_back((uint8_t)boolean);
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

void BinaryEncoder::encodeFloat(std::shared_ptr<std::vector<char>>& encodedData, double floatValue)
{
	try
	{
		double temp = std::abs(floatValue);
		int32_t exponent = 0;
		if(temp != 0 && temp < 0.5)
		{
			while(temp < 0.5)
			{
				temp *= 2;
				exponent--;
			}
		}
		else while(temp >= 1)
		{
			temp /= 2;
			exponent++;
		}
		if(floatValue < 0) temp *= -1;
		int32_t mantissa = std::lround(temp * 0x40000000);
		char data[8];
		HelperFunctions::memcpyBigEndian(data, (char*)&mantissa, 4);
		HelperFunctions::memcpyBigEndian(data + 4, (char*)&exponent, 4);
		encodedData->insert(encodedData->end(), data, data + 8);
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

void BinaryEncoder::encodeFloat(std::vector<uint8_t>& encodedData, double floatValue)
{
	try
	{
		double temp = std::abs(floatValue);
		int32_t exponent = 0;
		if(temp != 0 && temp < 0.5)
		{
			while(temp < 0.5)
			{
				temp *= 2;
				exponent--;
			}
		}
		else while(temp >= 1)
		{
			temp /= 2;
			exponent++;
		}
		if(floatValue < 0) temp *= -1;
		int32_t mantissa = std::lround(temp * 0x40000000);
		char data[8];
		HelperFunctions::memcpyBigEndian(data, (char*)&mantissa, 4);
		HelperFunctions::memcpyBigEndian(data + 4, (char*)&exponent, 4);
		encodedData.insert(encodedData.end(), data, data + 8);
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
