/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "XMLRPCDecoder.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

XMLRPCDecoder::XMLRPCDecoder(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what()))});
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>{RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what()))});
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
    }
    return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeResponse(std::shared_ptr<std::vector<char>> packet)
{
	xml_document<> doc;
	try
	{
		if(!packet) return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32500, "packet is nullptr."));
		int32_t startPos = 0;
		if(packet->front() != '<')
		{
			for(int32_t i = 0; i < (signed)packet->size(); i++)
			{
				if(packet->operator [](i) == '<')
				{
					startPos = i;
					break;
				}
			}
		}
		if(startPos >= (signed)packet->size()) return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: Could not find \"<\"."));
		doc.parse<0>(&packet->at(startPos));
		std::shared_ptr<RPCVariable> response = decodeResponse(&doc);
		doc.clear();
		return response;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	doc.clear();
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    	doc.clear();
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

		bool errorStruct = false;
		xml_node<>* subNode = node->first_node("params");
		if(subNode == nullptr)
		{
			subNode = node->first_node("fault");
			if(subNode == nullptr)
			{
				doc->clear();
				return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Node \"fault\" and \"params\" not found."));
			}
			errorStruct = true;
		}
		else
		{
			subNode = subNode->first_node("param");
			if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
		}

		subNode = subNode->first_node("value");
		if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));

		std::shared_ptr<RPCVariable> response = decodeParameter(subNode);
		if(errorStruct)
		{
			response->errorStruct = errorStruct;
			if(response->structValue->find("faultCode") == response->structValue->end()) response->structValue->insert(RPC::RPCStructElement("faultCode", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(-1))));
			if(response->structValue->find("faultString") == response->structValue->end()) response->structValue->insert(RPC::RPCStructElement("faultString", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("undefined")))));
		}
		return response;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed: " + std::string(ex.what())));
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCVariable>(RPCVariable::createError(-32700, "Parse error. Not well formed."));
}

std::shared_ptr<RPCVariable> XMLRPCDecoder::decodeParameter(xml_node<>* valueNode)
{
	try
	{
		if(valueNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
		xml_node<>* subNode = valueNode->first_node();
		if(subNode == nullptr) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcString));

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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			rpcStruct->structValue->insert(RPCStructElement(name, element));
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return rpcArray;
}

}
}
