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
	std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<char> packet, uint32_t packetLength);
private:
	int32_t getInteger(char* packet, uint32_t packetLength, uint32_t* position);
	std::string getString(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<RPCVariable> getParameter(char* packet, uint32_t packetLength, uint32_t* position);
	RPCVariableType getVariableType(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> getArray(char* packet, uint32_t packetLength, uint32_t* position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> getStruct(char* packet, uint32_t packetLength, uint32_t* position);
};

} /* namespace RPC */
#endif /* RPCDECODER_H_ */
