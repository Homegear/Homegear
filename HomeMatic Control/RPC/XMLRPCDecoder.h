#ifndef XMLRPCDECODER_H_
#define XMLRPCDECODER_H_

#include <memory>
#include <vector>

#include "../Exception.h"
#include "RPCVariable.h"
#include "rapidxml.hpp"

using namespace rapidxml;

namespace RPC {

class XMLRPCDecoder {
public:
	XMLRPCDecoder() {}
	virtual ~XMLRPCDecoder() {}

	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeRequest(std::shared_ptr<char> packet, uint32_t packetLength, std::string* methodName);
	std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<char> packet, uint32_t packetLength, uint32_t offset = 0);
private:
	std::shared_ptr<RPCVariable> decodeParameter(xml_node<>* valueNode);
	std::shared_ptr<RPCVariable> decodeArray(xml_node<>* dataNode);
	std::shared_ptr<RPCVariable> decodeStruct(xml_node<>* structNode);
};

} /* namespace RPC */
#endif /* XMLRPCDECODER_H_ */
