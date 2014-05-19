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

#include "HM-CFG-LAN.h"

namespace BidCoS
{

HM_CFG_LAN::HM_CFG_LAN(std::shared_ptr<BaseLib::Systems::PhysicalDeviceSettings> settings) : BaseLib::Systems::PhysicalDevice(settings)
{
	signal(SIGPIPE, SIG_IGN);
	if(!settings->key.empty())
	{
		_useAES = true;
		BaseLib::Output::printInfo("Info: Enabling AES encryption for communication with HM-CFG-LAN.");
	}
	else
	{
		_useAES = false;
		BaseLib::Output::printInfo("Info: Disabling AES encryption for communication with HM-CFG-LAN.");
	}
}

HM_CFG_LAN::~HM_CFG_LAN()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		if(_useAES) openSSLCleanup();
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			BaseLib::Output::printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();

		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
		if(!bidCoSPacket) return;
		std::vector<char> data = bidCoSPacket->byteArraySigned();
		send(data, true);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::send(std::vector<char>& data, bool printData)
{
    try
    {
    	_sendMutex.lock();
    	if(BaseLib::Obj::ins->debugLevel > 3 && printData)
        {
            BaseLib::Output::printInfo("Info: Sending: " + BaseLib::HelperFunctions::getHexString(data));
        }
    	int32_t written = _socket.proofwrite(data);
    }
    catch(BaseLib::SocketOperationException& ex)
    {
    	BaseLib::Output::printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _sendMutex.unlock();
}

void HM_CFG_LAN::startListening()
{
	try
	{
		stopListening();
		if(_useAES) openSSLInit();
		_socket = BaseLib::SocketOperations(_settings->host, _settings->port, _settings->ssl, _settings->verifyCertificate);
		BaseLib::Output::printDebug("Connecting to HM-CFG-LAN device with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket.open();
		BaseLib::Output::printInfo("Connected to HM-CFG-LAN device with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&HM_CFG_LAN::listen, this);
		BaseLib::Threads::setThreadPriority(_listenThread.native_handle(), 45);
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::stopListening()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		_socket.close();
		if(_useAES) openSSLCleanup();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::openSSLPrintError()
{
	uint32_t errorCode = ERR_get_error();
	std::vector<char> buffer(256); //At least 120 bytes
	ERR_error_string(errorCode, &buffer.at(0));
	BaseLib::Output::printError("Error: " + std::string(&buffer.at(0)));
}

bool HM_CFG_LAN::openSSLInit()
{
	if(_settings->key.size() != 32)
	{
		BaseLib::Output::printError("Error: The AES key specified in physicaldevices.conf for communication with your HM-CFG-LAN has the wrong size.");
		return false;
	}
	_key = BaseLib::HelperFunctions::getUBinary(_settings->key);
	if(_key.size() != 16)
	{
		BaseLib::Output::printError("Error: The AES key specified in physicaldevices.conf for communication with your HM-CFG-LAN is not a valid hexadecimal string.");
		return false;
	}

	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);
	_ctxEncrypt = EVP_CIPHER_CTX_new();
	if(!_ctxEncrypt)
	{
		openSSLPrintError();
		openSSLCleanup();
		return false;
	}
	_ctxDecrypt = EVP_CIPHER_CTX_new();
	if(!_ctxDecrypt)
	{
		openSSLPrintError();
		openSSLCleanup();
		return false;
	}
	_aesInitialized = true;
	return true;
}

void HM_CFG_LAN::openSSLCleanup()
{
	if(!_aesInitialized) return;
	_aesInitialized = false;
	if(_ctxDecrypt) EVP_CIPHER_CTX_free(_ctxDecrypt);
	if(_ctxEncrypt)	EVP_CIPHER_CTX_free(_ctxEncrypt);
	EVP_cleanup();
	ERR_free_strings();
}

bool HM_CFG_LAN::encrypt()
{
	return false;
}

std::vector<uint8_t> HM_CFG_LAN::decrypt(std::vector<uint8_t>& data)
{
	std::vector<uint8_t> decryptedData(data.size());

	int length = 0;
	if(EVP_DecryptUpdate(_ctxDecrypt, &decryptedData.at(0), &length, &data.at(0), data.size()) != 1)
	{
		openSSLPrintError();
		_stopCallbackThread = true;
		return std::vector<uint8_t>();
	}
	uint32_t totalLength = length;
	EVP_DecryptFinal_ex(_ctxDecrypt, &decryptedData.at(0) + totalLength, &length);
	totalLength += length;

	return decryptedData;
}

void HM_CFG_LAN::listen()
{
    try
    {
    	uint32_t receivedBytes;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);

        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(200));
        		if(_stopCallbackThread) return;
        		continue;
        	}
        	try
			{
				receivedBytes = _socket.proofread(&buffer[0], bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex) { continue; }
			catch(BaseLib::SocketClosedException& ex)
			{
				BaseLib::Output::printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(30000));
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				BaseLib::Output::printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(30000));
				continue;
			}
        	if(receivedBytes == 0) continue;
        	if(receivedBytes == bufferMax)
        	{
        		BaseLib::Output::printError("Could not read from HM-CFG-LAN: Too much data.");
        		continue;
        	}

        	std::vector<uint8_t> data(&buffer.at(0), &buffer.at(0) + receivedBytes);

        	if(BaseLib::Obj::ins->debugLevel >= 5)
        	{
        		BaseLib::Output::printDebug("Debug: Packet received from HM-CFG-LAN:");
        		BaseLib::Output::printBinary(data);
        	}

        	processData(data);

			_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.at(0) == 'V')
		{
			if(!_useAES)
			{
				_stopCallbackThread = true;
				BaseLib::Output::printError("Error: Error communicating with HM-CFG-LAN. Device requires AES, but no AES key was specified in physicaldevices.conf.");
				return;
			}
			if(data.size() != 35)
			{
				_stopCallbackThread = true;
				BaseLib::Output::printError("Error: Error communicating with HM-CFG-LAN. Received IV has wrong size.");
				return;
			}
			_remoteIV.clear();
			std::string ivHex(&data.at(1), &data.at(1) + (data.size() - 3));
			_remoteIV = BaseLib::HelperFunctions::getUBinary(ivHex);
			if(_remoteIV.size() != 16)
			{
				_stopCallbackThread = true;
				BaseLib::Output::printError("Error: Error communicating with HM-CFG-LAN. Received IV is not in hexadecimal format.");
				return;
			}
			if(BaseLib::Obj::ins->debugLevel >= 5)
			{
				BaseLib::Output::printDebug("HM-CFG-LAN IV is: ");
				BaseLib::Output::printBinary(_remoteIV);
			}

			if(EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_remoteIV.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return;
			}

			std::vector<char> response = { 'V' };
			std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<int32_t> distribution(0, 15);
			_myIV.clear();
			for(int32_t i = 0; i < 32; i++)
			{
				int32_t nibble = distribution(generator);
				if((i % 2) == 0)
				{
					_myIV.push_back(nibble << 4);
				}
				else
				{
					_myIV.at(i / 2) |= nibble;
				}
				response.push_back(BaseLib::HelperFunctions::getHexChar(nibble));
			}
			response.push_back(0x0D);
			response.push_back(0x0A);

			if(BaseLib::Obj::ins->debugLevel >= 5)
			{
				BaseLib::Output::printDebug("Homegear IV is: ");
				BaseLib::Output::printBinary(_myIV);
			}

			if(EVP_DecryptInit_ex(_ctxDecrypt, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_myIV.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return;
			}

			send(response, true);
			return;
		}
		std::string packets;
		if(_useAES)
		{
			std::vector<uint8_t> decryptedData = decrypt(data);
			if(decryptedData.empty()) return;
			packets = std::string(&decryptedData.at(0), &decryptedData.at(0) + decryptedData.size());
		}
		else packets = std::string(&data.at(0), &data.at(0) + data.size());

		std::istringstream stringStream(packets);
		std::string packet;
		while(std::getline(stringStream, packet))
		{
			parsePacket(packet);
		}
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CFG_LAN::parsePacket(std::string& packet)
{
	try
	{
		if(packet.empty()) return;
		if(BaseLib::Obj::ins->debugLevel >= 5) BaseLib::Output::printDebug("Debug: Packet received from HM-CFG-LAN: " + packet);
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
