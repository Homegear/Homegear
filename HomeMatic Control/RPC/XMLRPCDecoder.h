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

	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName);
	std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<std::vector<char>> packet);
	std::shared_ptr<RPCVariable> decodeResponse(std::string& packet);
private:
	std::shared_ptr<RPCVariable> decodeParameter(xml_node<>* valueNode);
	std::shared_ptr<RPCVariable> decodeArray(xml_node<>* dataNode);
	std::shared_ptr<RPCVariable> decodeStruct(xml_node<>* structNode);
	std::shared_ptr<RPCVariable> decodeResponse(xml_document<>* doc);
};

} /* namespace RPC */
#endif /* XMLRPCDECODER_H_ */
