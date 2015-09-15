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

#include "JsonDecoder.h"
#include "../HelperFunctions/Math.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

JsonDecoder::JsonDecoder(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

std::shared_ptr<Variable> JsonDecoder::decode(const std::string& json)
{
	uint32_t pos = 0;
	std::shared_ptr<Variable> variable(new Variable());
	skipWhitespace(json, pos);
	if(!posValid(json, pos)) return variable;

	while(pos < json.length())
	{
		switch(json[pos])
		{
		case '{':
			decodeObject(json, pos, variable);
			return variable;
		case '[':
			decodeArray(json, pos, variable);
			return variable;
		default:
			throw JsonDecoderException("JSON does not start with '{' or '['.");
		}
		pos++;
	}

	return variable;
}

std::shared_ptr<Variable> JsonDecoder::decode(const std::vector<char>& json)
{
	uint32_t pos = 0;
	std::shared_ptr<Variable> variable(new Variable());
	skipWhitespace(json, pos);
	if(!posValid(json, pos)) return variable;

	while(pos < json.size())
	{
		switch(json[pos])
		{
		case '{':
			decodeObject(json, pos, variable);
			return variable;
		case '[':
			decodeArray(json, pos, variable);
			return variable;
		default:
			throw JsonDecoderException("JSON does not start with '{' or '['.");
		}
		pos++;
	}

	return variable;
}

bool JsonDecoder::posValid(const std::string& json, uint32_t& pos)
{
	return pos < json.length();
}

bool JsonDecoder::posValid(const std::vector<char>& json, uint32_t& pos)
{
	return pos < json.size();
}

void JsonDecoder::skipWhitespace(const std::string& json, uint32_t& pos)
{
	while(pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t'))
	{
		pos++;
	}
}

void JsonDecoder::skipWhitespace(const std::vector<char>& json, uint32_t& pos)
{
	while(pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t'))
	{
		pos++;
	}
}

void JsonDecoder::decodeObject(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& variable)
{
	variable->type = VariableType::tStruct;
	if(!posValid(json, pos)) return;
	if(json[pos] == '{')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
	}
	skipWhitespace(json, pos);
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
	if(json[pos] == '}')
	{
		pos++;
		return; //Empty object
	}

	while(pos < json.length())
	{
		if(json[pos] != '"') throw JsonDecoderException("Object element has no name.");
		std::string name;
		decodeString(json, pos, name);
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		if(json[pos] != ':')
		{
			variable->structValue->insert(StructElement(name, std::shared_ptr<Variable>(new Variable(VariableType::tVoid))));
			if(json[pos] == ',')
			{
				pos++;
				skipWhitespace(json, pos);
				if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
				continue;
			}
			if(json[pos] == '}')
			{
				pos++;
				return;
			}
			throw JsonDecoderException("Invalid data after object name.");
		}
		pos++;
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		std::shared_ptr<Variable> element(new Variable(VariableType::tVoid));
		decodeValue(json, pos,element);
		variable->structValue->insert(StructElement(name, element));
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		if(json[pos] == ',')
		{
			pos++;
			skipWhitespace(json, pos);
			if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
			continue;
		}
		if(json[pos] == '}')
		{
			pos++;
			return;
		}
		throw JsonDecoderException("No closing '}' found.");
	}
}

void JsonDecoder::decodeObject(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& variable)
{
	variable->type = VariableType::tStruct;
	if(!posValid(json, pos)) return;
	if(json[pos] == '{')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
	}
	skipWhitespace(json, pos);
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
	if(json[pos] == '}')
	{
		pos++;
		return; //Empty object
	}

	while(pos < json.size())
	{
		if(json[pos] != '"') throw JsonDecoderException("Object element has no name.");
		std::string name;
		decodeString(json, pos, name);
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		if(json[pos] != ':')
		{
			variable->structValue->insert(StructElement(name, std::shared_ptr<Variable>(new Variable(VariableType::tVoid))));
			if(json[pos] == ',')
			{
				pos++;
				skipWhitespace(json, pos);
				if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
				continue;
			}
			if(json[pos] == '}')
			{
				pos++;
				return;
			}
			throw JsonDecoderException("Invalid data after object name.");
		}
		pos++;
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		std::shared_ptr<Variable> element(new Variable(VariableType::tVoid));
		decodeValue(json, pos,element);
		variable->structValue->insert(StructElement(name, element));
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
		if(json[pos] == ',')
		{
			pos++;
			skipWhitespace(json, pos);
			if(!posValid(json, pos)) throw JsonDecoderException("No closing '}' found.");
			continue;
		}
		if(json[pos] == '}')
		{
			pos++;
			return;
		}
		throw JsonDecoderException("No closing '}' found.");
	}
}

void JsonDecoder::decodeArray(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& variable)
{
	variable->type = VariableType::tArray;
	if(!posValid(json, pos)) return;
	if(json[pos] == '[')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
	}

	skipWhitespace(json, pos);
	if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
	if(json[pos] == ']')
	{
		pos++;
		return; //Empty array
	}

	while(pos < json.length())
	{
		std::shared_ptr<Variable> element(new Variable(VariableType::tVoid));
		decodeValue(json, pos, element);
		variable->arrayValue->push_back(element);
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
		if(json[pos] == ',')
		{
			pos++;
			skipWhitespace(json, pos);
			if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
			continue;
		}
		if(json[pos] == ']')
		{
			pos++;
			return;
		}
		throw JsonDecoderException("No closing ']' found.");
	}
}

