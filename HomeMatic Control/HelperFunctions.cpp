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
