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

#include "FileDescriptorManager.h"
#include "../BaseLib.h"

namespace BaseLib
{

FileDescriptorManager::FileDescriptorManager()
{
}

void FileDescriptorManager::init(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void FileDescriptorManager::dispose()
{
	_disposed = true;
	_descriptorsMutex.lock();
	for(FileDescriptors::iterator i = _descriptors.begin(); i != _descriptors.end(); ++i)
	{
		if(!i->second) continue;
		::close(i->second->descriptor);
	}
	_descriptors.clear();
	_descriptorsMutex.unlock();
}

PFileDescriptor FileDescriptorManager::add(int32_t fileDescriptor)
{
	try
	{
		if(fileDescriptor < 0 || _disposed) return PFileDescriptor(new FileDescriptor());
		_descriptorsMutex.lock();

		FileDescriptors::iterator descriptorIterator = _descriptors.find(fileDescriptor);
		if(descriptorIterator != _descriptors.end())
		{
			PFileDescriptor oldDescriptor = descriptorIterator->second;
			_bl->out.printInfo("Info: Old file descriptor " + std::to_string(fileDescriptor) + " was invalidated.");
			if(oldDescriptor->tlsSession)
			{
				if(_bl->settings.devLog()) _bl->out.printWarning("Devlog warning: Possibly dangerous operation: Cleaning up TLS session of closed socket descriptor.");
				gnutls_deinit(oldDescriptor->tlsSession);
				oldDescriptor->tlsSession = nullptr;
			}
			oldDescriptor->descriptor = -1;
		}
		PFileDescriptor descriptor(new FileDescriptor());
		descriptor->id = _currentID++;
		descriptor->descriptor = fileDescriptor;
		_descriptors[fileDescriptor] = descriptor;
		_descriptorsMutex.unlock();
		return descriptor;
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
	_descriptorsMutex.unlock();
	return PFileDescriptor(new FileDescriptor());
}

void FileDescriptorManager::remove(PFileDescriptor descriptor)
{
	try
	{
		if(!descriptor || descriptor->descriptor < 0) return;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(descriptor->descriptor);
		if(descriptorIterator != _descriptors.end() && descriptorIterator->second->id == descriptor->id)
		{
			if(descriptor->tlsSession) _bl->out.printWarning("Warning: Removed descriptor, but TLS session pointer is not empty.");
			descriptor->descriptor = -1;
			_descriptors.erase(descriptor->descriptor);
		}
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
	_descriptorsMutex.unlock();
}

void FileDescriptorManager::close(PFileDescriptor descriptor)
{
	try
	{
		if(!descriptor || descriptor->descriptor < 0) return;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(descriptor->descriptor);
		if(descriptorIterator != _descriptors.end() && descriptorIterator->second->id == descriptor->id)
		{
			_descriptors.erase(descriptor->descriptor);
			if(descriptor->tlsSession) gnutls_bye(descriptor->tlsSession, GNUTLS_SHUT_WR);
			::close(descriptor->descriptor);
			if(descriptor->tlsSession) gnutls_deinit(descriptor->tlsSession);
			descriptor->descriptor = -1;
			descriptor->tlsSession = nullptr;
		}
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
	_descriptorsMutex.unlock();
}

void FileDescriptorManager::shutdown(PFileDescriptor descriptor)
{
	try
	{
		if(!descriptor || descriptor->descriptor < 0) return;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(descriptor->descriptor);
		if(descriptorIterator != _descriptors.end() && descriptorIterator->second && descriptorIterator->second->id == descriptor->id)
		{
			_descriptors.erase(descriptor->descriptor);
			if(descriptor->tlsSession) gnutls_bye(descriptor->tlsSession, GNUTLS_SHUT_WR);
			//On SSL connections shutdown is not necessary and might even cause segfaults
			if(!descriptor->tlsSession) ::shutdown(descriptor->descriptor, 0);
			::close(descriptor->descriptor);
			if(descriptor->tlsSession) gnutls_deinit(descriptor->tlsSession);
			descriptor->descriptor = -1;
			descriptor->tlsSession = nullptr;
		}
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
	_descriptorsMutex.unlock();
}

void FileDescriptorManager::lock()
{
	_descriptorsMutex.lock();
}

void FileDescriptorManager::unlock()
{
	_descriptorsMutex.unlock();
}

PFileDescriptor FileDescriptorManager::get(int32_t fileDescriptor)
{
	try
	{
		if(fileDescriptor < 0) return PFileDescriptor();
		PFileDescriptor descriptor;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(descriptor->descriptor);
		if(descriptorIterator != _descriptors.end()) descriptor = descriptorIterator->second;
		_descriptorsMutex.unlock();
		return descriptor;
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
	_descriptorsMutex.unlock();
	return PFileDescriptor();
}

bool FileDescriptorManager::isValid(int32_t fileDescriptor, int32_t id)
{
	try
	{
		if(fileDescriptor < 0) return false;
		bool valid = false;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(fileDescriptor);
		if(descriptorIterator != _descriptors.end() && descriptorIterator->second->id == id) valid = true;
		_descriptorsMutex.unlock();
		return valid;
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
	_descriptorsMutex.unlock();
	return false;
}

bool FileDescriptorManager::isValid(PFileDescriptor descriptor)
{
	try
	{
		if(!descriptor || descriptor->descriptor < 0) return false;
		bool valid = false;
		_descriptorsMutex.lock();
		FileDescriptors::iterator descriptorIterator = _descriptors.find(descriptor->descriptor);
		if(descriptorIterator != _descriptors.end() && descriptorIterator->second->id == descriptor->id) valid = true;
		_descriptorsMutex.unlock();
		return valid;
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
	_descriptorsMutex.unlock();
	return false;
}
}