void JsonDecoder::decodeArray(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& variable)
{
	variable->type = VariableType::tArray;
	if(!posValid(json, pos)) return;
	if(json[pos] == '[')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
	}

	skipWhitespace(json, pos);
	if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
	if(json[pos] == ']')
	{
		pos++;
		return; //Empty array
	}

	while(pos < json.size())
	{
		std::shared_ptr<Variable> element(new Variable(VariableType::tVoid));
		decodeValue(json, pos, element);
		variable->arrayValue->push_back(element);
		skipWhitespace(json, pos);
		if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
		if(json[pos] == ',')
		{
			pos++;
			skipWhitespace(json, pos);
			if(!posValid(json, pos)) throw JsonDecoderException("No closing ']' found.");
			continue;
		}
		if(json[pos] == ']')
		{
			pos++;
			return;
		}
		throw JsonDecoderException("No closing ']' found.");
	}
}

void JsonDecoder::decodeString(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tString;
	std::string s;
	decodeString(json, pos, value->stringValue);
}

void JsonDecoder::decodeString(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tString;
	std::string s;
	decodeString(json, pos, value->stringValue);
}

void JsonDecoder::decodeString(const std::string& json, uint32_t& pos, std::string& s)
{
	s.clear();
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	if(json[pos] == '"')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	}
	while(pos < json.length())
	{
		char c = json[pos];
		if(c == '\\')
		{
			pos++;
			if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
			s.push_back(json[pos]);
		}
		else if(c == '"')
		{
			pos++;
			return;
		}
		else if((unsigned)c < 0x20) throw JsonDecoderException("Invalid character in string.");
		else s.push_back(json[pos]);
		pos++;
	}
	throw JsonDecoderException("No closing '\"' found.");
}

void JsonDecoder::decodeString(const std::vector<char>& json, uint32_t& pos, std::string& s)
{
	s.clear();
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	if(json[pos] == '"')
	{
		pos++;
		if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	}
	while(pos < json.size())
	{
		char c = json[pos];
		if(c == '\\')
		{
			pos++;
			if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
			s.push_back(json[pos]);
		}
		else if(c == '"')
		{
			pos++;
			return;
		}
		else if((unsigned)c < 0x20) throw JsonDecoderException("Invalid character in string.");
		else s.push_back(json[pos]);
		pos++;
	}
	throw JsonDecoderException("No closing '\"' found.");
}

