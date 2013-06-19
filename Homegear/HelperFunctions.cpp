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

void HelperFunctions::memcpyBigEndian(char* to, char* from, uint32_t length)
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
		stringstream << std::setw(2) << (int32_t)(*i);
	}
	stringstream << std::dec;
	std::cout << stringstream.str() << std::endl;
}
