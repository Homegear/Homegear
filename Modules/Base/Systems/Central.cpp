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

#include "Central.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

Central::Central(BaseLib::Obj* baseLib, LogicalDevice* me)
{
	_baseLib = baseLib;
	_me = me;
}

Central::~Central()
{
}

int32_t Central::physicalAddress()
{
	return _me->getAddress();
}

//RPC methods
std::shared_ptr<BaseLib::RPC::RPCVariable> Central::getLinks(std::string serialNumber, int32_t channel, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return getLinks(0, -1, flags);
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		return getLinks(peer->getID(), channel, flags);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}


std::shared_ptr<BaseLib::RPC::RPCVariable> Central::getLinks(uint64_t peerID, int32_t channel, int32_t flags)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		std::shared_ptr<BaseLib::RPC::RPCVariable> element(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		if(peerID == 0)
		{
			try
			{
				std::vector<std::shared_ptr<Peer>> peers;
				//Copy all peers first, because getLinks takes very long and we don't want to lock _peersMutex too long
				_me->getPeers(peers);

				for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					//listDevices really needs a lot of ressources, so wait a little bit after each device
					std::this_thread::sleep_for(std::chrono::milliseconds(3));
					element = (*i)->getLink(channel, flags, true);
					array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
			catch(const std::exception& ex)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		else
		{
			std::shared_ptr<Peer> peer(_me->getPeer(peerID));
			if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
			element = peer->getLink(channel, flags, false);
			array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
//End RPC methods

}
}
