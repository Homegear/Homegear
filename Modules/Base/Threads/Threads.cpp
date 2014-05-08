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

#include "Threads.h"
#include "../BaseLib.h"

Threads::~Threads() {

}

void Threads::setThreadPriority(pthread_t thread, int32_t priority, int32_t policy)
{
	try
	{
		if(!BaseLib::settings.prioritizeThreads()) return;
		if(policy != SCHED_FIFO && policy != SCHED_RR) priority = 0;
		if((policy == SCHED_FIFO || policy == SCHED_RR) && (priority < 1 || priority > 99)) throw Exception("Invalid thread priority: " + std::to_string(priority));
		sched_param schedParam;
		schedParam.sched_priority = priority;
		int32_t error;
		//Only use SCHED_FIFO or SCHED_RR
		if((error = pthread_setschedparam(thread, policy, &schedParam)) != 0)
		{
			if(error == EPERM)
			{
				Output::printError("Could not set thread priority. The executing user does not have enough privileges. Please run \"ulimit -r 100\" before executing Homegear.");
			}
			else if(error == ESRCH) Output::printError("Could not set thread priority. Thread could not be found.");
			else if(error == EINVAL) Output::printError("Could not set thread priority: policy is not a recognized policy, or param does not make sense for the policy.");
			else Output::printError("Error: Could not set thread priority to " + std::to_string(priority) + " Error: " + std::to_string(error));
			BaseLib::settings.setPrioritizeThreads(false);
		}
		else Output::printDebug("Debug: Thread priority successfully set to: " + std::to_string(priority), 7);
	}
	catch(const std::exception& ex)
    {
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
