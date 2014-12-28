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

#include "InsteonMessages.h"
#include "GD.h"

namespace Insteon
{
void InsteonMessages::add(std::shared_ptr<InsteonMessage> message)
{
	try
	{
		_messages.push_back(message);
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

std::shared_ptr<InsteonMessage> InsteonMessages::find(int32_t direction, std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(!packet) return std::shared_ptr<InsteonMessage>();
		int32_t subtypeMax = -1;
		std::shared_ptr<InsteonMessage>* elementToReturn = nullptr;
		for(uint32_t i = 0; i < _messages.size(); i++)
		{
			if(_messages[i]->getDirection() == direction && _messages[i]->typeIsEqual(packet))
			{
				if((signed)_messages[i]->subtypeCount() > subtypeMax)
				{
					elementToReturn = &_messages[i];
					subtypeMax = _messages[i]->subtypeCount();
				}
			}
		}
		if(elementToReturn == nullptr) return std::shared_ptr<InsteonMessage>(); else return *elementToReturn;
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
	return std::shared_ptr<InsteonMessage>();
}

std::shared_ptr<InsteonMessage> InsteonMessages::find(int32_t direction, int32_t messageType, int32_t messageSubtype, InsteonPacketFlags flags, std::vector<std::pair<uint32_t, int32_t>> subtypes)
{
	try
	{
		for(uint32_t i = 0; i < _messages.size(); i++)
		{
			if(_messages[i]->getDirection() == direction && _messages[i]->typeIsEqual(messageType, messageSubtype, flags, &subtypes)) return _messages[i];
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
    return std::shared_ptr<InsteonMessage>();
}
}
