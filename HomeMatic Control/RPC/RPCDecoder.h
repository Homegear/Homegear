#ifndef RPCDECODER_H_
#define RPCDECODER_H_

#include <memory>
#include <vector>
#include <cstring>

#include "RPCVariable.h"

namespace RPC
{

class RPCDecoder
{
public:
	RPCDecoder() {}
	virtual ~RPCDecoder() {}

	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeRequest(std::shared_ptr<char> packet, uint32_t packetLength, std::string* methodName);
	std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<char> packet, uint32_t packetLength, uint32_t offset = 0);
private:
	int32_t decodeInteger(char* packet, uint32_t packetLength, uint32_t* position);
	std::string decodeString(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<RPCVariable> decodeParameter(char* packet, uint32_t packetLength, uint32_t* position);
	RPCVariableType decodeType(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeArray(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeStruct(char* packet, uint32_t packetLength, uint32_t* position);
};

} /* namespace RPC */
#endif /* RPCDECODER_H_ */
