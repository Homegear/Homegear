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

#include "HelperFunctions.h"

bool HelperFunctions::_isBigEndian;
std::map<char, int32_t> HelperFunctions::_hexMap;

HelperFunctions::~HelperFunctions() {

}

void HelperFunctions::init()
{
	checkEndianness();

	_hexMap['0'] = 0x0;
	_hexMap['1'] = 0x1;
	_hexMap['2'] = 0x2;
	_hexMap['3'] = 0x3;
	_hexMap['4'] = 0x4;
	_hexMap['5'] = 0x5;
	_hexMap['6'] = 0x6;
	_hexMap['7'] = 0x7;
	_hexMap['8'] = 0x8;
	_hexMap['9'] = 0x9;
	_hexMap['A'] = 0xA;
	_hexMap['B'] = 0xB;
	_hexMap['C'] = 0xC;
	_hexMap['D'] = 0xD;
	_hexMap['E'] = 0xE;
	_hexMap['F'] = 0xF;
	_hexMap['a'] = 0xA;
	_hexMap['b'] = 0xB;
	_hexMap['c'] = 0xC;
	_hexMap['d'] = 0xD;
	_hexMap['e'] = 0xE;
	_hexMap['f'] = 0xF;
}

bool HelperFunctions::fileExists(std::string filename)
{
	std::ifstream in(filename.c_str());
	return in;
}

std::string HelperFunctions::getFileContent(std::string filename)
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
	throw(strerror(errno));
}

