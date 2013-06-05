/*
 * RPCEncoder.h
 *
 *  Created on: Jun 5, 2013
 *      Author: sathya
 */

#ifndef RPCENCODER_H_
#define RPCENCODER_H_

#include <memory>
#include <cstring>

#include "RPCVariable.h"

namespace RPC
{

class RPCEncoder
{
public:
	RPCEncoder() { _packetStart[0] = 'B'; _packetStart[1] = 'i'; _packetStart[2] = 'n'; _packetStart[3] = 1; }
	virtual ~RPCEncoder() {}

	uint32_t encodeResponse(std::shared_ptr<char>& packet, std::shared_ptr<RPCVariable> value);
	uint32_t calculateLength(std::shared_ptr<RPCVariable> value);
	void encodeInteger(char* packet, uint32_t* position, int32_t integer);
	void encodeString(char* packet,  uint32_t* position, std::string);
	void encodeVoid(char* packet, uint32_t* position);
private:
	char _packetStart[4];
};

} /* namespace RPC */
#endif /* RPCENCODER_H_ */
