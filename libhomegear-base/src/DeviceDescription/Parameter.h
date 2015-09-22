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

#ifndef DEVICEPARAMETER_H_
#define DEVICEPARAMETER_H_

#include "ParameterCast.h"
#include "Logical.h"
#include "Physical.h"
#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>

using namespace rapidxml;
using namespace BaseLib::DeviceDescription::ParameterCast;

namespace BaseLib
{

class Obj;

namespace DeviceDescription
{

class Parameter;
class ParameterGroup;

typedef std::shared_ptr<Parameter> PParameter;
typedef std::map<std::string, PParameter> Parameters;

class Parameter
{
public:
	class Packet
	{
	public:
		struct ConditionOperator
		{
			enum Enum { none, e, g, l, ge, le };
		};

		struct Type
		{
			enum Enum { none = 0, get = 1, set = 2, event = 3 };
		};

		std::string id;
		Type::Enum type = Type::none;
		std::vector<std::string> autoReset;
		std::pair<std::string, int32_t> delayedAutoReset;
		std::string responseId;
		ConditionOperator::Enum conditionOperator = ConditionOperator::none;
		int32_t conditionValue = -1;

		Packet() {}
		virtual ~Packet() {}

		bool checkCondition(int32_t value);
	};

	//Attributes
	std::string id;

	//Properties
	bool readable = true;
	bool writeable = true;
	bool addonWriteable = true;
	bool visible = true;
	bool internal = false;
	bool parameterGroupSelector = false;
	bool service = false;
	bool sticky = false;
	bool transform = false;
	bool isSigned = false;
	std::string control;
	std::string unit;
	Casts casts;

	//Elements
	std::shared_ptr<ILogical> logical;
	std::shared_ptr<IPhysical> physical;
	std::vector<std::shared_ptr<Packet>> getPackets;
	std::vector<std::shared_ptr<Packet>> setPackets;
	std::vector<std::shared_ptr<Packet>> eventPackets;

	//Helpers
	bool hasDelayedAutoResetParameters = false;

	Parameter(BaseLib::Obj* baseLib, ParameterGroup* parent);
	Parameter(BaseLib::Obj* baseLib, xml_node<>* node, ParameterGroup* parent);
	virtual ~Parameter();

	//Helpers
	/**
	 * Converts binary data of a packet received by a physical interface to a RPC variable.
	 *
	 * @param[in] data The data to convert. It is not modified, even though there is no "const".
	 * @param isEvent (default false) Set to "true" if packet is an event packet. Necessary to set value of "Action" correctly.
	 * @return Returns the RPC variable.
	 */
	PVariable convertFromPacket(std::vector<uint8_t>& data, bool isEvent = false);

	/**
	 * Converts a RPC variable to binary data to send it over a physical interface.
	 *
	 * @param[in] value The value to convert.
	 * @param[out] convertedValue The converted binary data.
	 */
	void convertToPacket(const PVariable value, std::vector<uint8_t>& convertedValue);

	/**
	 * Tries to convert a string value to a binary data to send it over a physical interface.
	 *
	 * @param[in] value The value to convert.
	 * @param[out] The converted binary data.
	 */
	void convertToPacket(std::string value, std::vector<uint8_t>& convertedValue);

	void adjustBitPosition(std::vector<uint8_t>& data);

	ParameterGroup* parent();
protected:
	BaseLib::Obj* _bl = nullptr;

	//Helpers
	ParameterGroup* _parent = nullptr;

	/**
	 * Reverses a binary array.
	 *
	 * @param[in] data The array to reverse.
	 * @param[out] reversedData The reversed array.
	 */
	void reverseData(const std::vector<uint8_t>& data, std::vector<uint8_t>& reversedData);
};

}
}

#endif
