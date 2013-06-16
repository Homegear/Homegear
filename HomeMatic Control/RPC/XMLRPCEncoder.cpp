#include "XMLRPCEncoder.h"

namespace RPC
{
std::string XMLRPCEncoder::encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	xml_document<> doc;
	try
	{
		xml_node<> *node = doc.allocate_node(node_element, "methodCall");
		doc.append_node(node);
		xml_node<> *nameNode = doc.allocate_node(node_element, "methodName", methodName.c_str());
		node->append_node(nameNode);
		xml_node<> *paramsNode = doc.allocate_node(node_element, "params");
		node->append_node(paramsNode);

		for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
		{
			xml_node<> *paramNode = doc.allocate_node(node_element, "param");
			paramsNode->append_node(paramNode);
			encodeVariable(&doc, paramNode, *i);
		}

		std::string xml("<?xml version=\"1.0\"?>\n");
		print(std::back_inserter(xml), doc, 1);
		doc.clear();
		return xml;
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
    doc.clear();
    return "";
}

std::string XMLRPCEncoder::encodeRequest(std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	xml_document<> doc;
	try
	{
		xml_node<> *node = doc.allocate_node(node_element, "methodCall");
		doc.append_node(node);
		xml_node<> *nameNode = doc.allocate_node(node_element, "methodName", methodName.c_str());
		node->append_node(nameNode);
		xml_node<> *paramsNode = doc.allocate_node(node_element, "params");
		node->append_node(paramsNode);

		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
		{
			xml_node<> *paramNode = doc.allocate_node(node_element, "param");
			paramsNode->append_node(paramNode);
			encodeVariable(&doc, paramNode, *i);
		}

		std::string xml("<?xml version=\"1.0\"?>\n");
		print(std::back_inserter(xml), doc, 1);
		doc.clear();
		return xml;
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
    doc.clear();
    return "";
}

std::string XMLRPCEncoder::encodeResponse(std::shared_ptr<RPCVariable> variable)
{
	xml_document<> doc;
	try
	{
		xml_node<> *node = doc.allocate_node(node_element, "methodResponse");
		doc.append_node(node);
		if(variable->errorStruct)
		{
			xml_node<> *faultNode = doc.allocate_node(node_element, "fault");
			node->append_node(faultNode);
			encodeVariable(&doc, faultNode, variable);
		}
		else
		{
			xml_node<> *paramsNode = doc.allocate_node(node_element, "params");
			node->append_node(paramsNode);
			xml_node<> *paramNode = doc.allocate_node(node_element, "param");
			paramsNode->append_node(paramNode);
			encodeVariable(&doc, paramNode, variable);
		}
		std::string xml;
		print(std::back_inserter(xml), doc, 1);
		doc.clear();
		return xml;
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
    doc.clear();
    return "";
}

void XMLRPCEncoder::encodeVariable(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	xml_node<> *valueNode = doc->allocate_node(node_element, "value");
	node->append_node(valueNode);
	if(variable->type == RPCVariableType::rpcVoid)
	{
		//leave valueNode empty
	}
	else if(variable->type == RPCVariableType::rpcInteger)
	{
		xml_node<> *valueNode2 = doc->allocate_node(node_element, "i4", doc->allocate_string(std::to_string(variable->integerValue).c_str()));
		valueNode->append_node(valueNode2);
	}
	else if(variable->type == RPCVariableType::rpcFloat)
	{
		xml_node<> *valueNode2 = doc->allocate_node(node_element, "double", doc->allocate_string(std::to_string(variable->floatValue).c_str()));
		valueNode->append_node(valueNode2);
	}
	else if(variable->type == RPCVariableType::rpcBoolean)
	{
		xml_node<> *valueNode2 = doc->allocate_node(node_element, "boolean", doc->allocate_string(std::to_string(variable->booleanValue).c_str()));
		valueNode->append_node(valueNode2);
	}
	else if(variable->type == RPCVariableType::rpcString)
	{
		//Don't allocate string. It is unnecessary, because variable->stringvalue exists until the encoding is done.
		//xml_node<> *valueNode2 = doc->allocate_node(node_element, "string", variable->stringValue.c_str());
		//valueNode->append_node(valueNode2);
		//Some servers/clients don't understand strings in string tags - don't ask me why, so just print the value
		valueNode->value(variable->stringValue.c_str());
	}
	else if(variable->type == RPCVariableType::rpcBase64)
	{
		//Don't allocate string. It is unnecessary, because variable->stringvalue exists until the encoding is done.
		xml_node<> *valueNode2 = doc->allocate_node(node_element, "base64", variable->stringValue.c_str());
		valueNode->append_node(valueNode2);
	}
	else if(variable->type == RPCVariableType::rpcStruct)
	{
		encodeStruct(doc, valueNode, variable);
	}
	else if(variable->type == RPCVariableType::rpcArray)
	{
		encodeArray(doc, valueNode, variable);
	}
}

void XMLRPCEncoder::encodeStruct(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	xml_node<> *structNode = doc->allocate_node(node_element, "struct");
	node->append_node(structNode);

	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
	{
		xml_node<> *memberNode = doc->allocate_node(node_element, "member");
		structNode->append_node(memberNode);
		xml_node<> *nameNode = doc->allocate_node(node_element, "name", (*i)->name.c_str());
		memberNode->append_node(nameNode);
		encodeVariable(doc, memberNode, *i);
	}
}

void XMLRPCEncoder::encodeArray(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	xml_node<> *arrayNode = doc->allocate_node(node_element, "array");
	node->append_node(arrayNode);

	xml_node<> *dataNode = doc->allocate_node(node_element, "data");
	arrayNode->append_node(dataNode);

	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
	{
		encodeVariable(doc, dataNode, *i);
	}
}
} /* namespace RPC */
