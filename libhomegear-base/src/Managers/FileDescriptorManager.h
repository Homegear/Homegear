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

#ifndef FILEDESCRIPTORMANAGER_H_
#define FILEDESCRIPTORMANAGER_H_

#include <memory>
#include <string>
#include <map>
#include <mutex>

#include <unistd.h>
#include <sys/socket.h>

#include <gnutls/gnutls.h>

namespace BaseLib
{

class Obj;

class FileDescriptor
{
public:
	virtual ~FileDescriptor() {}
	FileDescriptor() {}

	int32_t id = -1;
	int32_t descriptor = -1;
	gnutls_session_t tlsSession = nullptr;
};

typedef std::shared_ptr<FileDescriptor> PFileDescriptor;
typedef std::map<int32_t, PFileDescriptor> FileDescriptors;

class FileDescriptorManager
{
public:
	FileDescriptorManager();
	virtual ~FileDescriptorManager() {}
	void init(BaseLib::Obj* baseLib);
	void dispose();

	virtual PFileDescriptor add(int32_t fileDescriptor);
	virtual void remove(PFileDescriptor descriptor);
	virtual void close(PFileDescriptor descriptor);
	virtual void shutdown(PFileDescriptor descriptor);
	virtual PFileDescriptor get(int32_t fileDescriptor);
	virtual bool isValid(int32_t fileDescriptor, int32_t id);
	virtual bool isValid(PFileDescriptor descriptor);
	virtual void lock();
	virtual void unlock();
private:
	bool _disposed = false;
	BaseLib::Obj* _bl = nullptr;
	uint32_t _currentID = 0;
	std::mutex _descriptorsMutex;
	FileDescriptors _descriptors;
};
}
#endif
