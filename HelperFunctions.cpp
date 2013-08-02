/*
 * HelperFunctions.cpp
 *
 *  Created on: May 23, 2013
 *      Author: sathya
 */

#include "HelperFunctions.h"
#include "GD.h"

HelperFunctions::~HelperFunctions() {

}

void HelperFunctions::memcpyBigEndian(char* to, char* from, const uint32_t& length)
{
	if(GD::bigEndian) memcpy(to, from, length);
	else
	{
		uint32_t last = length - 1;
		for(uint32_t i = 0; i < length; i++)
		{
			to[i] = from[last - i];
		}
	}
}

void HelperFunctions::memcpyBigEndian(uint8_t* to, uint8_t* from, const uint32_t& length)
{
	memcpyBigEndian((char*)to, (char*)from, length);
}

void HelperFunctions::memcpyBigEndian(int32_t& to, std::vector<uint8_t>& from)
{
	to = 0; //Necessary if length is < 4
	if(from.empty()) return;
	uint32_t length = from.size();
	if(length > 4) length = 4;
	if(GD::bigEndian) memcpyBigEndian(((uint8_t*)&to) + (4 - length), &from.at(0), length);
	else memcpyBigEndian(((uint8_t*)&to), &from.at(0), length);
}

void HelperFunctions::memcpyBigEndian(std::vector<uint8_t>& to, int32_t& from)
{
	if(!to.empty()) to.clear();
	int32_t length = 4;
	if(from < 0) length = 4;
	else if(from < 256) length = 1;
	else if(from < 65536) length = 2;
	else if(from < 16777216) length = 3;
	to.resize(length, 0);
	if(GD::bigEndian) memcpyBigEndian(&to.at(0), (uint8_t*)&from + (4 - length), length);
	else memcpyBigEndian(&to.at(0), (uint8_t*)&from, length);
}

std::pair<std::string, std::string> HelperFunctions::split(std::string string, char delimiter)
{
	try
	{

		int32_t pos = string.find_last_of(delimiter);
		if(pos == -1) return std::pair<std::string, std::string>(string, "");
		if((unsigned)pos + 1 >= string.size()) return std::pair<std::string, std::string>(string.substr(0, pos), "");
		return std::pair<std::string, std::string>(string.substr(0, pos), string.substr(pos + 1));
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
    return std::pair<std::string, std::string>();
}

void HelperFunctions::printBinary(std::shared_ptr<std::vector<char>> data)
{
	if(!data) return;
	std::ostringstream stringstream;
	stringstream << std::hex << std::setfill('0') << std::uppercase;
	for(std::vector<char>::iterator i = data->begin(); i != data->end(); ++i)
	{
		stringstream << std::setw(2) << (int32_t)((uint8_t)(*i));
	}
	stringstream << std::dec;
	std::cout << stringstream.str() << std::endl;
}
