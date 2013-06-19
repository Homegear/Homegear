#ifndef XMLRPCENCODER_H_
#define XMLRPCENCODER_H_

#include <memory>
#include <list>

#include "../Exception.h"
#include "RPCVariable.h"
#include "rapidxml_print.hpp"

using namespace rapidxml;

namespace RPC {

class XMLRPCEncoder
{
public:
	XMLRPCEncoder() {}
	virtual ~XMLRPCEncoder() {}

	std::string encodeResponse(std::shared_ptr<RPCVariable> variable);
	std::string encodeRequest(std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
	std::string encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
private:
	void encodeVariable(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable);
	void encodeStruct(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable);
	void encodeArray(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable);
};

} /* namespace RPC */
#endif /* XMLRPCENCODER_H_ */
