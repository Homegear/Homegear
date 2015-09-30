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

#include "HelperFunctions.h"
#include "../BaseLib.h"
#include "sys/resource.h"

namespace BaseLib
{

HelperFunctions::HelperFunctions()
{
}

void HelperFunctions::init(Obj* baseLib)
{
	_bl = baseLib;
	checkEndianness();

	_asciiToBinaryTable[0] = 0;
	_asciiToBinaryTable[1] = 1;
	_asciiToBinaryTable[2] = 2;
	_asciiToBinaryTable[3] = 3;
	_asciiToBinaryTable[4] = 4;
	_asciiToBinaryTable[5] = 5;
	_asciiToBinaryTable[6] = 6;
	_asciiToBinaryTable[7] = 7;
	_asciiToBinaryTable[8] = 8;
	_asciiToBinaryTable[9] = 9;
	_asciiToBinaryTable[10] = 0;
	_asciiToBinaryTable[11] = 0;
	_asciiToBinaryTable[12] = 0;
	_asciiToBinaryTable[13] = 0;
	_asciiToBinaryTable[14] = 0;
	_asciiToBinaryTable[15] = 0;
	_asciiToBinaryTable[16] = 0;
	_asciiToBinaryTable[17] = 10;
	_asciiToBinaryTable[18] = 11;
	_asciiToBinaryTable[19] = 12;
	_asciiToBinaryTable[20] = 13;
	_asciiToBinaryTable[21] = 14;
	_asciiToBinaryTable[22] = 15;

	_binaryToASCIITable[0] = 0x30;
	_binaryToASCIITable[1] = 0x31;
	_binaryToASCIITable[2] = 0x32;
	_binaryToASCIITable[3] = 0x33;
	_binaryToASCIITable[4] = 0x34;
	_binaryToASCIITable[5] = 0x35;
	_binaryToASCIITable[6] = 0x36;
	_binaryToASCIITable[7] = 0x37;
	_binaryToASCIITable[8] = 0x38;
	_binaryToASCIITable[9] = 0x39;
	_binaryToASCIITable[0xA] = 0x41;
	_binaryToASCIITable[0xB] = 0x42;
	_binaryToASCIITable[0xC] = 0x43;
	_binaryToASCIITable[0xD] = 0x44;
	_binaryToASCIITable[0xE] = 0x45;
	_binaryToASCIITable[0xF] = 0x46;
}

HelperFunctions::~HelperFunctions()
{

}

int64_t HelperFunctions::getTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int32_t HelperFunctions::getTimeSeconds()
	{
		int32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();;
		if(time < 0) time = 0;
		return time;
	}

std::string HelperFunctions::getTimeString(int64_t time)
{
	const char timeFormat[] = "%x %X";
	std::time_t t;
	int32_t milliseconds;
	if(time > 0)
	{
		t = std::time_t(time / 1000);
		milliseconds = time % 1000;
	}
	else
	{
		const auto timePoint = std::chrono::system_clock::now();
		t = std::chrono::system_clock::to_time_t(timePoint);
		milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count() % 1000;
	}
	char timeString[50];
	strftime(&timeString[0], 50, &timeFormat[0], std::localtime(&t));
	std::ostringstream timeStream;
	timeStream << timeString << "." << std::setw(3) << std::setfill('0') << milliseconds;
	return timeStream.str();
}

std::string HelperFunctions::getTimeString(std::string format, int64_t time)
{
	std::time_t t;
	int32_t milliseconds;
	if(time > 0)
	{
		t = std::time_t(time / 1000);
		milliseconds = time % 1000;
	}
	else
	{
		const auto timePoint = std::chrono::system_clock::now();
		t = std::chrono::system_clock::to_time_t(timePoint);
		milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count() % 1000;
	}
	char timeString[50];
	strftime(&timeString[0], 50, format.c_str(), std::localtime(&t));
	std::ostringstream timeStream;
	timeStream << timeString << "." << std::setw(3) << std::setfill('0') << milliseconds;
	return timeStream.str();
}

int32_t HelperFunctions::getRandomNumber(int32_t min, int32_t max)
{
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<int32_t> distribution(min, max);
	return distribution(generator);
}

void HelperFunctions::memcpyBigEndian(char* to, const char* from, const uint32_t& length)
{
	try
	{
		if(_isBigEndian) memcpy(to, from, length);
		else
		{
			uint32_t last = length - 1;
			for(uint32_t i = 0; i < length; i++)
			{
				to[i] = from[last - i];
			}
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
}

void HelperFunctions::memcpyBigEndian(uint8_t* to, const uint8_t* from, const uint32_t& length)
{
	try
	{
		memcpyBigEndian((char*)to, (char*)from, length);
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
}

void HelperFunctions::memcpyBigEndian(int32_t& to, const std::vector<uint8_t>& from)
{
	try
	{
		to = 0; //Necessary if length is < 4
		if(from.empty()) return;
		uint32_t length = from.size();
		if(length > 4) length = 4;
		if(_isBigEndian) memcpyBigEndian(((uint8_t*)&to) + (4 - length), &from.at(0), length);
		else memcpyBigEndian(((uint8_t*)&to), &from.at(0), length);
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
}

void HelperFunctions::memcpyBigEndian(std::vector<uint8_t>& to, const int32_t& from)
{
	try
	{
		if(!to.empty()) to.clear();
		int32_t length = 4;
		if(from < 0) length = 4;
		else if(from < 256) length = 1;
		else if(from < 65536) length = 2;
		else if(from < 16777216) length = 3;
		to.resize(length, 0);
		if(_isBigEndian) memcpyBigEndian(&to.at(0), (uint8_t*)&from + (4 - length), length);
		else memcpyBigEndian(&to.at(0), (uint8_t*)&from, length);
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
}

std::pair<std::string, std::string> HelperFunctions::split(std::string string, char delimiter)
{
	int32_t pos = string.find_last_of(delimiter);
	if(pos == -1) return std::pair<std::string, std::string>(string, "");
	if((unsigned)pos + 1 >= string.size()) return std::pair<std::string, std::string>(string.substr(0, pos), "");
	return std::pair<std::string, std::string>(string.substr(0, pos), string.substr(pos + 1));
}

std::vector<std::string> HelperFunctions::splitAll(std::string string, char delimiter)
{
	std::vector<std::string> elements;
	std::stringstream stringStream(string);
	std::string element;
	while (std::getline(stringStream, element, delimiter))
	{
		elements.push_back(element);
	}
	return elements;
}

std::vector<uint8_t> HelperFunctions::hexToBin(const std::string& data)
{
	std::vector<uint8_t> bin;
	bin.reserve(data.size() / 2);
	for(uint32_t i = 0; i < data.size(); i+=2)
	{
		try	{ bin.push_back(std::stoi(data.substr(i, 2), 0, 16)); } catch(...) {}
	}
	return bin;
}

char HelperFunctions::getHexChar(int32_t nibble)
{
	try
	{
		if(nibble < 0 || nibble > 15) return 0;
		return _binaryToASCIITable[nibble];
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
    return 0;
}

std::string HelperFunctions::getHexString(const std::vector<uint8_t>& data)
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::setfill('0') << std::uppercase;
	for(std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
	{
		stringstream << std::setw(2) << (int32_t)(*i);
	}
	stringstream << std::dec;
	return stringstream.str();
}

std::string HelperFunctions::getHexString(const std::vector<char>& data)
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::setfill('0') << std::uppercase;
	for(std::vector<char>::const_iterator i = data.begin(); i != data.end(); ++i)
	{
		stringstream << std::setw(2) << (int32_t)((uint8_t)(*i));
	}
	stringstream << std::dec;
	return stringstream.str();
}

std::string HelperFunctions::getHexString(const std::string& data)
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::setfill('0') << std::uppercase;
	for(std::string::const_iterator i = data.begin(); i != data.end(); ++i)
	{
		stringstream << std::setw(2) << (int32_t)((uint8_t)(*i));
	}
	stringstream << std::dec;
	return stringstream.str();
}

std::string HelperFunctions::getHexString(int32_t number, int32_t width)
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::setfill('0');
	if(width > -1) stringstream << std::setw(width);
	stringstream << std::uppercase << number << std::dec;
	return stringstream.str();
}

std::vector<char> HelperFunctions::getBinary(std::string hexString)
{
    std::vector<char> binary;
    if(hexString.empty()) return binary;
	if(hexString.size() % 2 != 0) hexString = hexString.substr(1);
    for (std::string::const_iterator i = hexString.begin(); i != hexString.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = _asciiToBinaryTable[std::toupper(*i) - '0'] << 4;
        if(i + 1 != hexString.end() && isxdigit(*(i + 1))) byte += _asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

std::string HelperFunctions::getBinaryString(std::string hexString)
{
    std::string binary;
    if(hexString.empty()) return binary;
	if(hexString.size() % 2 != 0) hexString = hexString.substr(1);
    for (std::string::const_iterator i = hexString.begin(); i != hexString.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = _asciiToBinaryTable[std::toupper(*i) - '0'] << 4;
        if(i + 1 != hexString.end() && isxdigit(*(i + 1))) byte += _asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

std::vector<uint8_t> HelperFunctions::getUBinary(std::string hexString)
{
    std::vector<uint8_t> binary;
    if(hexString.empty()) return binary;
	if(hexString.size() % 2 != 0) hexString = hexString.substr(1);
    for (std::string::const_iterator i = hexString.begin(); i != hexString.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = (_asciiToBinaryTable[std::toupper(*i) - '0'] << 4);
        if(i + 1 != hexString.end() && isxdigit(*(i + 1))) byte += _asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

std::vector<uint8_t>& HelperFunctions::getUBinary(std::string hexString, uint32_t size, std::vector<uint8_t>& binary)
{
    if(hexString.empty()) return binary;
	if(size > hexString.size()) size = hexString.size();
	if(size % 2 != 0) size -= 1;
	if(size < 1) return binary;
    for (uint32_t i = 0; i < size; i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(hexString[i])) byte = (_asciiToBinaryTable[std::toupper(hexString[i]) - '0'] << 4);
        if(isxdigit(hexString[i + 1])) byte += _asciiToBinaryTable[std::toupper(hexString[i + 1]) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

std::vector<uint8_t> HelperFunctions::getUBinary(std::vector<uint8_t>& hexData)
{
    std::vector<uint8_t> binary;
    if(hexData.empty()) return binary;
    for (std::vector<uint8_t>::const_iterator i = hexData.begin(); i != hexData.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = (_asciiToBinaryTable[std::toupper(*i) - '0'] << 4);
        if(i + 1 != hexData.end() && isxdigit(*(i + 1))) byte += _asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

int32_t HelperFunctions::userID(std::string username)
{
	struct passwd pwd;
	struct passwd* pwdResult;
	int32_t bufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if(bufferSize < 0) bufferSize = 16384;
	std::vector<char> buffer(bufferSize);
	int32_t result = getpwnam_r(username.c_str(), &pwd, &buffer.at(0), buffer.size(), &pwdResult);
	if(!pwdResult)
	{
		if(result == 0) _bl->out.printError("User name " + username + " not found.");
		else _bl->out.printError("Error getting UID for user name " + username + ": " + std::string(strerror(result)));
		return -1;
	}
	return pwd.pw_uid;
}

int32_t HelperFunctions::groupID(std::string groupname)
{
	struct group grp;
	struct group* grpResult;
	int32_t bufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if(bufferSize < 0) bufferSize = 16384;
	std::vector<char> buffer(bufferSize);
	int32_t result = getgrnam_r(groupname.c_str(), &grp, &buffer.at(0), buffer.size(), &grpResult);
	if(!grpResult)
	{
		if(result == 0) _bl->out.printError("User name " + groupname + " not found.");
		else _bl->out.printError("Error getting GID for group name " + groupname + ": " + std::string(strerror(result)));
		return -1;
	}
	return grp.gr_gid;
}

pid_t HelperFunctions::system(std::string command, std::vector<std::string> arguments)
{
    pid_t pid;

    if(command.empty() || command.back() == '/') return -1;

    pid = fork();

    if(pid < 0) return pid;
    else if(pid == 0)
    {
    	//Child process
        struct rlimit limits;
    	if(getrlimit(RLIMIT_NOFILE, &limits) == -1)
    	{
    		_bl->out.printError("Error: Couldn't read rlimits.");
    		_exit(1);
    	}
        // Close all other descriptors for the safety sake.
        for(uint32_t i = 3; i < limits.rlim_cur; ++i) ::close(i);

        setsid();
        std::string programName = (command.find('/') == std::string::npos) ? command : command.substr(command.find_last_of('/') + 1);
        if(programName.empty()) _exit(1);
        char* argv[arguments.size() + 2];
        argv[0] = &programName[0]; //Dirty, but as argv is not modified, there are no problems. Since C++11 the data is null terminated.
        for(uint32_t i = 0; i < arguments.size(); i++)
        {
        	argv[i + 1] = &arguments[i][0];
        }
        argv[arguments.size() + 1] = nullptr;
        if(execv(command.c_str(), argv) == -1)
        {
        	_bl->out.printError("Error: Could not start program: " + std::string(strerror(errno)));
        }
        _exit(1);
    }

    //Parent process

    return pid;
}

int32_t HelperFunctions::exec(std::string command, std::string& output)
{
	FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;
    char buffer[128];
    int32_t bytesRead = 0;
    output.reserve(1024);
    while(!feof(pipe))
    {
    	if(fgets(buffer, 128, pipe) != 0)
    	{
    		if(output.size() + bytesRead > output.capacity()) output.reserve(output.capacity() + 1024);
    		output.insert(output.end(), buffer, buffer + strlen(buffer));
    	}
    }
    pclose(pipe);
    return 0;
}

void HelperFunctions::checkEndianness()
{
	union {
		uint32_t i;
		char c[4];
	} bint = {0x01020304};

	_isBigEndian = bint.c[0] == 1;
}

std::string HelperFunctions::getGNUTLSCertVerificationError(uint32_t errorCode)
{
	if(errorCode & GNUTLS_CERT_REVOKED) return "Certificate is revoked by its authority.";
	else if(errorCode & GNUTLS_CERT_SIGNER_NOT_FOUND) return "The certificate’s issuer is not known.";
	else if(errorCode & GNUTLS_CERT_SIGNER_NOT_CA) return "The certificate’s signer was not a CA.";
	else if(errorCode & GNUTLS_CERT_INSECURE_ALGORITHM) return "The certificate was signed using an insecure algorithm such as MD2 or MD5. These algorithms have been broken and should not be trusted.";
	else if(errorCode & GNUTLS_CERT_NOT_ACTIVATED) return "The certificate is not yet activated. ";
	else if(errorCode & GNUTLS_CERT_EXPIRED) return "The certificate has expired. ";
	//TODO: Add missing errors, when GNUTLS 3 is in Debian stable
	return "Unknown error code.";
}

std::string HelperFunctions::getGCRYPTError(int32_t errorCode)
{
	_gcryptBufferMutex.lock();
	gpg_strerror_r(errorCode, _gcryptBuffer, 1024);
	std::string result(_gcryptBuffer);
	_gcryptBufferMutex.unlock();
	return result;
}

bool HelperFunctions::isShortCLICommand(const std::string& command)
{
	int32_t spaceIndex = command.find(' ');
	if(spaceIndex < 0 && command.size() < 4) return true;
	else if(spaceIndex > 0 && spaceIndex < 4) return true;
	return false;
}

void* HelperFunctions::memrchr(const void* s, int c, size_t n)
{
	const unsigned char *cp;

    if(n != 0)
    {
		cp = (unsigned char *)s + n;
		do
		{
			if (*(--cp) == (unsigned char)c)
			return((void *)cp);
		} while (--n != 0);
    }
    return((void *)0);
}

}