void JsonDecoder::decodeValue(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	switch (json[pos]) {
		case 'n':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON null.");
			decodeNull(json, pos, value);
			break;
		case 't':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON boolean.");
			decodeBoolean(json, pos, value);
			break;
		case 'f':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON boolean.");
			decodeBoolean(json, pos, value);
			break;
		case '"':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON string.");
			decodeString(json, pos, value);
			break;
		case '{':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON object.");
			decodeObject(json, pos, value);
			break;
		case '[':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON array.");
			decodeArray(json, pos, value);
			break;
		default:
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON number.");
			decodeNumber(json, pos, value);
			break;
	}
}

void JsonDecoder::decodeValue(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	if(!posValid(json, pos)) throw JsonDecoderException("No closing '\"' found.");
	switch (json[pos]) {
		case 'n':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON null.");
			decodeNull(json, pos, value);
			break;
		case 't':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON boolean.");
			decodeBoolean(json, pos, value);
			break;
		case 'f':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON boolean.");
			decodeBoolean(json, pos, value);
			break;
		case '"':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON string.");
			decodeString(json, pos, value);
			break;
		case '{':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON object.");
			decodeObject(json, pos, value);
			break;
		case '[':
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON array.");
			decodeArray(json, pos, value);
			break;
		default:
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Decoding JSON number.");
			decodeNumber(json, pos, value);
			break;
	}
}

void JsonDecoder::decodeBoolean(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tBoolean;
	if(!posValid(json, pos)) return;
	if(json[pos] == 't')
	{
		value->booleanValue = true;
		pos += 4;
	}
	else
	{
		value->booleanValue = false;
		pos += 5;
	}
}

void JsonDecoder::decodeBoolean(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tBoolean;
	if(!posValid(json, pos)) return;
	if(json[pos] == 't')
	{
		value->booleanValue = true;
		pos += 4;
	}
	else
	{
		value->booleanValue = false;
		pos += 5;
	}
}

void JsonDecoder::decodeNull(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tVoid;
	pos += 4;
}

void JsonDecoder::decodeNull(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tVoid;
	pos += 4;
}

void JsonDecoder::decodeNumber(const std::string& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tInteger;
	if(!posValid(json, pos)) return;
	bool minus = false;
	if(json[pos] == '-')
	{
		minus = true;
		pos++;
		if(!posValid(json, pos)) return;
	}
	else if(json[pos] == '+')
	{
		pos++;
		if(!posValid(json, pos)) return;
	}

	bool isDouble = false;
	uint64_t number = 0;
	if(json[pos] == '0')
	{
		number = 0;
		pos++;
		if(!posValid(json, pos)) return;
	}
	else if(json[pos] >= '1' && json[pos] <= '9')
	{
		while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9')
		{
			if (number >= 214748364)
			{
				value->type = VariableType::tFloat;
				isDouble = true;
				value->floatValue = number;
				break;
			}
			number = number * 10 + (json[pos] - '0');
			pos++;
		}
	}
	else throw JsonDecoderException("Tried to decode invalid number.");

	if(isDouble)
	{
		while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9')
		{
			value->floatValue = value->floatValue * 10 + (json[pos] - '0');
			pos++;
		}
	}

	if(!posValid(json, pos)) return;
	int32_t exponent = 0;
	if(json[pos] == '.')
	{
		if(!isDouble)
		{
			value->type = VariableType::tFloat;
			isDouble = true;
			value->floatValue = number;
		}
		pos++;
		while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9')
		{
			value->floatValue = value->floatValue * 10 + (json[pos] - '0');
			pos++;
			exponent--;
		}
	}

	if(!posValid(json, pos)) return;
	int32_t exponent2 = 0;
	if(json[pos] == 'e' || json[pos] == 'E')
	{
		pos++;
		if(!posValid(json, pos)) return;

		bool negative = false;
		if(json[pos] == '-')
		{
			negative = true;
			pos++;
			if(!posValid(json, pos)) return;
		}
		else if(json[pos] == '+')
		{
			pos++;
			if(!posValid(json, pos)) return;
		}
		if (json[pos] >= '0' && json[pos] <= '9')
		{
			exponent2 = json[pos] - '0';
			pos++;
			if(!posValid(json, pos)) return;
			while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9')
			{
				exponent2 = exponent2 * 10 + (json[pos] - '0');
			}
		}
		if(negative) exponent2 *= -1;
	}

	if(isDouble)
	{
		exponent += exponent2;
		if(exponent < -308) exponent = -308;
		else if(exponent > 308) exponent = 308;
		value->floatValue = (exponent >= 0) ? value->floatValue * Math::Pow10(exponent) : value->floatValue / Math::Pow10(-exponent);
		if(minus) value->floatValue *= -1;
		value->integerValue = std::lround(value->floatValue);
	}
	else
	{
		value->integerValue = minus ? -number : number;
		value->floatValue = value->integerValue;
	}
}

