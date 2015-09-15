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

#include "Io.h"
#include "../BaseLib.h"

#include <sys/stat.h>

namespace BaseLib
{

Io::Io()
{
}

void Io::init(Obj* baseLib)
{
	_bl = baseLib;
}

Io::~Io()
{

}

bool Io::fileExists(std::string filename)
{
	std::ifstream in(filename.c_str());
	return in;
}

int32_t Io::isDirectory(std::string path, bool& result)
{
	struct stat s;
	result = false;
	if(stat(path.c_str(), &s) == 0)
	{
		if(s.st_mode & S_IFDIR) result = true;
		return 0;
	}
	return -1;
}

int32_t Io::getFileLastModifiedTime(const std::string& filename)
{
	struct stat attributes;
	if(stat(filename.c_str(), &attributes) == -1) return -1;
	return attributes.st_mtim.tv_sec;
}

std::string Io::getFileContent(std::string filename)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
	if(in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw Exception(strerror(errno));
}

std::vector<char> Io::getBinaryFileContent(std::string filename)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
	if(in)
	{
		std::vector<char> contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw Exception(strerror(errno));
}

std::vector<uint8_t> Io::getUBinaryFileContent(std::string filename)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
	if(in)
	{
		std::vector<uint8_t> contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read((char*)&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw Exception(strerror(errno));
}

void Io::writeFile(std::string& filename, std::string& content)
{
	std::ofstream file;
	file.open(filename);
	if(!file.is_open()) throw Exception("Could not open file.");
	file << content;
	file.close();
}

std::vector<std::string> Io::getFiles(std::string path, bool recursive)
{
	std::vector<std::string> files;
	DIR* directory;
	struct dirent* entry;
	struct stat statStruct;

	if(path.back() != '/') path += '/';
	if((directory = opendir(path.c_str())) != 0)
	{
		while((entry = readdir(directory)) != 0)
		{
			std::string name(entry->d_name);
			if(name == "." || name == "..") continue;
			if(stat((path + name).c_str(), &statStruct) == -1)
			{
				_bl->out.printWarning("Warning: Could not stat file \"" + path + name + "\": " + std::string(strerror(errno)));
				continue;
			}
			//Don't use dirent::d_type as it is not supported on all file systems. See http://nerdfortress.com/2008/09/19/linux-xfs-does-not-support-direntd_type/
			//Thanks to telkamp (https://github.com/Homegear/Homegear/issues/223)
			if(S_ISREG(statStruct.st_mode)) files.push_back(name);
			else if(S_ISDIR(statStruct.st_mode))
			{
				std::vector<std::string> subdirFiles = getFiles(path + name, recursive);
				for(std::vector<std::string>::iterator i = subdirFiles.begin(); i != subdirFiles.end(); ++i)
				{
					files.push_back(name + '/' + *i);
				}
			}
		}
		closedir(directory);
	}
	else throw(Exception("Could not open directory."));
	return files;
}

bool Io::copyFile(std::string source, std::string dest)
{
	try
	{
		int32_t in_fd = open(source.c_str(), O_RDONLY);
		if(in_fd == -1)
		{
			_bl->out.printError("Error copying file " + source + ": " + strerror(errno));
			return false;
		}

		unlink(dest.c_str());

		int32_t out_fd = open(dest.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
		if(out_fd == -1)
		{
			close(in_fd);
			_bl->out.printError("Error copying file " + source + ": " + strerror(errno));
			return false;
		}
		char buf[8192];

		while (true)
		{
			ssize_t result = read(in_fd, &buf[0], sizeof(buf));
			if (!result) break;
			if(result == -1)
			{
				close(in_fd);
				close(out_fd);
				_bl->out.printError("Error reading file " + source + ": " + strerror(errno));
				return false;
			}
			if(write(out_fd, &buf[0], result) != result)
			{
				close(in_fd);
				close(out_fd);
				_bl->out.printError("Error writing file " + dest + ": " + strerror(errno));
				return false;
			}
		}
		close(in_fd);
		close(out_fd);
		return true;
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
    return false;
}

bool Io::moveFile(std::string source, std::string dest)
{
	if(rename(source.c_str(), dest.c_str()) == 0) return true;
    return false;
}

bool Io::deleteFile(std::string file)
{
	if(remove(file.c_str()) == 0) return true;
	return false;
}

}
