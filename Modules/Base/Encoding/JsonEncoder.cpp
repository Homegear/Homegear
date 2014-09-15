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

void JsonEncoder::encode(const std::shared_ptr<Variable> variable, std::string& json)
{
	if(!variable) return;
	std::ostringstream s;
	switch(variable->type)
	{
	case VariableType::rpcStruct:
		encodeStruct(variable, s);
		break;
	case VariableType::rpcArray:
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

void JsonEncoder::encodeValue(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	switch(variable->type)
	{
	case VariableType::rpcArray:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON array.");
		encodeArray(variable, s);
		break;
	case VariableType::rpcStruct:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON struct.");
		encodeStruct(variable, s);
		break;
	case VariableType::rpcBoolean:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON boolean.");
		encodeBoolean(variable, s);
		break;
	case VariableType::rpcInteger:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON integer.");
		encodeInteger(variable, s);
		break;
	case VariableType::rpcFloat:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON float.");
		encodeFloat(variable, s);
		break;
	case VariableType::rpcBase64:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::rpcString:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON string.");
		encodeString(variable, s);
		break;
	case VariableType::rpcVoid:
		if(_bl->debugLevel >= 5) _bl->out.printDebug("Encoding JSON null.");
		encodeVoid(variable, s);
		break;
	case VariableType::rpcVariant:
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

void JsonEncoder::encodeBoolean(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << ((variable->booleanValue) ? "true" : "false");
}

void JsonEncoder::encodeInteger(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << std::to_string(variable->integerValue);
}

void JsonEncoder::encodeFloat(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << std::fixed << std::setprecision(15) << variable->floatValue;
}

void JsonEncoder::encodeString(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << variable->stringValue;
}

void JsonEncoder::encodeVoid(const std::shared_ptr<Variable>& variable, std::ostringstream& s)
{
	s << "null";
}

}
}
