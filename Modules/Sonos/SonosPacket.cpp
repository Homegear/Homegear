/* Copyright 2013-2015 Sathya Laufer
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

#include "SonosPacket.h"
#include "GD.h"
#include "../Base/Encoding/RapidXml/rapidxml.hpp"

namespace Sonos
{
SonosPacket::SonosPacket()
{
	_values.reset(new std::map<std::string, std::string>());
}

SonosPacket::SonosPacket(std::string soap)
{
	try
	{
		if(soap.empty()) return;
		xml_document<> doc;
		doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(&soap.at(0));
		xml_node<>* node = doc.first_node("s:Envelope");
		if(!node) return;
		node = node->first_node("s:Body");
		if(!node) return;
		node = node->first_node();
		if(!node) return;
		_functionName = std::string(node->name());
		if(_functionName.size() > 2) _functionName = _functionName.substr(2);
		_values.reset(new std::map<std::string, std::string>());
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			_values->operator [](std::string(subNode->name())) = std::string(subNode->value());
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

SonosPacket::SonosPacket(std::string& ip, std::string& path, std::string& soapAction, std::string& schema, std::string& functionName, std::shared_ptr<std::map<std::string, std::string>>& values)
{
	_ip = ip;
	_path = path;
	_soapAction = soapAction;
	_schema = schema;
	_functionName = functionName;
	_values = values;
	if(!_values) _values.reset(new std::map<std::string, std::string>());
}

SonosPacket::~SonosPacket()
{
}

void SonosPacket::getSoapRequest(std::string& request)
{
	try
	{
		request = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" + _functionName + " xmlns:u=\"" + _schema + "\">";
		for(std::map<std::string, std::string>::iterator i = _values->begin(); i != _values->end(); ++i)
		{
			request += '<' + i->first + '>' + i->second + "</" + i->first + '>';
		}
		request += "</u:" + _functionName + "></s:Body></s:Envelope>";

		std::string header = "POST " + _path + " HTTP/1.1\r\nCONNECTION: close\r\nHOST: " + _ip + ":1400\r\nCONTENT-LENGTH: " + std::to_string(request.size()) + "\r\nCONTENT-TYPE: text/xml; charset=\"utf-8\"\r\nSOAPACTION: \"" + _soapAction + "\"\r\n\r\n";
		request.insert(request.begin(), header.begin(), header.end());
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
