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

#include "HelperFunctions.h"
#include "GD.h"

HelperFunctions::~HelperFunctions() {

}

void HelperFunctions::setThreadPriority(pthread_t thread, int32_t priority, int32_t policy)
{
	try
	{
		if(!GD::settings.prioritizeThreads()) return;
		if(policy != SCHED_FIFO && policy != SCHED_RR) priority = 0;
		if((policy == SCHED_FIFO || policy == SCHED_RR) && (priority < 1 || priority > 99)) throw Exception("Invalid thread priority: " + std::to_string(priority));
		sched_param schedParam;
		schedParam.sched_priority = priority;
		int32_t error;
		//Only use SCHED_FIFO or SCHED_RR
		if((error = pthread_setschedparam(thread, policy, &schedParam)) != 0)
		{
			if(error == EPERM)
			{
				printError("Could not set thread priority. The executing user does not have enough privileges. Please run \"ulimit -r 100\" before executing Homegear.");
			}
			else if(error == ESRCH) printError("Could not set thread priority. Thread could not be found.");
			else if(error == EINVAL) printError("Could not set thread priority: policy is not a recognized policy, or param does not make sense for the policy.");
			else printError("Error: Could not set thread priority to " + std::to_string(priority) + " Error: " + std::to_string(error));
			GD::settings.setPrioritizeThreads(false);
		}
		else printDebug("Debug: Thread priority successfully set to: " + std::to_string(priority), 7);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HelperFunctions::printThreadPriority()
{
	try
	{
		int32_t policy, error;
		sched_param param;
		if((error = pthread_getschedparam(pthread_self(), &policy, &param)) != 0) printError("Could not get thread priority. Error: " + std::to_string(error));

		printMessage("Thread policy: " + std::to_string(policy) + " Thread priority: " + std::to_string(param.sched_priority));
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

void HelperFunctions::memcpyBigEndian(char* to, char* from, const uint32_t& length)
{
	try
	{
		if(GD::bigEndian) memcpy(to, from, length);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		if(GD::bigEndian) memcpyBigEndian(((uint8_t*)&to) + (4 - length), &from.at(0), length);
		else memcpyBigEndian(((uint8_t*)&to), &from.at(0), length);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		if(GD::bigEndian) memcpyBigEndian(&to.at(0), (uint8_t*)&from + (4 - length), length);
		else memcpyBigEndian(&to.at(0), (uint8_t*)&from, length);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::pair<std::string, std::string>();
}

void HelperFunctions::printBinary(std::shared_ptr<std::vector<char>> data)
{
	try
	{
		if(!data) return;
		std::ostringstream stringstream;
		stringstream << std::hex << std::setfill('0') << std::uppercase;
		for(std::vector<char>::iterator i = data->begin(); i != data->end(); ++i)
		{
			stringstream << std::setw(2) << (int32_t)((uint8_t)(*i));
		}
		stringstream << std::dec;
		std::cout << stringstream.str() << std::endl;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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

void HelperFunctions::copyFile(std::string source, std::string dest)
{
	try
	{
		int in_fd = open(source.c_str(), O_RDONLY);
		if(in_fd == -1)
		{
			printError("Error copying file " + source + ": " + strerror(errno));
			return;
		}
		int out_fd = open(dest.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
		if(out_fd == -1)
		{
			printError("Error copying file " + source + ": " + strerror(errno));
			return;
		}
		char buf[8192];

		while (1) {
			ssize_t result = read(in_fd, &buf[0], sizeof(buf));
			if (!result) break;
			if(result == -1)
			{
				printError("Error reading file " + source + ": " + strerror(errno));
				return;
			}
			if(write(out_fd, &buf[0], result) != result)
			{
				printError("Error writing file " + dest + ": " + strerror(errno));
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HelperFunctions::printEx(std::string file, uint32_t line, std::string function, std::string what)
{
	if(!what.empty()) std::cerr << getTimeString() << " Error in file " << file << " line " << line << " in function " << function <<": " << what << std::endl;
	else std::cerr << getTimeString() << " Unknown error in file " << file << " line " << line << " in function " << function << "." << std::endl;
}

void HelperFunctions::printCritical(std::string errorString)
{
	if(GD::debugLevel < 1) return;
	std::cerr << getTimeString() << " " << errorString << std::endl;
}

void HelperFunctions::printError(std::string errorString)
{
	if(GD::debugLevel < 2) return;
	std::cerr << getTimeString() << " " << errorString << std::endl;
}

void HelperFunctions::printWarning(std::string errorString)
{
	if(GD::debugLevel < 3) return;
	std::cerr << getTimeString() << " " << errorString << std::endl;
}

void HelperFunctions::printInfo(std::string message)
{
	if(GD::debugLevel < 4) return;
	std::cout << getTimeString() << " " << message << std::endl;
}

void HelperFunctions::printDebug(std::string message, int32_t minDebugLevel)
{
	if(GD::debugLevel < minDebugLevel) return;
	std::cout << getTimeString() << " " << message << std::endl;
}

void HelperFunctions::printMessage(std::string message, int32_t minDebugLevel)
{
	if(GD::debugLevel < minDebugLevel) return;
	std::cout << getTimeString() << " " << message << std::endl;
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
