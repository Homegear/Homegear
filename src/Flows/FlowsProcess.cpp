/* Copyright 2013-2016 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "FlowsProcess.h"
#include "../GD/GD.h"

namespace Flows
{

FlowsProcess::FlowsProcess()
{

}

FlowsProcess::~FlowsProcess()
{

}

uint32_t FlowsProcess::flowCount()
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		return _flows.size();
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
    return (uint32_t)-1;
}

void FlowsProcess::invokeFlowFinished(int32_t exitCode)
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		for(std::map<int32_t, PFlowFinishedInfo>::iterator i = _flowFinishedInfo.begin(); i != _flowFinishedInfo.end(); ++i)
		{
			i->second->finished = true;
		}
		for(std::map<int32_t, PFlowInfoServer>::iterator i = _flows.begin(); i != _flows.end(); ++i)
		{
			GD::out.printInfo("Info: Flow with id " + std::to_string(i->first) + " finished with exit code " + std::to_string(exitCode));
			i->second->finished = true;
			i->second->exitCode = exitCode;
			if(i->second->flowFinishedCallback) i->second->flowFinishedCallback(i->second, exitCode);
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
}

void FlowsProcess::invokeFlowFinished(int32_t id, int32_t exitCode)
{
	try
	{
		GD::out.printInfo("Info: Flow with id " + std::to_string(id) + " finished with exit code " + std::to_string(exitCode));
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		std::map<int32_t, PFlowFinishedInfo>::iterator flowFinishedIterator = _flowFinishedInfo.find(id);
		if(flowFinishedIterator != _flowFinishedInfo.end())
		{
			flowFinishedIterator->second->finished = true;
		}
		std::map<int32_t, PFlowInfoServer>::iterator flowIterator = _flows.find(id);
		if(flowIterator != _flows.end())
		{
			flowIterator->second->finished = true;
			flowIterator->second->exitCode = exitCode;
			if(flowIterator->second->flowFinishedCallback) flowIterator->second->flowFinishedCallback(flowIterator->second, exitCode);
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
}

PFlowInfoServer FlowsProcess::getFlow(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		std::map<int32_t, PFlowInfoServer>::iterator flowIterator = _flows.find(id);
		if(flowIterator != _flows.end()) return flowIterator->second;
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
    return PFlowInfoServer();
}

PFlowFinishedInfo FlowsProcess::getFlowFinishedInfo(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		std::map<int32_t, PFlowFinishedInfo>::iterator flowIterator = _flowFinishedInfo.find(id);
		if(flowIterator != _flowFinishedInfo.end()) return flowIterator->second;
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
    return PFlowFinishedInfo();
}

void FlowsProcess::registerFlow(int32_t id, PFlowInfoServer& flowInfo)
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		_flows[id] = flowInfo;
		_flowFinishedInfo[id] = PFlowFinishedInfo(new FlowFinishedInfo());
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

void FlowsProcess::unregisterFlow(int32_t id)
{
	try
	{
		std::lock_guard<std::mutex> flowsGuard(_flowsMutex);
		_flows.erase(id);
		_flowFinishedInfo.erase(id);
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

}
