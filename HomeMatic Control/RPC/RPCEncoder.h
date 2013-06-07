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
	RPCEncoder() { _packetStart[0] = 'B'; _packetStart[1] = 'i'; _packetStart[2] = 'n'; _packetStart[3] = 1; _packetStartError[0] = 'B'; _packetStartError[1] = 'i'; _packetStartError[2] = 'n'; _packetStartError[3] = 0xFF; }
	virtual ~RPCEncoder() {}

	uint32_t encodeResponse(std::shared_ptr<char>& packet, std::shared_ptr<RPCVariable> variable);
private:
	char _packetStart[4];
	char _packetStartError[4];

	uint32_t calculateLength(std::shared_ptr<RPCVariable> variable);
	void encodeVariable(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
	void encodeRawInteger(char* packet, uint32_t* position, uint32_t packetLength, int32_t integer);
	void encodeInteger(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
	void encodeBoolean(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
	void encodeType(char* packet, uint32_t* position, uint32_t packetLength, RPCVariableType type);
	void encodeRawString(char* packet,  uint32_t* position, uint32_t packetLength, std::string string);
	void encodeString(char* packet,  uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
	void encodeVoid(char* packet, uint32_t* position, uint32_t packetLength);
	void encodeStruct(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
	void encodeArray(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable);
};

} /* namespace RPC */
#endif /* RPCENCODER_H_ */
