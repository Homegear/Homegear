#include "XMLRPCDecoder.h"
#include "../HelperFunctions.h"

namespace RPC
{
std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> XMLRPCDecoder::decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName)
{
	xml_document<> doc;
	try
	{
		doc.parse<0>(&packet->at(0));
		xml_node<>* node = doc.first_node();
		if(node == nullptr || std::string(doc.first_node()->name()) != "methodCall")
		{
			doc.clear();
			return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. First root node has to be \"methodCall\".")});
		}
		xml_node<>* subNode = node->first_node("methodName");
		if(subNode == nullptr || std::string(subNode->name()) != "methodName")
		{
			doc.clear();
			return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Node \"methodName\" not found.")});
		}
		methodName = std::string(subNode->value());
		if(methodName.empty())
		{
			doc.clear();
			return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. \"methodName\" is empty.")});
		}

		subNode = node->first_node("params");
		if(subNode == nullptr)
		{
			doc.clear();
			return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Node \"params\" not found.")});
		}

		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters(new std::vector<std::shared_ptr<RPCVariable>>());
		for(xml_node<>* paramNode = subNode->first_node(); paramNode; paramNode = paramNode->next_sibling())
		{
			xml_node<>* valueNode = paramNode->first_node("value");
			if(valueNode == nullptr) continue;
			parameters->push_back(decodeParameter(valueNode));
		}

		doc.clear();
		return parameters;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what()))});
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what()))});
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
    	return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed.")});
    }
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed.")});
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeResponse(std::string& packet)
{
	xml_document<> doc;
	try
	{
		doc.parse<0>((char*)packet.c_str());
		std::shared_ptr<RPCVariable> response = decodeResponse(&doc);
		doc.clear();
		return response;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
    }
    return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeResponse(std::shared_ptr<std::vector<char>> packet)
{
	xml_document<> doc;
	try
	{
		doc.parse<0>(&packet->at(0));
		std::shared_ptr<RPCVariable> response = decodeResponse(&doc);
		doc.clear();
		return response;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
    }
    return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeResponse(xml_document<>* doc)
{
	try
	{
		xml_node<>* node = doc->first_node();
		if(node == nullptr || std::string(doc->first_node()->name()) != "methodResponse")
		{
			doc->clear();
			return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. First root node has to be \"methodResponse\"."));
		}

		xml_node<>* subNode = node->first_node("params");
		if(subNode == nullptr)
		{
			doc->clear();
			return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Node \"params\" not found."));
		}

		subNode = subNode->first_node("param");
		if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));

		subNode = subNode->first_node("value");
		if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));

		return decodeParameter(subNode);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
    }
    return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeParameter(xml_node<>* valueNode)
{
	try
	{
		if(valueNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
		xml_node<>* subNode = valueNode->first_node();
		if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));

		std::string type(subNode->name());
		HelperFunctions::toLower(type);
		std::string value(subNode->value());
		if(type == "string")
		{
			return std::shared_ptr<RPCVariable>(new RPCVariable(value));
		}
		else if(type == "boolean")
		{
			bool boolean = false;
			if(value == "true" || value == "1") boolean = true;
			return std::shared_ptr<RPCVariable>(new RPCVariable(boolean));
		}
		else if(type == "i4" || type == "int")
		{
			return std::shared_ptr<RPCVariable>(new RPCVariable(HelperFunctions::getNumber(value)));
		}
		else if(type == "double")
		{
			double number = 0;
			try { number = std::stod(value); } catch(...) {}
			return std::shared_ptr<RPCVariable>(new RPCVariable(number));
		}
		else if(type == "base64")
		{
			std::shared_ptr<RPCVariable> base64(new RPCVariable(RPCVariableType::rpcBase64));
			base64->stringValue = value;
			return base64;
		}
		else if(type == "array")
		{
			return decodeArray(subNode);
		}
		else if(type == "struct")
		{
			return decodeStruct(subNode);
		}
		else if(type == "nil" || type == "ex:nil")
		{
			return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
		}
		return std::shared_ptr<RPCVariable>(new RPCVariable(value)); //if no type is specified return string
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCVariable>(new RPCVariable(0));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeStruct(xml_node<>* structNode)
{
	std::shared_ptr<RPCVariable> rpcStruct(new RPCVariable(RPCVariableType::rpcStruct));
	try
	{
		if(structNode == nullptr) return rpcStruct;

		for(xml_node<>* memberNode = structNode->first_node(); memberNode; memberNode = memberNode->next_sibling())
		{
			xml_node<>* subNode = memberNode->first_node("name");
			if(subNode == nullptr) continue;
			std::string name(subNode->value());
			if(name.empty()) continue;
			subNode = subNode->next_sibling("value");
			if(subNode == nullptr) continue;
			std::shared_ptr<RPCVariable> element = decodeParameter(subNode);
			element->name = name;
			rpcStruct->structValue->push_back(element);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return rpcStruct;
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeArray(xml_node<>* arrayNode)
{
	std::shared_ptr<RPCVariable> rpcArray(new RPCVariable(RPCVariableType::rpcArray));
	try
	{
		if(arrayNode == nullptr) return rpcArray;

		xml_node<>* dataNode = arrayNode->first_node("data");
		if(dataNode == nullptr) return rpcArray;

		for(xml_node<>* valueNode = dataNode->first_node(); valueNode; valueNode = valueNode->next_sibling())
		{
			rpcArray->arrayValue->push_back(decodeParameter(valueNode));
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return rpcArray;
}

} /* namespace RPC */
