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

#ifndef MONITOR_H_
#define MONITOR_H_

#include <thread>
#include <mutex>
#include <atomic>

class Monitor
{
public:
	Monitor();
	~Monitor();
	void init();
	void stop();
	void dispose();
	bool killedProcess();

	/**
	 * Causes checkHealthThread to exit as soon as possible and prevents starting checkHealthThread.
	 */
	void suspend();

	void prepareParent();
	void prepareChild();
	void checkHealth(pid_t mainProcessId);
private:
	int _pipeToChild[2];
	int _pipeFromChild[2];
	std::atomic_bool _suspendMonitoring;
	std::atomic_bool _stopMonitorThread;
	std::thread _checkHealthThread;
	std::thread _monitorThread;
	bool _killedProcess = false;
	bool _disposing = false;
	std::mutex _checkHealthMutex;

	void monitor();
	void killChild(pid_t mainProcessId);
	bool lifetick();
	void checkHealthThread(pid_t mainProcessId);
};

#endif
