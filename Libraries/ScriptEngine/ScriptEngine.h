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

#ifndef SCRIPTENGINE_H_
#define SCRIPTENGINE_H_

#include "../../Modules/Base/BaseLib.h"
#include "PH7VariableConverter.h"

#include <mutex>

#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include "ph7.h"

class ScriptEngine
{
public:
	class Session
	{
	public:
		Session() {}
		virtual ~Session() {}

		int64_t lastAccess = 0;
		std::map<std::string, std::string> data;
	};

	class ScriptInfo
	{
	public:
		ScriptInfo() {}
		virtual ~ScriptInfo()
		{
			if(compiledProgram)
			{
				ph7_vm_release(compiledProgram);
				compiledProgram = nullptr;
			}
		}

		ph7_vm* compiledProgram = nullptr;
		int32_t lastModified = 0;
		std::string path;
		ScriptEngine* engine = nullptr;
		std::vector<std::string>* headers = nullptr;
		std::string sessionID;
		bool died = false;
		ph7_value* cookie = nullptr;
	};

	ScriptEngine();
	virtual ~ScriptEngine();
	void dispose();

	int32_t execute(const std::string& path, const std::string& arguments);
	int32_t executeWebRequest(const std::string& path, const std::vector<char>& request, std::vector<char>& output, std::vector<std::string>& headers);
	void clearPrograms();
	std::shared_ptr<Session> getSession(std::string id);

	void appendOutput(std::string& path, const char* output, uint32_t outputLength);
	void resetOutput(std::string& path);
protected:
	bool _noExecution = false;
	bool _disposing = false;
	ph7 *_engine = nullptr;

	std::mutex _programsMutex;
	std::map<std::string, ScriptInfo> _programs;
	volatile int32_t _executionCount;
	std::map<std::string, std::unique_ptr<std::mutex>> _executeMutexes;
	std::mutex _outputMutex;
	std::map<std::string, std::vector<char>*> _outputs;
	std::mutex _sessionsMutex;
	std::map<std::string, std::shared_ptr<Session>> _sessions;

	ScriptInfo* addProgram(std::string path,  bool storeOutput);
	ScriptInfo* getProgram(std::string path, bool storeOutput);
	bool isValid(const std::string& path, ph7_vm* compiledProgram);
	void removeProgram(std::string path);
	void printError(int32_t code);
	void cleanUpSessions();
	std::vector<std::string> getArgs(const std::string& path, const std::string& args);
};
#endif
