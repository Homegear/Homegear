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

#ifndef IEVENTS_H_
#define IEVENTS_H_

#include "Output/Output.h"

#include <vector>
#include <mutex>

namespace BaseLib
{
class IEventSinkBase
{
public:
	IEventSinkBase() {}
	virtual ~IEventSinkBase() {};
};

class IEvents
{
public:
	IEvents();
	virtual ~IEvents();

	virtual void addEventHandler(IEventSinkBase* eventHandler);
	virtual void addEventHandlers(std::vector<IEventSinkBase*> eventHandlers);
	virtual void removeEventHandler(IEventSinkBase* eventHandler);
	virtual std::vector<IEventSinkBase*> getEventHandlers();
protected:
	std::mutex _eventHandlerMutex;
    std::vector<IEventSinkBase*> _eventHandlers;
};
}
#endif /* IEVENTS_H_ */
