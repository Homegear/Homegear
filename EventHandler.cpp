/* Copyright 2013 Sathya Laufer
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

#include "EventHandler.h"
#include "HelperFunctions.h"
#include "GD.h"

EventHandler::EventHandler()
{


}

EventHandler::~EventHandler()
{

}

std::shared_ptr<RPC::RPCVariable> EventHandler::add(std::shared_ptr<RPC::RPCVariable> eventDescription)
{
	try
	{
		if(eventDescription->type != RPC::RPCVariableType::rpcStruct) return RPC::RPCVariable::createError(-5, "Parameter is not of type Struct.");
		if(eventDescription->structValue->find("TYPE") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No type specified.");

		std::shared_ptr<Event> event(new Event());
		event->type = (Event::Type::Enum)eventDescription->structValue->at("TYPE")->integerValue;

		if(eventDescription->structValue->find("EVENTMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("EVENTMETHOD")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No event method specified.");
		event->eventMethod = eventDescription->structValue->at("EVENTMETHOD")->stringValue;
		if(eventDescription->structValue->find("EVENTMETHODPARAMS") != eventDescription->structValue->end())
		{
			event->eventMethodParameters = eventDescription->structValue->at("EVENTMETHODPARAMS");
		}

		if(event->type == Event::Type::Enum::triggered)
		{
			if(eventDescription->structValue->find("ADDRESS") == eventDescription->structValue->end() || eventDescription->structValue->at("ADDRESS")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No device address specified.");
			event->address = eventDescription->structValue->at("ADDRESS");
			if(eventDescription->structValue->find("VARIABLE") == eventDescription->structValue->end() || eventDescription->structValue->at("VARIABLE")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No variable specified.");
			event->variable = eventDescription->structValue->at("VARIABLE");
			if(eventDescription->structValue->find("TRIGGER") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No trigger specified.");
			event->trigger = (Event::Trigger::Enum)eventDescription->structValue->find("TRIGGER");

			if(eventDescription->structValue->find("RESETAFTER") != eventDescription->structValue->end())
			{
				if(eventDescription->structValue->find("RESETMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("RESETMETHOD")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No reset method specified.");
				event->resetMethod = eventDescription->structValue->at("RESETMETHOD")->stringValue;
				if(eventDescription->structValue->find("RESETMETHODPARAMS") != eventDescription->structValue->end())
				{
					event->resetMethodParameters = eventDescription->structValue->at("RESETMETHODPARAMS");
				}

				if(eventDescription->structValue->at("RESETAFTER")->type == RPC::RPCVariableType::rpcInteger)
				{
					event->resetAfter = eventDescription->structValue->at("RESETAFTER")->integerValue;
				}
				else if(eventDescription->structValue->at("RESETAFTER")->type == RPC::RPCVariableType::rpcStruct)
				{

				}
			}
		}
		else
		{
			if(eventDescription->structValue->find("EVENTTIME") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No time specified.");

		}

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> EventHandler::remove(std::string name)
{
	try
	{
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
