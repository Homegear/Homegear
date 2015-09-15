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

#include "JsonEncoder.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

JsonEncoder::JsonEncoder(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void JsonEncoder::encodeRequest(std::string& methodName, std::shared_ptr<std::list<std::shared_ptr<Variable>>>& parameters, std::vector<char>& encodedData)
{
	try
	{
		std::shared_ptr<Variable> methodCall(new Variable(VariableType::tStruct));
		methodCall->structValue->insert(StructElement("jsonrpc", std::shared_ptr<Variable>(new Variable(std::string("2.0")))));
		methodCall->structValue->insert(StructElement("method", std::shared_ptr<Variable>(new Variable(methodName))));
		std::shared_ptr<Variable> params(new Variable(VariableType::tArray));
		for(std::list<std::shared_ptr<Variable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
		{
			params->arrayValue->push_back(*i);
		}
		methodCall->structValue->insert(StructElement("params", params));
		methodCall->structValue->insert(StructElement("id", std::shared_ptr<Variable>(new Variable(_requestId++))));
		encode(methodCall, encodedData);
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
}

void JsonEncoder::encodeResponse(const std::shared_ptr<Variable>& variable, int32_t id, std::vector<char>& json)
{
	try
	{
		std::shared_ptr<Variable> response(new Variable(VariableType::tStruct));
		response->structValue->insert(StructElement("jsonrpc", std::shared_ptr<Variable>(new Variable(std::string("2.0")))));
		if(variable->errorStruct)
		{
			std::shared_ptr<Variable> error(new Variable(VariableType::tStruct));
			error->structValue->insert(StructElement("code", variable->structValue->at("faultCode")));
			error->structValue->insert(StructElement("message", variable->structValue->at("faultString")));
			response->structValue->insert(StructElement("error", error));
		}
		else response->structValue->insert(StructElement("result", variable));
		response->structValue->insert(StructElement("id", std::shared_ptr<Variable>(new Variable(id))));
		encode(response, json);
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
}

void JsonEncoder::encodeMQTTResponse(const std::string methodName, const std::shared_ptr<Variable>& variable, int32_t id, std::vector<char>& json)
{
	try
	{
		std::shared_ptr<Variable> response(new Variable(VariableType::tStruct));
		response->structValue->insert(StructElement("method", std::shared_ptr<Variable>(new Variable(methodName))));
		if(variable->errorStruct)
		{
			std::shared_ptr<Variable> error(new Variable(VariableType::tStruct));
			error->structValue->insert(StructElement("code", variable->structValue->at("faultCode")));
			error->structValue->insert(StructElement("message", variable->structValue->at("faultString")));
			response->structValue->insert(StructElement("error", error));
		}
		else response->structValue->insert(StructElement("result", variable));
		response->structValue->insert(StructElement("id", std::shared_ptr<Variable>(new Variable(id))));
		encode(response, json);
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
}

void JsonEncoder::encode(const std::shared_ptr<Variable> variable, std::string& json)
{
	if(!variable) return;
	std::ostringstream s;
	switch(variable->type)
	{
	case VariableType::tStruct:
		encodeStruct(variable, s);
		break;
	case VariableType::tArray:
		encodeArray(variable, s);
		break;
	default:
		s << '[';
		encodeValue(variable, s);
		s << ']';
		break;
	}
	json = s.str();
}

void JsonEncoder::encode(const std::shared_ptr<Variable> variable, std::vector<char>& json)
{
	if(!variable) return;
	json.clear();
	json.reserve(1024);
	switch(variable->type)
	{
	case VariableType::tStruct:
		encodeStruct(variable, json);
		break;
	case VariableType::tArray:
		encodeArray(variable, json);
		break;
	default:
		json.push_back('[');
		encodeValue(variable, json);
		json.push_back(']');
		break;
	}
}

void JsonEncoder::encodeValue(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	switch(variable->type)
	{
	case VariableType::tArray:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON array.");
		encodeArray(variable, s);
		break;
	case VariableType::tStruct:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON struct.");
		encodeStruct(variable, s);
		break;
	case VariableType::tBoolean:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON boolean.");
		encodeBoolean(variable, s);
		break;
	case VariableType::tInteger:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON integer.");
		encodeInteger(variable, s);
		break;
	case VariableType::tFloat:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON float.");
		encodeFloat(variable, s);
		break;
	case VariableType::tBase64:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::tString:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::tVoid:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	case VariableType::tVariant:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	case VariableType::tBinary:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	}
}

void JsonEncoder::encodeValue(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	if(s.size() + 128 > s.capacity()) s.reserve(s.capacity() + 1024);
	switch(variable->type)
	{
	case VariableType::tArray:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON array.");
		encodeArray(variable, s);
		break;
	case VariableType::tStruct:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON struct.");
		encodeStruct(variable, s);
		break;
	case VariableType::tBoolean:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON boolean.");
		encodeBoolean(variable, s);
		break;
	case VariableType::tInteger:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON integer.");
		encodeInteger(variable, s);
		break;
	case VariableType::tFloat:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON float.");
		encodeFloat(variable, s);
		break;
	case VariableType::tBase64:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::tString:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::tVoid:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	case VariableType::tVariant:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	case VariableType::tBinary:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	}
}

void JsonEncoder::encodeArray(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << '[';
	if(!variable->arrayValue->empty())
	{
		encodeValue(variable->arrayValue->at(0), s);
		for(std::vector<std::shared_ptr<Variable>>::iterator i = ++variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
		{
			s << ',';
			encodeValue(*i, s);
		}
	}
	s << ']';
}

void JsonEncoder::encodeArray(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	s.push_back('[');
	if(!variable->arrayValue->empty())
	{
		encodeValue(variable->arrayValue->at(0), s);
		for(std::vector<std::shared_ptr<Variable>>::iterator i = ++variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
		{
			s.push_back(',');
			encodeValue(*i, s);
		}
	}
	s.push_back(']');
}

void JsonEncoder::encodeStruct(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << '{';
	if(!variable->structValue->empty())
	{
		s << '"';
		s << variable->structValue->begin()->first;
		s << "\":";
		encodeValue(variable->structValue->begin()->second, s);
		for(std::map<std::string, std::shared_ptr<Variable>>::iterator i = ++variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			s << ',';
			s << '"';
			s << i->first;
			s << "\":";
			encodeValue(i->second, s);
		}
	}
	s << '}';
}

void JsonEncoder::encodeStruct(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	s.push_back('{');
	if(!variable->structValue->empty())
	{
		s.push_back('"');
		s.insert(s.end(), variable->structValue->begin()->first.begin(), variable->structValue->begin()->first.end());
		s.push_back('"');
		s.push_back(':');
		encodeValue(variable->structValue->begin()->second, s);
		for(std::map<std::string, std::shared_ptr<Variable>>::iterator i = ++variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			s.push_back(',');
			s.push_back('"');
			s.insert(s.end(), i->first.begin(), i->first.end());
			s.push_back('"');
			s.push_back(':');
			encodeValue(i->second, s);
		}
	}
	s.push_back('}');
}

void JsonEncoder::encodeBoolean(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << ((variable->booleanValue) ? "true" : "false");
}

void JsonEncoder::encodeBoolean(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	if(variable->booleanValue)
	{
		s.push_back('t');
		s.push_back('r');
		s.push_back('u');
		s.push_back('e');
	}
	else
	{
		s.push_back('f');
		s.push_back('a');
		s.push_back('l');
		s.push_back('s');
		s.push_back('e');
	}
}

void JsonEncoder::encodeInteger(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << std::to_string(variable->integerValue);
}

void JsonEncoder::encodeInteger(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	std::string value(std::to_string(variable->integerValue));
	s.insert(s.end(), value.begin(), value.end());
}

void JsonEncoder::encodeFloat(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << std::fixed << std::setprecision(15) << variable->floatValue;
}

void JsonEncoder::encodeFloat(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	std::ostringstream os;
	os << std::fixed << std::setprecision(15) << variable->floatValue;
	std::string value(os.str());
	s.insert(s.end(), value.begin(), value.end());
}

void JsonEncoder::encodeString(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	//Source: https://github.com/miloyip/rapidjson/blob/master/include/rapidjson/writer.h
	static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	static const char escape[256] =
	{
		#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		//0 1 2 3 4 5 6 7 8 9 A B C D E F
		'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
		'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
		0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
		Z16, Z16, // 30~4F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0, // 50
		Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16 // 60~FF
		#undef Z16
	};
	s << "\"";
	for(std::string::iterator i = variable->stringValue.begin(); i != variable->stringValue.end(); ++i)
	{
		if(escape[(uint8_t)*i])
		{
			s << '\\' << escape[(uint8_t)*i];
			if (escape[(uint8_t)*i] == 'u')
			{
				s << '0' << '0' << hexDigits[((uint8_t)*i) >> 4] << hexDigits[((uint8_t)*i) & 0xF];
			}
		}
		else s << *i;
	}
	s << "\"";
}

void JsonEncoder::encodeString(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	if(s.size() + variable->stringValue.size() + 128 > s.capacity())
	{
		int32_t factor = variable->stringValue.size() / 1024;
		s.reserve(s.capacity() + (factor * 1024) + 1024);
	}
	//Source: https://github.com/miloyip/rapidjson/blob/master/include/rapidjson/writer.h
	static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	static const char escape[256] =
	{
		#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		//0 1 2 3 4 5 6 7 8 9 A B C D E F
		'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
		'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
		0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
		Z16, Z16, // 30~4F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0, // 50
		Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16 // 60~FF
		#undef Z16
	};
	s.push_back('"');
	for(std::string::iterator i = variable->stringValue.begin(); i != variable->stringValue.end(); ++i)
	{
		if(escape[(uint8_t)*i])
		{
			s.push_back('\\');
			s.push_back(escape[(uint8_t)*i]);
			if (escape[(uint8_t)*i] == 'u')
			{
				s.push_back('0');
				s.push_back('0');
				s.push_back(hexDigits[((uint8_t)*i) >> 4]);
				s.push_back(hexDigits[((uint8_t)*i) & 0xF]);
			}
		}
		else s.push_back(*i);
	}
	s.push_back('"');
}

void JsonEncoder::encodeVoid(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << "null";
}

void JsonEncoder::encodeVoid(const std::shared_ptr<Variable>& variable, std::vector<char>& s)
{
	s.push_back('n');
	s.push_back('u');
	s.push_back('l');
	s.push_back('l');
}

}
}
