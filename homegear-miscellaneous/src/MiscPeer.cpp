/* Copyright 2013-2019 Homegear GmbH
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

#include "MiscPeer.h"
#include "MiscCentral.h"
#include "GD.h"
#include "sys/wait.h"
#include "sys/stat.h"

namespace Misc
{
std::shared_ptr<BaseLib::Systems::ICentral> MiscPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
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
	return std::shared_ptr<BaseLib::Systems::ICentral>();
}

MiscPeer::MiscPeer(uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, parentID, eventHandler)
{
	init();
}

MiscPeer::MiscPeer(int32_t id, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, id, -1, serialNumber, parentID, eventHandler)
{
	init();
}

void MiscPeer::init()
{
    _lastScriptFinished.store(0);
	_shuttingDown = false;
	_scriptRunning = false;
	_stopScript = false;
    _stopped = true;
	_stopRunProgramThread = true;
}

MiscPeer::~MiscPeer()
{
	try
	{
		_shuttingDown = true;
		std::lock_guard<std::mutex> scriptInfoGuard(_scriptInfoMutex);
		if(_scriptInfo)
		{
			int32_t i = 0;
			while(_scriptRunning && !_scriptInfo->finished && i < 30)
			{
				GD::out.printInfo("Info: Peer " + std::to_string(_peerID) + " Waiting for script to finish...");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				i++;
			}
			if(i == 30) GD::out.printError("Error: Script of peer " + std::to_string(_peerID) + " did not finish.");
            _scriptInfo->scriptFinishedCallback = nullptr;
		}
		if(_programPID != -1)
		{
			kill(_programPID, 15);
			GD::out.printInfo("Info: Waiting for process with pid " + std::to_string(_programPID) + " started by peer " + std::to_string(_peerID) + "...");
		}
		_stopRunProgramThread = true;
		_bl->threadManager.join(_runProgramThread);
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

void MiscPeer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		Peer::homegearShuttingDown();

        stopScript(!GD::bl->shuttingDown);
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

void MiscPeer::stopScript(bool callStop)
{
    try
    {
        if(_stopScript) return;
        _stopScript = true;

        if(callStop) stop();

        int32_t i = 0;
        _stopRunProgramThread = true;
        if(!_rpcDevice->runProgram->script2.empty())
        {
            while(_scriptRunning && i < 30)
            {
                GD::out.printInfo("Info: Peer " + std::to_string(_peerID) + " Waiting for script to finish...");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                i++;
            }
            if(i == 30) GD::out.printError("Error: Script of peer " + std::to_string(_peerID) + " did not finish.");
            std::lock_guard<std::mutex> scriptInfoGuard(_scriptInfoMutex);
            if(_scriptInfo) _scriptInfo->scriptFinishedCallback = nullptr;
        }

        if(_programPID != -1)
        {
            kill(_programPID, 15);
            _programPID = -1;
        }

        if(_programPID != -1) GD::out.printInfo("Info: Waiting for process with pid " + std::to_string(_programPID) + " started by peer " + std::to_string(_peerID) + "...");
        _bl->threadManager.join(_runProgramThread);
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

bool MiscPeer::stop()
{
	try
	{
        if(_rpcDevice->runProgram->script2.empty()) return false;
        if(_stopped) return true;

        _stopped = true;

        std::string methodName = "executeMiscellaneousDeviceMethod";

		BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<BaseLib::Variable>(_peerID));
		parameters->push_back(std::make_shared<BaseLib::Variable>("stop"));
		BaseLib::PVariable innerParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
		parameters->push_back(innerParameters);

        bool noError = true;
		BaseLib::PVariable result = raiseInvokeRpc(methodName, parameters);
		if(result->errorStruct)
        {
            GD::out.printError("Error calling stop on peer " + std::to_string(_peerID) + ": " + result->structValue->at("faultString")->stringValue);
            noError = false;
        }

        parameters->at(1)->stringValue = "waitForStop";
        result = raiseInvokeRpc(methodName, parameters);
        if(result->errorStruct)
        {
            GD::out.printError("Error calling waitForStop on peer " + std::to_string(_peerID) + ": " + result->structValue->at("faultString")->stringValue);
            noError = false;
        }
        return noError;
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
    return false;
}

void MiscPeer::runProgram()
{
	try
	{
		if(!_rpcDevice->runProgram) return;
		while(GD::bl->booting && !_stopRunProgramThread) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::string path = _rpcDevice->runProgram->path;
		if(path.empty()) return;
		if(path.front() != '/') path = GD::bl->settings.scriptPath() + path;
		std::vector<std::string> arguments = _rpcDevice->runProgram->arguments;
		for(std::vector<std::string>::iterator i = arguments.begin(); i != arguments.end(); ++i)
		{
			GD::bl->hf.stringReplace(*i, "$PEERID", std::to_string(_peerID));
			GD::bl->hf.stringReplace(*i, "$RPCPORT", std::to_string(_bl->rpcPort));
		}
		if(_rpcDevice->runProgram->interval == 0) _rpcDevice->runProgram->interval = 10;
		while(!_stopRunProgramThread)
		{
			_programPID = -1;
			int64_t startTime = GD::bl->hf.getTime();
			struct stat statStruct;
			if(stat(path.c_str(), &statStruct) < 0)
			{
				GD::out.printError("Error: Could not execute script: " + std::string(strerror(errno)));
				std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
				if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
				std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
				continue;
			}

			uint32_t uid = getuid();
			uint32_t gid = getgid();
			if((statStruct.st_mode & S_IXOTH) == 0)
			{
				if(statStruct.st_gid != gid || (statStruct.st_gid == gid && (statStruct.st_mode & S_IXGRP) == 0))
				{
					if(statStruct.st_uid != uid || (statStruct.st_uid == uid && (statStruct.st_mode & S_IXUSR) == 0))
					{
						GD::out.printError("Error: Could not execute script. No permission or executable bit is not set.");
						std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
						if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
						continue;
					}
				}
			}
			if((statStruct.st_mode & (S_IXGRP | S_IXUSR)) == 0) //At least in Debian it is not possible to execute scripts, when the execution bit is only set for "other".
			{
				GD::out.printError("Error: Could not execute script. Executable bit is not set for user or group.");
				std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
				if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
				continue;
			}
			if((statStruct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
			{
				GD::out.printError("Error: Could not execute script. The file mode is not set to executable.");
				std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
				if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
				continue;
			}
			_programPID = GD::bl->hf.system(path, arguments);
			if(_programPID < 0)
			{
				GD::out.printError("Error: Could not execute script.");
				std::this_thread::sleep_for(std::chrono::milliseconds(_rpcDevice->runProgram->interval));
				if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
				continue;
			}
			GD::out.printInfo("Info: Started program " + path + ". PID is " + std::to_string(_programPID) + ".");

			if(_rpcDevice->runProgram->startType == RunProgram::StartType::once) return;
			else if(_rpcDevice->runProgram->startType == RunProgram::StartType::interval)
			{
				int64_t totalTimeToSleep = (_rpcDevice->runProgram->interval * 1000) - (GD::bl->hf.getTime() - startTime);
				if(totalTimeToSleep < 0) totalTimeToSleep = 0;
				for(int32_t i = 0; i < (totalTimeToSleep / 100) + 1; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			int32_t result = 0;
			int32_t status = 0;
			while(!_stopRunProgramThread && (result = waitpid(_programPID, &status, WNOHANG)) == 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			if(result == -1)
			{
				GD::out.printCritical("Critical error executing waitpid: " + std::string(strerror(errno)) + ". Exiting run program thread of peer " + std::to_string(_peerID));
				_programPID = -1;
				return;
			}
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
	_programPID = -1;
}

void MiscPeer::scriptFinished(BaseLib::ScriptEngine::PScriptInfo& scriptInfo, int32_t exitCode)
{
	try
	{
		_scriptRunning = false;
		if(!_shuttingDown && !GD::bl->shuttingDown && !deleting && !_stopScript)
		{
            if(exitCode == 0) GD::out.printInfo("Info: Script of peer " + std::to_string(_peerID) + " finished with exit code 0. Restarting...");
			else GD::out.printError("Error: Script of peer " + std::to_string(_peerID) + " was killed. Restarting...");
            _bl->threadManager.start(_runProgramThread, true, &MiscPeer::runScript, this, BaseLib::HelperFunctions::getTime() - _lastScriptFinished.load() < 10000 ? 10000 : 0);
            _lastScriptFinished.store(BaseLib::HelperFunctions::getTime());
		}
		else GD::out.printInfo("Info: Script of peer " + std::to_string(_peerID) + " finished.");
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

void MiscPeer::runScript(int32_t delay)
{
	try
	{
		if(!_rpcDevice->runProgram || _shuttingDown || deleting) return;
		while(GD::bl->booting && !_stopRunProgramThread)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}
		if(_stopRunProgramThread) return;
		if(delay > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			if(_stopRunProgramThread) return;
		}
		std::string script = _rpcDevice->runProgram->script.empty() ? _rpcDevice->runProgram->script2 : _rpcDevice->runProgram->script;
		if(script.empty()) return;

		std::string path = _rpcDevice->getPath();
		std::string args;
		std::vector<std::string> arguments = _rpcDevice->runProgram->arguments;
		for(std::vector<std::string>::iterator i = arguments.begin(); i != arguments.end(); ++i)
		{
			GD::bl->hf.stringReplace(*i, "$PEERID", std::to_string(_peerID));
			GD::bl->hf.stringReplace(*i, "$RPCPORT", std::to_string(_bl->rpcPort));
			args += *i + " ";
		}
		BaseLib::HelperFunctions::trim(args);

		if(_rpcDevice->runProgram->interval == 0) _rpcDevice->runProgram->interval = 10;

		std::lock_guard<std::mutex> scriptInfoGuard(_scriptInfoMutex);
		if(_shuttingDown) return;
        if(_scriptInfo) _scriptInfo->scriptFinishedCallback = nullptr;
		_scriptInfo = std::make_shared<BaseLib::ScriptEngine::ScriptInfo>(!_rpcDevice->runProgram->script2.empty() ? BaseLib::ScriptEngine::ScriptInfo::ScriptType::device2 : BaseLib::ScriptEngine::ScriptInfo::ScriptType::device, path, path, script, args, _peerID);
		if(_rpcDevice->runProgram->startType != RunProgram::StartType::once)
		{
			_scriptInfo->scriptFinishedCallback = std::bind(&MiscPeer::scriptFinished, this, std::placeholders::_1, std::placeholders::_2);
		}

        _stopped = false;

		raiseRunScript(_scriptInfo, false);
		_scriptRunning = _scriptInfo->started;
		if(!_scriptRunning && !_bl->shuttingDown)
		{
			GD::out.printError("Error: Could not start script of peer " + std::to_string(_peerID) + ".");
			_scriptInfo->finished = true;
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

std::string MiscPeer::handleCliCommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "config print\t\tPrints all configuration parameters and their values" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << _rpcDevice->functions.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "config print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints all configuration parameters of this peer. The values are in BidCoS packet format." << std::endl;
						stringStream << "Usage: config print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			return printConfig();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}

std::string MiscPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "VALUES" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		return stringStream.str();
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
    return "";
}


void MiscPeer::loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows)
{
	try
	{
		if(!rows) rows = _bl->db->getPeerVariables(_peerID);
		Peer::loadVariables(central, rows);
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

bool MiscPeer::load(BaseLib::Systems::ICentral* central)
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows;
		loadVariables(central, rows);

		_rpcDevice = GD::family->getRpcDevices()->find(_deviceType, _firmwareVersion, -1);
		if(!_rpcDevice)
		{
			GD::out.printError("Error loading Miscellaneous peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString(_deviceType) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		initializeTypeString();
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		initProgram();

		return true;
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
    return false;
}

void MiscPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
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

void MiscPeer::initProgram()
{
	try
    {
		if(_rpcDevice->runProgram && !GD::bl->shuttingDown && !deleting && !_shuttingDown)
		{
			_stopRunProgramThread = true;
			_bl->threadManager.join(_runProgramThread);
			_stopRunProgramThread = false;
            _stopScript = false;
            _lastScriptFinished.store(0);
            _scriptRunning = false;
			if(!_rpcDevice->runProgram->script.empty() || !_rpcDevice->runProgram->script2.empty()) _bl->threadManager.start(_runProgramThread, true, &MiscPeer::runScript, this, false);
			else _bl->threadManager.start(_runProgramThread, true, &MiscPeer::runProgram, this);
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

PParameterGroup MiscPeer::getParameterSet(int32_t channel, ParameterGroup::Type::Enum type)
{
	try
	{
		PFunction rpcChannel = _rpcDevice->functions.at(channel);
		if(type == ParameterGroup::Type::Enum::variables) return rpcChannel->variables;
		else if(type == ParameterGroup::Type::Enum::config) return rpcChannel->configParameters;
		else if(type == ParameterGroup::Type::Enum::link) return rpcChannel->linkParameters;
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
	return PParameterGroup();
}

bool MiscPeer::getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(parameter->id == "IP_ADDRESS")
		{
			std::vector<uint8_t> parameterData;
			parameter->convertToPacket(std::make_shared<Variable>(_ip), parameterData);
			valuesCentral[channel][parameter->id].setBinaryData(parameterData);
		}
		else if(parameter->id == "PEER_ID")
		{
			std::vector<uint8_t> parameterData;
			parameter->convertToPacket(std::make_shared<Variable>((int32_t)_peerID), parameterData);
			valuesCentral[channel][parameter->id].setBinaryData(parameterData);
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
    return false;
}

bool MiscPeer::getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(parameter->id == "IP_ADDRESS")
		{
			std::vector<uint8_t> parameterData;
			parameter->convertToPacket(std::make_shared<Variable>(_ip), parameterData);
			valuesCentral[channel][parameter->id].setBinaryData(parameterData);
		}
		else if(parameter->id == "PEER_ID")
		{
			std::vector<uint8_t> parameterData;
			parameter->convertToPacket(std::make_shared<Variable>((int32_t)_peerID), parameterData);
			valuesCentral[channel][parameter->id].setBinaryData(parameterData);
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
    return false;
}

PVariable MiscPeer::getDeviceInfo(BaseLib::PRpcClientInfo clientInfo, std::map<std::string, bool> fields)
{
	try
	{
		PVariable info(Peer::getDeviceInfo(clientInfo, fields));
		return info;
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
    return PVariable();
}

PVariable MiscPeer::getParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, bool checkAcls)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		Functions::iterator functionIterator = _rpcDevice->functions.find(channel);
		if(functionIterator == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		if(type == ParameterGroup::Type::none) type = ParameterGroup::Type::link;
		PFunction rpcFunction = functionIterator->second;
		PParameterGroup parameterGroup = rpcFunction->getParameterGroup(type);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set.");
		PVariable variables(new Variable(VariableType::tStruct));

		auto central = getCentral();
		if(!central) return Variable::createError(-32500, "Could not get central.");

		for(Parameters::iterator i = parameterGroup->parameters.begin(); i != parameterGroup->parameters.end(); ++i)
		{
			if(!i->second || i->second->id.empty() || !i->second->visible) continue;
			if(!i->second->visible && !i->second->service && !i->second->internal && !i->second->transform)
			{
				GD::out.printDebug("Debug: Omitting parameter " + i->second->id + " because of it's ui flag.");
				continue;
			}
			PVariable element;
			if(type == ParameterGroup::Type::Enum::variables)
			{
				if(checkAcls && !clientInfo->acls->checkVariableReadAccess(central->getPeer(_peerID), channel, i->first)) continue;

				if(!i->second->readable) continue;
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find(i->second->id) == valuesCentral[channel].end()) continue;
				if(getParamsetHook2(clientInfo, i->second, channel, variables)) continue;
				std::vector<uint8_t> parameterData = valuesCentral[channel][i->second->id].getBinaryData();
				element = i->second->convertFromPacket(parameterData);
			}
			else if(type == ParameterGroup::Type::Enum::config)
			{
				if(configCentral.find(channel) == configCentral.end()) continue;
				if(configCentral[channel].find(i->second->id) == configCentral[channel].end()) continue;
				std::vector<uint8_t> parameterData = configCentral[channel][i->second->id].getBinaryData();
				element = i->second->convertFromPacket(parameterData);
				if(i->second->password && (!clientInfo || !clientInfo->scriptEngineServer)) element.reset(new Variable(element->type));
			}
			else if(type == ParameterGroup::Type::Enum::link)
			{
				return Variable::createError(-3, "Parameter set type is not supported.");
			}

			if(!element) continue;
			if(element->type == VariableType::tVoid) continue;
			variables->structValue->insert(StructElement(i->second->id, element));
		}
		return variables;
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable MiscPeer::getParamsetDescription(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, bool checkAcls)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		Functions::iterator functionIterator = _rpcDevice->functions.find(channel);
		if(functionIterator == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel");
		PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(type);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set");
		if(type == ParameterGroup::Type::link && remoteID > 0)
		{
			std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return Variable::createError(-2, "Unknown remote peer.");
		}

		return Peer::getParamsetDescription(clientInfo, channel, parameterGroup, checkAcls);
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable MiscPeer::putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool checkAcls, bool onlyPushing)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		Functions::iterator functionIterator = _rpcDevice->functions.find(channel);
		if(functionIterator == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		if(type == ParameterGroup::Type::none) type = ParameterGroup::Type::link;
		PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(type);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return PVariable(new Variable(VariableType::tVoid));

		auto central = getCentral();
		if(!central) return Variable::createError(-32500, "Could not get central.");

		if(type == ParameterGroup::Type::Enum::config)
		{
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::vector<uint8_t> value;
				if(configCentral[channel].find(i->first) == configCentral[channel].end()) continue;
				BaseLib::Systems::RpcConfigurationParameter& parameter = configCentral[channel][i->first];
				if(!parameter.rpcParameter) continue;
				if(parameter.rpcParameter->password && i->second->stringValue.empty()) continue; //Don't safe password if empty
				parameter.rpcParameter->convertToPacket(i->second, value);
				std::vector<uint8_t> shiftedValue = value;
				parameter.rpcParameter->adjustBitPosition(shiftedValue);
				int32_t intIndex = (int32_t)parameter.rpcParameter->physical->index;
				int32_t list = parameter.rpcParameter->physical->list;
				if(list == -1) list = 0;
				if(allParameters[list].find(intIndex) == allParameters[list].end()) allParameters[list][intIndex] = shiftedValue;
				else
				{
					uint32_t index = 0;
					for(std::vector<uint8_t>::iterator j = shiftedValue.begin(); j != shiftedValue.end(); ++j)
					{
						if(index >= allParameters[list][intIndex].size()) allParameters[list][intIndex].push_back(0);
						allParameters[list][intIndex].at(index) |= *j;
						index++;
					}
				}
				parameter.setBinaryData(value);
				if(parameter.databaseId > 0) saveParameter(parameter.databaseId, value);
				else saveParameter(0, ParameterGroup::Type::Enum::config, channel, i->first, value);
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter.rpcParameter->physical->operationType != IPhysical::OperationType::Enum::config && parameter.rpcParameter->physical->operationType != IPhysical::OperationType::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(!changedParameters.empty() && !changedParameters.begin()->second.empty()) raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);
		}
		else if(type == ParameterGroup::Type::Enum::variables)
		{
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;

				if(checkAcls && !clientInfo->acls->checkVariableWriteAccess(central->getPeer(_peerID), channel, i->first)) continue;

				setValue(clientInfo, channel, i->first, i->second, false);
			}
		}
		else
		{
			return Variable::createError(-3, "Parameter set type is not supported.");
		}
		return PVariable(new Variable(VariableType::tVoid));
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable MiscPeer::setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait)
{
	try
	{
		if(!clientInfo) clientInfo.reset(new BaseLib::RpcClientInfo());
		Peer::setValue(clientInfo, channel, valueKey, value, wait); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(valueKey.empty()) return Variable::createError(-5, "Value key is empty.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");
		PParameter rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return Variable::createError(-5, "Unknown parameter.");
		if(rpcParameter->service)
		{
			if(channel == 0 && value->type == VariableType::tBoolean)
			{
				if(serviceMessages->set(valueKey, value->booleanValue)) return PVariable(new Variable(VariableType::tVoid));
			}
			else if(value->type == VariableType::tInteger) serviceMessages->set(valueKey, value->integerValue, channel);
		}
		if(rpcParameter->logical->type == ILogical::Type::tAction && !value->booleanValue) return Variable::createError(-5, "Parameter of type action cannot be set to \"false\".");
		if(!rpcParameter->writeable && clientInfo->id != -1 && !(rpcParameter->addonWriteable && clientInfo->addon)) return Variable::createError(-6, "parameter is read only");
		BaseLib::Systems::RpcConfigurationParameter& parameter = valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>());

		if(rpcParameter->physical->operationType == IPhysical::OperationType::Enum::store)
		{
			std::vector<uint8_t> parameterData;
			rpcParameter->convertToPacket(value, parameterData);
			parameter.setBinaryData(parameterData);
			if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
			else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameterData);

			valueKeys->push_back(valueKey);
			values->push_back(rpcParameter->convertFromPacket(parameterData, true));
            std::string address = _serialNumber + ":" + std::to_string(channel);
            if(clientInfo->scriptEngineServer)
            {
                if(clientInfo->peerId != 0 && clientInfo->peerId == _peerID)
                {
                    std::string eventSource = "device-" + std::to_string(_peerID);
                    raiseEvent(eventSource, _peerID, channel, valueKeys, values);
                    raiseRPCEvent(eventSource, _peerID, channel, address, valueKeys, values);
                }
                else
                {
                    std::string eventSource = "scriptEngine";
                    raiseEvent(eventSource, _peerID, channel, valueKeys, values);
                    raiseRPCEvent(eventSource, _peerID, channel, address, valueKeys, values);
                }
            }
            else
            {
                raiseEvent(clientInfo->initInterfaceId, _peerID, channel, valueKeys, values);
                raiseRPCEvent(clientInfo->initInterfaceId, _peerID, channel, address, valueKeys, values);
            }

			return PVariable(new Variable(VariableType::tVoid));
		}
		return Variable::createError(-6, "Only interface type \"store\" is supported for this device family.");
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
    return Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

}
