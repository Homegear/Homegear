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

#include "IPhysicalInterface.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

IPhysicalInterface::IPhysicalInterface(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
	_settings.reset(new PhysicalInterfaceSettings());
	_fileDescriptor = std::shared_ptr<FileDescriptor>(new FileDescriptor());
	_gpioDescriptors[1] = std::shared_ptr<FileDescriptor>(new FileDescriptor());
	_gpioDescriptors[2] = std::shared_ptr<FileDescriptor>(new FileDescriptor());
	_gpioDescriptors[3] = std::shared_ptr<FileDescriptor>(new FileDescriptor());
}

IPhysicalInterface::IPhysicalInterface(BaseLib::Obj* baseLib, std::shared_ptr<PhysicalInterfaceSettings> settings) : IPhysicalInterface(baseLib)
{
	if(settings) _settings = settings;
}

IPhysicalInterface::~IPhysicalInterface()
{

}

void IPhysicalInterface::enableUpdateMode()
{
	throw Exception("Error: Method enableUpdateMode is not implemented.");
}

void IPhysicalInterface::disableUpdateMode()
{
	throw Exception("Error: Method disableUpdateMode is not implemented.");
}

void IPhysicalInterface::raisePacketReceived(std::shared_ptr<Packet> packet)
{
	try
	{
		std::thread t(&IPhysicalInterface::raisePacketReceivedThread, this, packet);
		BaseLib::Threads::setThreadPriority(_bl, t.native_handle(), 45);
		t.detach();
	}
    catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::raisePacketReceivedThread(std::shared_ptr<Packet> packet)
{
	try
	{
		std::vector<IPhysicalInterfaceEventSink*> eventHandlers;
		_eventHandlerMutex.lock();
		//We need to copy all elements. In packetReceived so much can happen, that _homeMaticDevicesMutex might deadlock
		for(std::forward_list<IEventSinkBase*>::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) eventHandlers.push_back((IPhysicalInterfaceEventSink*)(*i));
		}
		_eventHandlerMutex.unlock();
		_lastPacketReceived = HelperFunctions::getTime();
		for(std::vector<IPhysicalInterfaceEventSink*>::iterator i = eventHandlers.begin(); i != eventHandlers.end(); ++i)
		{
			(*i)->onPacketReceived(_settings->id, packet);
		}
	}
    catch(const std::exception& ex)
    {
    	_eventHandlerMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_eventHandlerMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_eventHandlerMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void IPhysicalInterface::setDevicePermission(int32_t userID, int32_t groupID)
{
	try
	{
		if(_settings->device.empty())
    	{
    		_bl->out.printError("Could not setup device " + _settings->type + " the device path is empty.");
    		return;
    	}
    	int32_t result = chown(_settings->device.c_str(), userID, groupID);
    	if(result == -1)
    	{
    		_bl->out.printError("Could not set owner for device " + _settings->device + ": " + std::string(strerror(errno)));
    	}
    	result = chmod(_settings->device.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    	if(result == -1)
    	{
    		_bl->out.printError("Could not set permissions for device " + _settings->device + ": " + std::string(strerror(errno)));
    	}
    }
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::openGPIO(uint32_t index, bool readOnly)
{
	try
	{
		if(!gpioDefined(index))
		{
			throw(Exception("Failed to open GPIO with index \"" + std::to_string(index) + "\" for device " + _settings->type + ": Not configured in physical devices' configuration file."));
		}
		if(_settings->gpio.at(index).path.empty()) getGPIOPath(index);
		if(_settings->gpio[index].path.empty()) throw(Exception("Failed to open value file for GPIO with index " + std::to_string(index) + " and device \"" + _settings->type + "\": Unable to retrieve path."));
		std::string path = _settings->gpio[index].path + "value";
		_gpioDescriptors[index] = _bl->fileDescriptorManager.add(open(path.c_str(), readOnly ? O_RDONLY : O_RDWR));
		if (_gpioDescriptors[index]->descriptor == -1) throw(Exception("Failed to open GPIO value file \"" + path + "\": " + std::string(strerror(errno))));
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::getGPIOPath(uint32_t index)
{
	try
	{
		if(!gpioDefined(index))
		{
			throw(Exception("Failed to get path for GPIO with index \"" + std::to_string(index) + "\": Not configured in physical devices' configuration file."));
		}
		if(!_settings->gpio.at(index).path.empty()) return;
		DIR* directory;
		struct dirent* entry;
		std::string gpioDir(_bl->settings.gpioPath());
		if((directory = opendir(gpioDir.c_str())) != 0)
		{
			while((entry = readdir(directory)) != 0)
			{
				struct stat dirStat;
				std::string dirName(gpioDir + std::string(entry->d_name));
				if(stat(dirName.c_str(), &dirStat) == -1)
				{
					_bl->out.printError("Error executing \"stat\" on entry \"" + dirName + "\": " + std::string(strerror(errno)));
					continue;
				}
				if(S_ISDIR(dirStat.st_mode))
				{
					try
					{
						int32_t pos = dirName.find_last_of('/');
						if(pos == std::string::npos || pos >= dirName.length()) continue;
						std::string subdirName = dirName.substr(pos + 1);
						if(subdirName.compare(0, 4, "gpio") != 0) continue;
						std::string number(std::to_string(_settings->gpio[index].number));
						if(subdirName.length() < 4 + number.length()) continue;
						if(subdirName.length() > 4 + number.length() && std::isdigit(subdirName.at(4 + number.length()))) continue;
						std::string number2(subdirName.substr(4, number.length()));
						if(number2 == number)
						{
							_bl->out.printDebug("Debug: GPIO path for GPIO with index " + std::to_string(index) + " and device " + _settings->type + " set to \"" + dirName + "\".");
							if(dirName.back() != '/') dirName.push_back('/');
							_settings->gpio[index].path = dirName;
							return;
						}
					}
					catch(const std::exception& ex)
					{
						_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(Exception& ex)
					{
						_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(...)
					{
						_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
					}
				}
			}
		}
		else throw(Exception("Could not open directory \"" + _bl->settings.gpioPath() + "\"."));
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::closeGPIO(uint32_t index)
{
	try
	{
		if(_gpioDescriptors.find(index) != _gpioDescriptors.end())
		{
			_bl->fileDescriptorManager.close(_gpioDescriptors.at(index));
		}
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::setGPIO(uint32_t index, bool value)
{
	try
	{
		if(!gpioOpen(index))
		{
			_bl->out.printError("Failed to set GPIO with index \"" + std::to_string(index) + "\": Device not open.");
			return;
		}
		std::string temp(std::to_string((int32_t)value));
		if(write(_gpioDescriptors[index]->descriptor, temp.c_str(), temp.size()) <= 0)
		{
			_bl->out.printError("Could not write GPIO with index " + std::to_string(index) + ".");
		}
		_bl->out.printDebug("Debug: GPIO " + std::to_string(_settings->gpio.at(index).number) + " set to " + std::to_string(value) + ".");
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::setGPIOPermission(uint32_t index, int32_t userID, int32_t groupID, bool readOnly)
{
	try
	{
		if(!gpioDefined(index))
    	{
    		_bl->out.printError("Error: Could not setup GPIO for device " + _settings->type + ": GPIO path for index " + std::to_string(index) + " is not set.");
    		return;
    	}
		if(_settings->gpio[index].path.empty()) getGPIOPath(index);
		if(_settings->gpio[index].path.empty()) throw(Exception("Error: Failed to get path for GPIO with index " + std::to_string(index) + " and device \"" + _settings->type + "\"."));
		std::string path = _settings->gpio[index].path + "value";
    	int32_t result = chown(path.c_str(), userID, groupID);
    	if(result == -1)
    	{
    		_bl->out.printError("Error: Could not set owner for GPIO value file " + path + ": " + std::string(strerror(errno)));
    	}
    	result = chmod(path.c_str(), readOnly ? (S_IRUSR | S_IRGRP) : (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    	if(result == -1)
    	{
    		_bl->out.printError("Error: Could not set permissions for GPIO value file " + path + ": " + std::string(strerror(errno)));
    	}
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool IPhysicalInterface::gpioDefined(uint32_t index)
{
	try
	{
		if(_settings->gpio.find(index) == _settings->gpio.end() || _settings->gpio.at(index).number <= 0) return false;
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return true;
}

bool IPhysicalInterface::gpioOpen(uint32_t index)
{
	try
	{
		if(_gpioDescriptors.find(index) == _gpioDescriptors.end() || !_gpioDescriptors.at(index) || _gpioDescriptors.at(index)->descriptor == -1) return false;
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return true;
}

void IPhysicalInterface::exportGPIO(uint32_t index)
{
	try
	{
		if(!gpioDefined(index))
		{
			_bl->out.printError("Error: Failed to export GPIO with index " + std::to_string(index) + " for device \"" + _settings->type + ": GPIO not defined in physicel devices' settings.");
			return;
		}
		if(_settings->gpio[index].path.empty()) getGPIOPath(index);
		std::string path;
		std::shared_ptr<FileDescriptor> fileDescriptor;
		std::string temp(std::to_string(_settings->gpio[index].number));
		if(!_settings->gpio[index].path.empty())
		{
			_bl->out.printDebug("Debug: Unexporting GPIO with index " + std::to_string(index) + " and number " + std::to_string(_settings->gpio[index].number) + " for device \"" + _settings->type + "\".");
			path = _bl->settings.gpioPath() + "unexport";
			fileDescriptor = _bl->fileDescriptorManager.add(open(path.c_str(), O_WRONLY));
			if (fileDescriptor->descriptor == -1) throw(Exception("Could not unexport GPIO with index " + std::to_string(index) + " for device \"" + _settings->type + "\". Failed to write to unexport file: " + std::string(strerror(errno))));
			if(write(fileDescriptor->descriptor, temp.c_str(), temp.size()) == -1)
			{
				_bl->out.printError("Error: Could not unexport GPIO with index " + std::to_string(index) + " and number " + temp + " for device \"" + _settings->type + "\": " + std::string(strerror(errno)));
			}
			_bl->fileDescriptorManager.close(fileDescriptor);
			_settings->gpio[index].path.clear();
		}

		_bl->out.printDebug("Debug: Exporting GPIO with index " + std::to_string(index) + " and number " + std::to_string(_settings->gpio[index].number) + " for device \"" + _settings->type + "\".");
		path = _bl->settings.gpioPath() + "export";
		fileDescriptor = _bl->fileDescriptorManager.add(open(path.c_str(), O_WRONLY));
		if (fileDescriptor->descriptor == -1) throw(Exception("Error: Could not export GPIO with index " + std::to_string(index) + " for device \"" + _settings->type + "\". Failed to write to export file: " + std::string(strerror(errno))));
		if(write(fileDescriptor->descriptor, temp.c_str(), temp.size()) == -1)
		{
			_bl->out.printError("Error: Could not export GPIO with index " + std::to_string(index) + " and number " + temp + " for device \"" + _settings->type + "\": " + std::string(strerror(errno)));
		}
		_bl->fileDescriptorManager.close(fileDescriptor);
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::setGPIODirection(uint32_t index, GPIODirection::Enum direction)
{
	try
	{
		if(!gpioDefined(index))
		{
			_bl->out.printError("Failed to set direction for GPIO with index \"" + std::to_string(index) + "\": GPIO not defined in physicel devices' settings.");
			return;
		}
		if(_settings->gpio[index].path.empty()) getGPIOPath(index);
		if(_settings->gpio[index].path.empty()) throw(Exception("Failed to open direction file for GPIO with index " + std::to_string(index) + " and device \"" + _settings->type + "\": Unable to retrieve path."));
		std::string path(_settings->gpio[index].path + "direction");
		std::shared_ptr<FileDescriptor> fileDescriptor = _bl->fileDescriptorManager.add(open(path.c_str(), O_WRONLY));
		if (fileDescriptor->descriptor == -1) throw(Exception("Could not write to direction file (" + path + ") of GPIO with index " + std::to_string(index) + ": " + std::string(strerror(errno))));
		std::string temp((direction == GPIODirection::OUT) ? "out" : "in");
		if(write(fileDescriptor->descriptor, temp.c_str(), temp.size()) <= 0)
		{
			_bl->out.printError("Could not write to direction file \"" + path + "\": " + std::string(strerror(errno)));
		}
		_bl->fileDescriptorManager.close(fileDescriptor);
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void IPhysicalInterface::setGPIOEdge(uint32_t index, GPIOEdge::Enum edge)
{
	try
	{
		if(!gpioDefined(index))
		{
			_bl->out.printError("Failed to set edge for GPIO with index \"" + std::to_string(index) + "\": GPIO not defined in physicel devices' settings.");
			return;
		}
		if(_settings->gpio[index].path.empty()) getGPIOPath(index);
		if(_settings->gpio[index].path.empty()) throw(Exception("Failed to open edge file for GPIO with index " + std::to_string(index) + " and device \"" + _settings->type + "\": Unable to retrieve path."));
		std::string path(_settings->gpio[index].path + "edge");
		std::shared_ptr<FileDescriptor> fileDescriptor = _bl->fileDescriptorManager.add(open(path.c_str(), O_WRONLY));
		if (fileDescriptor->descriptor == -1) throw(Exception("Could not write to edge file (" + path + ") of GPIO with index " + std::to_string(index) + ": " + std::string(strerror(errno))));
		std::string temp((edge == GPIOEdge::RISING) ? "rising" : ((edge == GPIOEdge::FALLING) ? "falling" : "both"));
		if(write(fileDescriptor->descriptor, temp.c_str(), temp.size()) <= 0)
		{
			_bl->out.printError("Could not write to edge file \"" + path + "\": " + std::string(strerror(errno)));
		}
		_bl->fileDescriptorManager.close(fileDescriptor);
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
}
