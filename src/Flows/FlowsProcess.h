/* Copyright 2013-2017 Sathya Laufer
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

#ifndef FLOWSPROCESS_H_
#define FLOWSPROCESS_H_

#include "FlowsClientData.h"
#include <homegear-base/BaseLib.h>
#include "FlowInfoServer.h"

namespace Flows
{

struct FlowFinishedInfo
{
	bool finished = false;
};
typedef std::shared_ptr<FlowFinishedInfo> PFlowFinishedInfo;

class FlowsProcess
{
private:
	pid_t _pid = 0;
	std::mutex _flowsMutex;
	std::map<int32_t, PFlowInfoServer> _flows;
	std::map<int32_t, PFlowFinishedInfo> _flowFinishedInfo;
	PFlowsClientData _clientData;
	std::atomic_uint _nodeThreadCount;
public:
	FlowsProcess();
	virtual ~FlowsProcess();

	std::condition_variable requestConditionVariable;

	pid_t getPid() { return _pid; }
	void setPid(pid_t value) { _pid = value; }
	PFlowsClientData& getClientData() { return _clientData; }
	void setClientData(PFlowsClientData& value) { _clientData = value; }

	void invokeFlowFinished(int32_t exitCode);
	void invokeFlowFinished(int32_t id, int32_t exitCode);
	uint32_t flowCount();
	uint32_t nodeThreadCount();
	PFlowInfoServer getFlow(int32_t id);
	PFlowFinishedInfo getFlowFinishedInfo(int32_t id);
	void registerFlow(int32_t id, PFlowInfoServer& flowInfo);
	void unregisterFlow(int32_t id);
};

typedef std::shared_ptr<FlowsProcess> PFlowsProcess;

}
#endif