void JsonDecoder::decodeNumber(const std::vector<char>& json, uint32_t& pos, std::shared_ptr<Variable>& value)
{
	value->type = VariableType::tInteger;
	if(!posValid(json, pos)) return;
	bool minus = false;
	if(json[pos] == '-')
	{
		minus = true;
		pos++;
		if(!posValid(json, pos)) return;
	}
	else if(json[pos] == '+')
	{
		pos++;
		if(!posValid(json, pos)) return;
	}

	bool isDouble = false;
	uint64_t number = 0;
	if(json[pos] == '0')
	{
		number = 0;
		pos++;
		if(!posValid(json, pos)) return;
	}
	else if(json[pos] >= '1' && json[pos] <= '9')
	{
		while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
		{
			if (number >= 214748364)
			{
				value->type = VariableType::tFloat;
				isDouble = true;
				value->floatValue = number;
				break;
			}
			number = number * 10 + (json[pos] - '0');
			pos++;
		}
	}
	else throw JsonDecoderException("Tried to decode invalid number.");

	if(isDouble)
	{
		while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
		{
			value->floatValue = value->floatValue * 10 + (json[pos] - '0');
			pos++;
		}
	}

	if(!posValid(json, pos)) return;
	int32_t exponent = 0;
	if(json[pos] == '.')
	{
		if(!isDouble)
		{
			value->type = VariableType::tFloat;
			isDouble = true;
			value->floatValue = number;
		}
		pos++;
		while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
		{
			value->floatValue = value->floatValue * 10 + (json[pos] - '0');
			pos++;
			exponent--;
		}
	}

	if(!posValid(json, pos)) return;
	int32_t exponent2 = 0;
	if(json[pos] == 'e' || json[pos] == 'E')
	{
		pos++;
		if(!posValid(json, pos)) return;

		bool negative = false;
		if(json[pos] == '-')
		{
			negative = true;
			pos++;
			if(!posValid(json, pos)) return;
		}
		else if(json[pos] == '+')
		{
			pos++;
			if(!posValid(json, pos)) return;
		}
		if (json[pos] >= '0' && json[pos] <= '9')
		{
			exponent2 = json[pos] - '0';
			pos++;
			if(!posValid(json, pos)) return;
			while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
			{
				exponent2 = exponent2 * 10 + (json[pos] - '0');
			}
		}
		if(negative) exponent2 *= -1;
	}

	if(isDouble)
	{
		exponent += exponent2;
		if(exponent < -308) exponent = -308;
		else if(exponent > 308) exponent = 308;
		value->floatValue = (exponent >= 0) ? value->floatValue * Math::Pow10(exponent) : value->floatValue / Math::Pow10(-exponent);
		if(minus) value->floatValue *= -1;
		value->integerValue = std::lround(value->floatValue);
	}
	else
	{
		value->integerValue = minus ? -number : number;
		value->floatValue = value->integerValue;
	}
}

}
}
