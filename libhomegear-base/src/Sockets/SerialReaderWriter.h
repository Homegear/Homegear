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

#ifndef SERIALREADERWRITER_H_
#define SERIALREADERWRITER_H_

#include "../Exception.h"
#include "../Managers/FileDescriptorManager.h"
#include "../IEvents.h"

#include <thread>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

namespace BaseLib
{
class SerialReaderWriterException : public Exception
{
public:
	SerialReaderWriterException(std::string message) : Exception(message) {}
};

class SerialReaderWriter : public IEventsEx
{
public:
	//Event handling
	class ISerialReaderWriterEventSink : public IEventSinkBase
	{
	public:
		virtual void lineReceived(const std::string& data) = 0;
	};
	//End event handling

	SerialReaderWriter(BaseLib::Obj* baseLib, std::string device, int32_t baudrate, int32_t flags, bool createLockFile, int32_t readThreadPriority);
	virtual ~SerialReaderWriter();

	bool isOpen() { return _fileDescriptor && _fileDescriptor->descriptor > -1; }
	void openDevice();
	void closeDevice();
	void writeLine(std::string& data);
protected:
	BaseLib::Obj* _bl = nullptr;
	std::shared_ptr<FileDescriptor> _fileDescriptor;
	std::string _device;
	struct termios _termios;
	int32_t _baudrate = 0;
	int32_t _flags = 0;
	bool _createLockFile;
	std::string _lockfile;
	int32_t _readThreadPriority = 0;
	int32_t _handles = 0;

	bool _stopped = true;
	bool _stopReadThread = false;
	std::mutex _readThreadMutex;
	std::thread _readThread;
	std::mutex _sendMutex;

	void createLockFile();
	void readThread();
};

}
#endif
