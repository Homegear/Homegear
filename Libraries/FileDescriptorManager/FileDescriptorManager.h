/* Copyright 2013 Sathya Laufer
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

#include "../Exception/Exception.h"
#include "../Output/Output.h"

#include <memory>
#include <string>
#include <map>
#include <mutex>

#include <unistd.h>
#include <sys/socket.h>

class FileDescriptor
{
public:
	virtual ~FileDescriptor() {}
	FileDescriptor() {}

	int32_t id = -1;
	int32_t descriptor = -1;
};

class FileDescriptorManager
{
public:
	virtual ~FileDescriptorManager() {}
	FileDescriptorManager() {};

	std::shared_ptr<FileDescriptor> add(int32_t fileDescriptor);
	void remove(std::shared_ptr<FileDescriptor> descriptor);
	void close(std::shared_ptr<FileDescriptor> descriptor);
	void shutdown(std::shared_ptr<FileDescriptor> descriptor);
	std::shared_ptr<FileDescriptor> get(int32_t fileDescriptor);
	bool isValid(int32_t fileDescriptor, int32_t id);
	bool isValid(std::shared_ptr<FileDescriptor> descriptor);
private:
	uint32_t _currentID = 0;
	std::mutex _descriptorsMutex;
	std::map<int32_t, std::shared_ptr<FileDescriptor>> _descriptors;
};

#endif
