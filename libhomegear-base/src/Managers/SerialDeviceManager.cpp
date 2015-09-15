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

#include "SerialDeviceManager.h"
#include "../BaseLib.h"

namespace BaseLib
{

SerialDeviceManager::SerialDeviceManager()
{
}

void SerialDeviceManager::init(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void SerialDeviceManager::add(const std::string& device, std::shared_ptr<SerialReaderWriter> readerWriter)
{
	try
	{
		if(device.empty() || !readerWriter) return;
		_devicesMutex.lock();
		if(_devices.find(device) != _devices.end())
		{
			_devicesMutex.unlock();
			return;
		}
		_devices[device] = readerWriter;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_devicesMutex.unlock();
}

std::shared_ptr<SerialReaderWriter> SerialDeviceManager::create(std::string device, int32_t baudrate, int32_t flags, bool createLockFile, int32_t readThreadPriority)
{
	std::shared_ptr<SerialReaderWriter> readerWriter(new SerialReaderWriter(_bl, device, baudrate, flags, createLockFile, readThreadPriority));
	add(device, readerWriter);
	return readerWriter;
}

std::shared_ptr<SerialReaderWriter> SerialDeviceManager::get(const std::string& device)
{
	try
	{
		std::shared_ptr<SerialReaderWriter> readerWriter;
		_devicesMutex.lock();
		if(_devices.find(device) != _devices.end()) readerWriter = _devices.at(device);
		_devicesMutex.unlock();
		return readerWriter;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_devicesMutex.unlock();
	return std::shared_ptr<SerialReaderWriter>();
}

void SerialDeviceManager::remove(const std::string& device)
{
	try
	{
		_devicesMutex.lock();
		if(_devices.find(device) != _devices.end()) _devices.erase(device);
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_devicesMutex.unlock();
}
}
