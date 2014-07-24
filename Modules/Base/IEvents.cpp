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

#include "IEvents.h"

namespace BaseLib
{

IEvents::IEvents()
{


}

IEvents::~IEvents()
{

}

void IEvents::setEventHandler(IEventSinkBase* eventHandler)
{
	if(!eventHandler) return;
	_eventHandler = eventHandler;
}

void IEvents::resetEventHandler()
{
	_eventHandler = nullptr;
}

IEventSinkBase* IEvents::getEventHandler()
{
    return _eventHandler;
}

IEventsEx::IEventsEx()
{


}

IEventsEx::~IEventsEx()
{

}

void IEventsEx::addEventHandler(IEventSinkBase* eventHandler)
{
	if(!eventHandler) return;
	_eventHandlerMutex.lock();
	for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
	{
		if(*i == eventHandler)
		{
			_eventHandlerMutex.unlock();
			return;
		}
	}
	_eventHandlers.push_front(eventHandler);
    _eventHandlerMutex.unlock();
}

void IEventsEx::addEventHandlers(std::forward_list<IEventSinkBase*> eventHandlers)
{
	if(eventHandlers.empty()) return;
	_eventHandlerMutex.lock();
	for(std::forward_list<IEventSinkBase*>::iterator i = eventHandlers.begin(); i != eventHandlers.end(); ++i)
	{
		bool exists = false;
		for(std::forward_list<IEventSinkBase*>::iterator j = _eventHandlers.begin(); j != _eventHandlers.end(); ++j)
		{
			if(*j == *i)
			{
				exists = true;
				break;
			}
		}
		if(exists) continue;
		_eventHandlers.push_front(*i);
	}
    _eventHandlerMutex.unlock();
}

void IEventsEx::removeEventHandler(IEventSinkBase* eventHandler)
{
	if(!eventHandler) return;
	_eventHandlerMutex.lock();
	_eventHandlers.remove(eventHandler);
    _eventHandlerMutex.unlock();
}

std::forward_list<IEventSinkBase*> IEventsEx::getEventHandlers()
{
	std::forward_list<IEventSinkBase*> eventHandlers;
	_eventHandlerMutex.lock();
	eventHandlers = _eventHandlers;
    _eventHandlerMutex.unlock();
    return eventHandlers;
}

}
