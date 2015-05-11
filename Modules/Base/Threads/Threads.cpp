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

#include "Threads.h"
#include "../BaseLib.h"

namespace BaseLib
{

Threads::~Threads() {

}

int32_t Threads::getThreadPolicyFromString(std::string policy)
{
	HelperFunctions::toLower(policy);
	if(policy == "sched_other") return SCHED_OTHER;
	else if(policy == "sched_rr") return SCHED_RR;
	else if(policy == "sched_fifo") return SCHED_FIFO;
#ifdef SCHED_IDLE
	else if(policy == "sched_idle") return SCHED_IDLE;
#endif
#ifdef SCHED_BATCH
	else if(policy == "sched_batch") return SCHED_BATCH;
#endif
    return 0;
}

int32_t Threads::parseThreadPriority(int32_t priority, int32_t policy)
{
	if(policy == SCHED_FIFO || policy == SCHED_OTHER)
	{
		if(priority > 99) return 99;
		else if(priority < 1) return 1;
		else return priority;
	}
	else return 0;
}

void Threads::setThreadPriority(BaseLib::Obj* baseLib, pthread_t thread, int32_t priority, int32_t policy)
{
	try
	{
		if(!baseLib->settings.prioritizeThreads()) return;
		if(priority == -1)
		{
			baseLib->out.printWarning("Warning: Priority of -1 was passed to setThreadPriority.");
			priority = 0;
			policy = SCHED_OTHER;
		}
		if(policy == SCHED_OTHER) return;
		if((policy == SCHED_FIFO || policy == SCHED_RR) && (priority < 1 || priority > 99)) throw Exception("Invalid thread priority for SCHED_FIFO or SCHED_RR: " + std::to_string(priority));
#ifdef SCHED_IDLE
#ifdef SCHED_BATCH
		else if((policy == SCHED_IDLE || policy == SCHED_BATCH) && priority != 0) throw Exception("Invalid thread priority for SCHED_IDLE: " + std::to_string(priority));
#endif
#endif
		sched_param schedParam;
		schedParam.sched_priority = priority;
		int32_t error;
		//Only use SCHED_FIFO or SCHED_RR
		if((error = pthread_setschedparam(thread, policy, &schedParam)) != 0)
		{
			if(error == EPERM)
			{
				baseLib->out.printError("Could not set thread priority. The executing user does not have enough privileges. Please run \"ulimit -r 100\" before executing Homegear.");
			}
			else if(error == ESRCH) baseLib->out.printError("Could not set thread priority. Thread could not be found.");
			else if(error == EINVAL) baseLib->out.printError("Could not set thread priority: policy is not a recognized policy, or param does not make sense for the policy.");
			else baseLib->out.printError("Error: Could not set thread priority to " + std::to_string(priority) + " Error: " + std::to_string(error));
			baseLib->settings.setPrioritizeThreads(false);
		}
		else baseLib->out.printDebug("Debug: Thread priority successfully set to: " + std::to_string(priority), 7);
	}
	catch(const std::exception& ex)
    {
		baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
