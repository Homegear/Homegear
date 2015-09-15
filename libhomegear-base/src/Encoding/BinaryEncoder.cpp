/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "BinaryEncoder.h"
#include "../BaseLib.h"

namespace BaseLib
{

BinaryEncoder::BinaryEncoder(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void BinaryEncoder::encodeInteger(std::vector<char>& encodedData, int32_t integer)
{
	try
	{
		char result[4];
		_bl->hf.memcpyBigEndian(result, (char*)&integer, 4);
		encodedData.insert(encodedData.end(), result, result + 4);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BinaryEncoder::encodeInteger(std::vector<uint8_t>& encodedData, int32_t integer)
{
	try
	{
		uint8_t result[4];
		_bl->hf.memcpyBigEndian(result, (uint8_t*)&integer, 4);
		encodedData.insert(encodedData.end(), result, result + 4);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BinaryEncoder::encodeByte(std::vector<char>& encodedData, uint8_t byte)
{
	try
	{
		encodedData.push_back(byte);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BinaryEncoder::encodeString(std::vector<char>& encodedData, std::string& string)
{
	try
	{
		encodeInteger(encodedData, string.size());
		if(string.size() > 0) encodedData.insert(encodedData.end(), string.begin(), string.end());
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BinaryEncoder::encodeBoolean(std::vector<char>& encodedData, bool boolean)
{
	try
	{
		encodedData.push_back((char)boolean);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BinaryEncoder::encodeFloat(std::vector<char>& encodedData, double floatValue)
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
		_bl->hf.memcpyBigEndian(data, (char*)&mantissa, 4);
		_bl->hf.memcpyBigEndian(data + 4, (char*)&exponent, 4);
		encodedData.insert(encodedData.end(), data, data + 8);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		_bl->hf.memcpyBigEndian(data, (char*)&mantissa, 4);
		_bl->hf.memcpyBigEndian(data + 4, (char*)&exponent, 4);
		encodedData.insert(encodedData.end(), data, data + 8);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
