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

#include "XMLRPCEncoder.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

XMLRPCEncoder::XMLRPCEncoder()
{
}

std::shared_ptr<std::vector<char>> XMLRPCEncoder::encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters)
{
	std::shared_ptr<std::vector<char>> xml(new std::vector<char>());
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

		std::string temp("<?xml version=\"1.0\"?>\n");
		xml->insert(xml->end(), temp.begin(), temp.end());
		print(std::back_inserter(*xml), doc, 1);
		doc.clear();
		return xml;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    doc.clear();
    return xml;
}

std::shared_ptr<std::vector<char>> XMLRPCEncoder::encodeRequest(std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	xml_document<> doc;
	std::shared_ptr<std::vector<char>> xml(new std::vector<char>());
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
		std::string temp("<?xml version=\"1.0\"?>\n");
		xml->insert(xml->end(), temp.begin(), temp.end());
		print(std::back_inserter(*xml), doc, 1);
		doc.clear();
		return xml;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    doc.clear();
    return xml;
}

std::shared_ptr<std::vector<char>> XMLRPCEncoder::encodeResponse(std::shared_ptr<RPCVariable> variable)
{
	std::shared_ptr<std::vector<char>> xml(new std::vector<char>());
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
		print(std::back_inserter(*xml), doc, 1);
		doc.clear();
		return xml;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    doc.clear();
    return xml;
}

void XMLRPCEncoder::encodeVariable(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	try
	{
		xml_node<> *valueNode = doc->allocate_node(node_element, "value");
		node->append_node(valueNode);
		if(!variable || variable->type == RPCVariableType::rpcVoid)
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
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void XMLRPCEncoder::encodeStruct(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	try
	{
		xml_node<> *structNode = doc->allocate_node(node_element, "struct");
		node->append_node(structNode);

		for(RPCStruct::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			if(i->first.empty() || !i->second) continue;
			xml_node<> *memberNode = doc->allocate_node(node_element, "member");
			structNode->append_node(memberNode);
			xml_node<> *nameNode = doc->allocate_node(node_element, "name", i->first.c_str());
			memberNode->append_node(nameNode);
			encodeVariable(doc, memberNode, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void XMLRPCEncoder::encodeArray(xml_document<>* doc, xml_node<>* node, std::shared_ptr<RPCVariable> variable)
{
	try
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
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
}
