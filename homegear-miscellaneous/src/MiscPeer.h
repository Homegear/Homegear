/* Copyright 2013-2020 Homegear GmbH
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

#ifndef MISCPEER_H_
#define MISCPEER_H_

#include <homegear-base/BaseLib.h>

#include <list>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace Misc
{
class MiscCentral;

class MiscPeer : public BaseLib::Systems::Peer
{
public:
	MiscPeer(uint32_t parentID, IPeerEventSink* eventHandler);
	MiscPeer(int32_t id, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler);
	virtual ~MiscPeer();

	//Features
	virtual bool wireless() { return false; }
	//End features

	virtual std::string handleCliCommand(std::string command);

	virtual bool load(BaseLib::Systems::ICentral* central);
    void initProgram();

	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
	virtual std::string getFirmwareVersionString(int32_t firmwareVersion) { return "1.0"; }
    virtual bool firmwareUpdateAvailable() { return false; }

    std::string printConfig();

    /**
	 * {@inheritDoc}
	 */
    virtual void homegearShuttingDown();

	bool stop();
    void stopScript(bool callStop = true);

	//RPC methods
	virtual PVariable getDeviceInfo(BaseLib::PRpcClientInfo clientInfo, std::map<std::string, bool> fields);
	virtual PVariable getParamsetDescription(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, bool checkAcls);
	virtual PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool checkAcls, bool onlyPushing = false);
	virtual PVariable setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait);
	//End RPC methods
protected:
    std::atomic_long _lastScriptFinished;
    std::atomic_bool _stopScript;
    std::atomic_bool _stopped;
	std::atomic_bool _shuttingDown;
	std::atomic_bool _scriptRunning;
	std::atomic_bool _stopRunProgramThread;
	std::thread _runProgramThread;
	pid_t _programPID = -1;
	std::mutex _scriptInfoMutex;
	BaseLib::ScriptEngine::PScriptInfo _scriptInfo;

	void init();

	virtual void loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows);
    virtual void saveVariables();
    virtual void savePeers() {}

	void runProgram();
	void runScript(int32_t delay = 0);
	void scriptFinished(BaseLib::ScriptEngine::PScriptInfo& scriptInfo, int32_t exitCode);

	virtual std::shared_ptr<BaseLib::Systems::ICentral> getCentral();

	virtual PParameterGroup getParameterSet(int32_t channel, ParameterGroup::Type::Enum type);

	// {{{ Hooks
		/**
		 * {@inheritDoc}
		 */
		virtual bool getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters);

		/**
		 * {@inheritDoc}
		 */
		virtual bool getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters);
	// }}}
};

}

#endif
