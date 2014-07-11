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

#include "IBidCoSInterface.h"
#include "../GD.h"

namespace BidCoS
{

std::vector<char> IBidCoSInterface::PeerInfo::getAESChannelMap()
{
	std::vector<char> map;
	try
	{
		for(std::map<int32_t, bool>::iterator i = aesChannels.begin(); i != aesChannels.end(); ++i)
		{
			int32_t byte = i->first / 8;
			if(map.size() < (byte + 1)) map.resize(byte + 1, 0);
			if(i->second) map.at(byte) |= (1 << (i->first % 8));
		}
		std::reverse(map.begin(), map.end());
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
    return map;
}

IBidCoSInterface::IBidCoSInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IPhysicalInterface(GD::bl, settings)
{
	_bl->hf.toLower(settings->rfKey);

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 0;
		settings->listenThreadPolicy = SCHED_OTHER;
	}
}

IBidCoSInterface::~IBidCoSInterface()
{

}

}