std::vector<std::string> HelperFunctions::getFiles(std::string path)
{
	std::vector<std::string> files;
	DIR* directory;
	struct dirent* entry;
	if((directory = opendir(path.c_str())) != 0)
	{
		while((entry = readdir(directory)) != 0)
		{
			if(entry->d_type == 8)
			{
				try
				{
					files.push_back(std::string(entry->d_name));
				}
				catch(const std::exception& ex)
				{
					Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(Exception& ex)
				{
					Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(...)
				{
					Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				}
			}
		}
	}
	else throw(Exception("Could not open directory."));
	return files;
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

int32_t HelperFunctions::getRandomNumber(int32_t min, int32_t max)
{
	try
	{
		std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
		std::uniform_int_distribution<int32_t> distribution(min, max);
		return distribution(generator);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

void HelperFunctions::memcpyBigEndian(char* to, char* from, const uint32_t& length)
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HelperFunctions::memcpyBigEndian(uint8_t* to, uint8_t* from, const uint32_t& length)
{
	try
	{
		memcpyBigEndian((char*)to, (char*)from, length);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HelperFunctions::memcpyBigEndian(int32_t& to, std::vector<uint8_t>& from)
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HelperFunctions::memcpyBigEndian(std::vector<uint8_t>& to, int32_t& from)
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::pair<std::string, std::string> HelperFunctions::split(std::string string, char delimiter)
{
	try
	{

		int32_t pos = string.find_last_of(delimiter);
		if(pos == -1) return std::pair<std::string, std::string>(string, "");
		if((unsigned)pos + 1 >= string.size()) return std::pair<std::string, std::string>(string.substr(0, pos), "");
		return std::pair<std::string, std::string>(string.substr(0, pos), string.substr(pos + 1));
	}
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::pair<std::string, std::string>();
}

std::string HelperFunctions::getHexString(const std::vector<uint8_t>& data)
{
	try
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
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

std::string HelperFunctions::getHexString(const std::vector<char>& data)
{
	try
	{
		std::ostringstream stringstream;
		stringstream << std::hex << std::setfill('0') << std::uppercase;
		for(std::vector<char>::const_iterator i = data.begin(); i != data.end(); ++i)
		{
			stringstream << std::setw(2) << (int32_t)(*i);
		}
		stringstream << std::dec;
		return stringstream.str();
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
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
	int32_t asciiToBinaryTable[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};
    std::vector<char> binary;
    for (std::string::const_iterator i = hexString.begin(); i != hexString.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = asciiToBinaryTable[std::toupper(*i) - '0'] << 4;
        if(i + 1 != hexString.end() && isxdigit(*(i + 1))) byte += asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

std::vector<uint8_t> HelperFunctions::getUBinary(std::string hexString)
{
	int32_t asciiToBinaryTable[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};
    std::vector<uint8_t> binary;
    for (std::string::const_iterator i = hexString.begin(); i != hexString.end(); i += 2)
    {
        uint8_t byte = 0;
        if(isxdigit(*i)) byte = (asciiToBinaryTable[std::toupper(*i) - '0'] << 4);
        if(i + 1 != hexString.end() && isxdigit(*(i + 1))) byte += asciiToBinaryTable[std::toupper(*(i + 1)) - '0'];
        binary.push_back(byte);
    }
    return binary;
}

void HelperFunctions::copyFile(std::string source, std::string dest)
{
	try
	{
		int in_fd = open(source.c_str(), O_RDONLY);
		if(in_fd == -1)
		{
			Output::printError("Error copying file " + source + ": " + strerror(errno));
			return;
		}
		int out_fd = open(dest.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
		if(out_fd == -1)
		{
			Output::printError("Error copying file " + source + ": " + strerror(errno));
			return;
		}
		char buf[8192];

		while (1) {
			ssize_t result = read(in_fd, &buf[0], sizeof(buf));
			if (!result) break;
			if(result == -1)
			{
				Output::printError("Error reading file " + source + ": " + strerror(errno));
				return;
			}
			if(write(out_fd, &buf[0], result) != result)
			{
				Output::printError("Error writing file " + dest + ": " + strerror(errno));
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string HelperFunctions::getSSLError(int32_t errorNumber)
{
	switch(errorNumber)
	{
	case SSL_ERROR_ZERO_RETURN:
		return "The connection has been closed.";
	case SSL_ERROR_WANT_READ:
		return "Read error: The operation did not complete.";
	case SSL_ERROR_WANT_WRITE:
		return "Write error: The operation did not complete.";
	case SSL_ERROR_WANT_CONNECT:
		return "The operation did not complete. Not connected yet.";
	case SSL_ERROR_WANT_ACCEPT:
		return "The operation did not complete. Not connected yet.";
	case SSL_ERROR_SYSCALL:
		errorNumber = ERR_get_error();
		if(errorNumber > 0) return std::string(ERR_reason_error_string(errorNumber));
		else return "Some I/O error occured.";
	case SSL_ERROR_SSL:
		errorNumber = ERR_get_error();
		if(errorNumber > 0) return std::string(ERR_reason_error_string(errorNumber));
		else return "Unknown error.";
	}
	return std::to_string(errorNumber);
}

std::string HelperFunctions::getSSLCertVerificationError(int32_t errorNumber)
{
	switch(errorNumber)
	{
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		return "Unable to get issuer certificate.";
	case X509_V_ERR_UNABLE_TO_GET_CRL:
		return "Unable to get certificate CRL.";
	case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		return "Unable to decrypt certificate's signature.";
	case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		return "Unable to decrypt CRL's signature.";
	case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		return "Unable to decode issuer public key.";
	case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		return "Certificate signature failure.";
	case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		return "CRL signature failure.";
	case X509_V_ERR_CERT_NOT_YET_VALID:
		return "Certificate is not yet valid.";
	case X509_V_ERR_CERT_HAS_EXPIRED:
		return "Certificate has expired.";
	case X509_V_ERR_CRL_NOT_YET_VALID:
		return "CRL is not yet valid.";
	case X509_V_ERR_CRL_HAS_EXPIRED:
		return "CRL has expired.";
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		return "Format error in certificate's notBefore field.";
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		return "Format error in certificate's notAfter field.";
	case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		return "Format error in CRL's lastUpdate field.";
	case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		return "Format error in CRL's nextUpdate field.";
	case X509_V_ERR_OUT_OF_MEM:
		return "Out of memory.";
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		return "Certificate is a self signed certificate. If you want to use it add the root certificate to your certificate store (recommended) or set \"verifyCertificate\" to false in main.conf (not recommended).";
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		return "Self signed certificate in certificate chain.";
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		return "Unable to get local issuer certificate.";
	case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		return "Unable to verify the first certificate.";
	case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		return "Certificate chain too long.";
	case X509_V_ERR_CERT_REVOKED:
		return "Certificate revoked.";
	case X509_V_ERR_INVALID_CA:
		return "Invalid CA certificate.";
	case X509_V_ERR_PATH_LENGTH_EXCEEDED:
		return "Path length constraint exceeded.";
	case X509_V_ERR_INVALID_PURPOSE:
		return "Unsupported certificate purpose.";
	case X509_V_ERR_CERT_UNTRUSTED:
		return "Certificate not trusted.";
	case X509_V_ERR_CERT_REJECTED:
		return "Certificate rejected.";
	case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		return "Subject issuer mismatch.";
	case X509_V_ERR_AKID_SKID_MISMATCH:
		return "Authority and subject key identifier mismatch.";
	case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		return "Authority and issuer serial number mismatch.";
	case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
		return "Key usage does not include certificate signing.";
	case X509_V_ERR_APPLICATION_VERIFICATION:
		return "Application verification failure.";
	}
	return "Unknown verification error. Error number: " + std::to_string(errorNumber);
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
		if(result == 0) Output::printError("User name " + username + " not found.");
		else Output::printError("Error getting UID for user name " + username + ": " + std::string(strerror(result)));
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
		if(result == 0) Output::printError("User name " + groupname + " not found.");
		else Output::printError("Error getting GID for group name " + groupname + ": " + std::string(strerror(result)));
		return -1;
	}
	return grp.gr_gid;
}
