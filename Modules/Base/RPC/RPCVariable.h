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

#ifndef RPCVARIABLE_H_
#define RPCVARIABLE_H_

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <map>

namespace BaseLib
{
namespace RPC
{

enum class RPCVariableType
{
	rpcVoid = 0x00,
	rpcInteger = 0x01,
	rpcBoolean = 0x02,
	rpcString = 0x03,
	rpcFloat = 0x04,
	rpcArray = 0x100,
	rpcStruct = 0x101,
	rpcDate = 0x10,
	rpcBase64 = 0x11,
	rpcVariant = 0x1111
};

class RPCVariable {
public:
	bool errorStruct = false;
	RPCVariableType type;
	std::string stringValue;
	int32_t integerValue = 0;
	double floatValue = 0;
	bool booleanValue = false;
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> arrayValue;
	std::shared_ptr<std::map<std::string, std::shared_ptr<RPCVariable>>> structValue;

	RPCVariable() { type = RPCVariableType::rpcVoid; arrayValue = std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>(new std::vector<std::shared_ptr<RPCVariable>>()); structValue = std::shared_ptr<std::map<std::string, std::shared_ptr<RPCVariable>>>(new std::map<std::string, std::shared_ptr<RPCVariable>>()); }
	RPCVariable(RPCVariableType variableType) : RPCVariable() { type = variableType; }
	RPCVariable(uint8_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = (int32_t)integer; }
	RPCVariable(int32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = integer; }
	RPCVariable(uint32_t integer) : RPCVariable() { type = RPCVariableType::rpcInteger; integerValue = (int32_t)integer; }
	RPCVariable(std::string string) : RPCVariable() { type = RPCVariableType::rpcString; stringValue = string; }
	RPCVariable(bool boolean) : RPCVariable() { type = RPCVariableType::rpcBoolean; booleanValue = boolean; }
	RPCVariable(double floatVal) : RPCVariable() { type = RPCVariableType::rpcFloat; floatValue = floatVal; }
	virtual ~RPCVariable();
	static std::shared_ptr<RPCVariable> createError(int32_t faultCode, std::string faultString);
	void print();
	static std::string getTypeString(RPCVariableType type);
	static std::shared_ptr<RPCVariable> fromString(std::string value, RPCVariableType type);
	bool operator==(const RPCVariable& rhs);
	bool operator<(const RPCVariable& rhs);
	bool operator<=(const RPCVariable& rhs);
	bool operator>(const RPCVariable& rhs);
	bool operator>=(const RPCVariable& rhs);
	bool operator!=(const RPCVariable& rhs);
private:
	void print(std::shared_ptr<RPCVariable>, std::string indent);
	void printStruct(std::shared_ptr<std::map<std::string, std::shared_ptr<RPCVariable>>> rpcStruct, std::string indent);
	void printArray(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcArray, std::string indent);
};

typedef std::pair<std::string, std::shared_ptr<RPCVariable>> RPCStructElement;
typedef std::map<std::string, std::shared_ptr<RPCVariable>> RPCStruct;
typedef std::vector<std::shared_ptr<RPCVariable>> RPCArray;

}
}

#endif /* RPCVARIABLE_H_ */
