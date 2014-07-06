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

#include "HM-LGW.h"
#include "../GD.h"

namespace BidCoS
{
CRC16::CRC16()
{
	if(_crcTable.empty()) initCRCTable();
}

void CRC16::initCRCTable()
{
	uint32_t bit, crc;

	for (uint32_t i = 0; i < 256; i++)
	{
		crc = i << 8;

		for (uint32_t j = 0; j < 8; j++) {

			bit = crc & 0x8000;
			crc <<= 1;
			if(bit) crc ^= 0x8005;
		}

		crc &= 0xFFFF;
		_crcTable[i]= crc;
	}
}

uint16_t CRC16::calculate(const std::vector<char>& data, bool ignoreLastTwoBytes)
{
	int32_t size = ignoreLastTwoBytes ? data.size() - 2 : data.size();
	uint16_t crc = 0xd77f;
	for(int32_t i = 0; i < size; i++)
	{
		crc = (crc << 8) ^ _crcTable[((crc >> 8) & 0xff) ^ (uint8_t)data[i]];
	}

    return crc;
}

uint16_t CRC16::calculate(const std::vector<uint8_t>& data, bool ignoreLastTwoBytes)
{
	int32_t size = ignoreLastTwoBytes ? data.size() - 2 : data.size();
	uint16_t crc = 0xd77f;
	for(int32_t i = 0; i < size; i++)
	{
		crc = (crc << 8) ^ _crcTable[((crc >> 8) & 0xff) ^ data[i]];
	}

    return crc;
}

HM_LGW::HM_LGW(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IBidCoSInterface(settings)
{
	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));
	_socketKeepAlive = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(!settings)
	{
		GD::out.printCritical("Critical: Error initializing HM-LGW. Settings pointer is empty.");
		return;
	}
	if(settings->lanKey.empty())
	{
		GD::out.printError("Error: No security key specified for HM-LGW in physicalinterfaces.conf.");
		return;
	}

	if(!settings->rfKey.empty())
	{
		_rfKey = _bl->hf.getUBinary(settings->rfKey);
		if(_rfKey.size() != 16)
		{
			GD::out.printError("Error: The RF AES key specified in physicalinterfaces.conf for communication with your BidCoS devices is not a valid hexadecimal string.");
			_rfKey.clear();
		}
	}

	if(!settings->oldRFKey.empty())
	{
		_oldRFKey = _bl->hf.getUBinary(settings->oldRFKey);
		if(_oldRFKey.size() != 16)
		{
			GD::out.printError("Error: The old RF AES key specified in physicalinterfaces.conf for communication with your BidCoS devices is not a valid hexadecimal string.");
			_oldRFKey.clear();
		}
	}

	if(!_rfKey.empty() && settings->currentRFKeyIndex == 0)
	{
		GD::out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is \"0\". That means, the default AES key will be used (not yours!).");
		_rfKey.clear();
	}

	if(!_oldRFKey.empty() && settings->currentRFKeyIndex == 1)
	{
		GD::out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is \"1\" but \"OldRFKey\" is specified. That is not possible. Increase the key index to \"2\".");
		_oldRFKey.clear();
	}

	if(!_oldRFKey.empty() && _rfKey.empty())
	{
		_oldRFKey.clear();
		if(settings->currentRFKeyIndex > 0)
		{
			GD::out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is greater than \"0\" but no AES key is specified. Setting it to \"0\".");
			settings->currentRFKeyIndex = 0;
		}
	}

	if(settings->currentRFKeyIndex > 253)
	{
		GD::out.printError("Error: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is greater than \"253\". That is not allowed.");
		settings->currentRFKeyIndex = 253;
	}
}

HM_LGW::~HM_LGW()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		if(_listenThreadKeepAlive.joinable()) _listenThreadKeepAlive.join();
		openSSLCleanup();
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string HM_LGW::getPeerInfoPacket(PeerInfo& peerInfo)
{
	try
	{
		std::string packetHex = std::string("+") + BaseLib::HelperFunctions::getHexString(peerInfo.address, 6) + ",";
		if(!peerInfo.aesChannels.empty())
		{
			packetHex += peerInfo.wakeUp ? "03," : "01,";
			packetHex += BaseLib::HelperFunctions::getHexString(peerInfo.keyIndex, 2) + ",";
			packetHex += BaseLib::HelperFunctions::getHexString(peerInfo.getAESChannelMap()) + ",";
		}
		else
		{
			//When no AES is used, wakeUp is handled by Homegear
			packetHex += "00,00,";
		}
		packetHex += "\r\n";
		return packetHex;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

void HM_LGW::addPeer(PeerInfo peerInfo)
{
	try
	{
		if(peerInfo.address == 0) return;
		_peersMutex.lock();
		//Remove old peer first. removePeer() is not called, so we don't need to unlock _peersMutex
		if(_peers.find(peerInfo.address) != _peers.end()) _peers.erase(peerInfo.address);
		if(_initComplete)
		{
			std::string packetHex = std::string("-") + BaseLib::HelperFunctions::getHexString(peerInfo.address, 6) + "\r\n";
			send(packetHex);
		}
		_peers[peerInfo.address] = peerInfo;
		std::string packetHex = getPeerInfoPacket(peerInfo);
		if(_initComplete) send(packetHex);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
}

void HM_LGW::addPeers(std::vector<PeerInfo>& peerInfos)
{
	try
	{
		_peersMutex.lock();
		for(std::vector<PeerInfo>::iterator i = peerInfos.begin(); i != peerInfos.end(); ++i)
		{
			addPeer(*i);
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
}

void HM_LGW::sendPeers()
{
	try
	{
		_peersMutex.lock();
		for(std::map<int32_t, PeerInfo>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			//send(getPeerInfoPacket(i->second));
		}
		_initComplete = true; //Init complete is set here within _peersMutex, so there is no conflict with addPeer() and peers are not sent twice
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
}

void HM_LGW::removePeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end()) _peers.erase(address);
		if(_initComplete)
		{
			std::string packetHex = std::string("-") + BaseLib::HelperFunctions::getHexString(address, 6) + "\r\n";
			send(packetHex);
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
}

void HM_LGW::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			GD::out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();

		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
		if(!bidCoSPacket) return;
		if(bidCoSPacket->messageType() == 0x02 && packet->senderAddress() == _myAddress && bidCoSPacket->controlByte() == 0x80 && bidCoSPacket->payload()->size() == 1 && bidCoSPacket->payload()->at(0) == 0)
		{
			GD::out.printDebug("Debug: HM-LGW: Ignoring ACK packet.", 6);
			_lastPacketSent = BaseLib::HelperFunctions::getTime();
			return;
		}

		int64_t currentTimeMilliseconds = BaseLib::HelperFunctions::getTime();
		uint32_t currentTime = currentTimeMilliseconds & 0xFFFFFFFF;
		std::string packetString = packet->hexString();
		if(_bl->debugLevel >= 4) GD::out.printInfo("Info: Sending (" + _settings->id + "): " + packetString);
		std::string hexString = "S" + BaseLib::HelperFunctions::getHexString(currentTime, 8) + ",00,00000000,01," + BaseLib::HelperFunctions::getHexString(currentTimeMilliseconds - _startUpTime, 8) + "," + packetString.substr(2) + "\r\n";
		send(hexString, false);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::send(std::string hexString, bool raw)
{
	try
    {
		if(hexString.empty()) return;
		std::vector<char> data(&hexString.at(0), &hexString.at(0) + hexString.size());
		send(data, raw);
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::send(std::vector<char>& data, bool raw)
{
    try
    {
    	if(data.size() < 3) return; //Otherwise error in printInfo
    	std::vector<char> encryptedData;
    	if(!raw) encryptedData = encrypt(data);
    	_sendMutex.lock();
    	if(!_socket->connected() || _stopped)
    	{
    		GD::out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
    		_sendMutex.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5)
        {
            GD::out.printDebug("Debug: Sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
        }
    	(!raw) ? _socket->proofwrite(encryptedData) : _socket->proofwrite(data);
    	 _sendMutex.unlock();
    	 return;
    }
    catch(BaseLib::SocketOperationException& ex)
    {
    	GD::out.printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stopped = true;
    _sendMutex.unlock();
}

void HM_LGW::sendKeepAlive(std::vector<char>& data, bool raw)
{
    try
    {
    	if(data.size() < 3) return; //Otherwise error in printInfo
    	std::vector<char> encryptedData;
    	if(!raw) encryptedData = encryptKeepAlive(data);
    	_sendMutexKeepAlive.lock();
    	if(!_socketKeepAlive->connected() || _stopped)
    	{
    		GD::out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->portKeepAlive + "): " + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
    		_sendMutexKeepAlive.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5)
        {
            GD::out.printDebug(std::string("Debug: Sending (Port " + _settings->portKeepAlive + "): ") + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
        }
    	(!raw) ? _socketKeepAlive->proofwrite(encryptedData) : _socketKeepAlive->proofwrite(data);
    	 _sendMutexKeepAlive.unlock();
    	 return;
    }
    catch(BaseLib::SocketOperationException& ex)
    {
    	GD::out.printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stopped = true;
    _sendMutexKeepAlive.unlock();
}

void HM_LGW::startListening()
{
	try
	{
		stopListening();
		openSSLInit();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->verifyCertificate));
		_socket->setReadTimeout(1000000);
		_socketKeepAlive = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->portKeepAlive, _settings->ssl, _settings->verifyCertificate));
		_socketKeepAlive->setReadTimeout(1000000);
		GD::out.printDebug("Connecting to HM-LGW with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		_listenThread = std::thread(&HM_LGW::listen, this);
		BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), 45);
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::reconnect()
{
	try
	{
		_socket->close();
		_socketKeepAlive->close();
		openSSLInit();
		createInitCommandQueue();
		_initCompleteKeepAlive = false;
		GD::out.printDebug("Connecting to HM-LGW device with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_socketKeepAlive->open();
		GD::out.printInfo("Connected to HM-LGW device with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		if(_listenThreadKeepAlive.joinable()) _listenThreadKeepAlive.join();
		_stopCallbackThread = false;
		_socket->close();
		_socketKeepAlive->close();
		openSSLCleanup();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
		_sendMutexKeepAlive.unlock();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::createInitCommandQueue()
{
	try
	{
		_initComplete = false;
		_packetIndex = 0;
		_initCommandQueue.clear();

		int32_t i = 0;
		while(!GD::family->getCentral() && i < 30)
		{
			GD::out.printDebug("Debug: HM-LGW: Waiting for central to load.");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			i++;
		}
		if(!GD::family->getCentral())
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: Could not get central address for HM-LGW. Stopping listening.");
			return;
		}

		_myAddress = GD::family->getCentral()->physicalAddress();

		_initCommandQueue.push_back(std::vector<char>{ 0, 3 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 3 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 2 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 0xA, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 0xA, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 9, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 9, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 0xA, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 0, 0xE });
		_initCommandQueue.push_back(std::vector<char>{ 0, 9, 0 });
		_initCommandQueue.push_back(std::vector<char>{ 1, 3 });
		if(_settings->currentRFKeyIndex > 1) _initCommandQueue.push_back(std::vector<char>{ 1, 0xF });
		_initCommandQueue.push_back(std::vector<char>{ 1, 0 });
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::openSSLPrintError()
{
	uint32_t errorCode = ERR_get_error();
	std::vector<char> buffer(256); //At least 120 bytes
	ERR_error_string(errorCode, &buffer.at(0));
	GD::out.printError("Error: " + std::string(&buffer.at(0)));
}

bool HM_LGW::openSSLInit()
{
	openSSLCleanup();

	if(_settings->lanKey.empty())
	{
		GD::out.printError("Error: No AES key specified in physicalinterfaces.conf for communication with your HM-LGW.");
		return false;
	}
	_key.resize(16);
	MD5((uint8_t*)_settings->lanKey.c_str(), _settings->lanKey.size(), &_key.at(0));

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
	_ctxEncryptKeepAlive = EVP_CIPHER_CTX_new();
	if(!_ctxEncryptKeepAlive)
	{
		openSSLPrintError();
		openSSLCleanup();
		return false;
	}
	_ctxDecryptKeepAlive = EVP_CIPHER_CTX_new();
	if(!_ctxDecryptKeepAlive)
	{
		openSSLPrintError();
		openSSLCleanup();
		return false;
	}
	_aesInitialized = true;
	_aesExchangeComplete = false;
	_aesExchangeKeepAliveComplete = false;
	return true;
}

void HM_LGW::openSSLCleanup()
{
	if(!_aesInitialized) return;
	_aesInitialized = false;
	if(_ctxDecrypt) EVP_CIPHER_CTX_free(_ctxDecrypt);
	if(_ctxEncrypt)	EVP_CIPHER_CTX_free(_ctxEncrypt);
	if(_ctxDecryptKeepAlive) EVP_CIPHER_CTX_free(_ctxDecryptKeepAlive);
	if(_ctxEncryptKeepAlive)	EVP_CIPHER_CTX_free(_ctxEncryptKeepAlive);
	EVP_cleanup();
	ERR_free_strings();
	_myIV.clear();
	_remoteIV.clear();
	_myIVKeepAlive.clear();
	_remoteIVKeepAlive.clear();
	_aesExchangeComplete = false;
	_aesExchangeKeepAliveComplete = false;
}

std::vector<char> HM_LGW::encrypt(std::vector<char>& data)
{
	std::vector<char> encryptedData(data.size());
	if(!_ctxEncrypt) return encryptedData;

	int length = 0;
	if(EVP_EncryptUpdate(_ctxEncrypt, (uint8_t*)&encryptedData.at(0), &length, (uint8_t*)&data.at(0), data.size()) != 1)
	{
		openSSLPrintError();
		_stopCallbackThread = true;
		return std::vector<char>();
	}
	EVP_EncryptFinal_ex(_ctxEncrypt, (uint8_t*)&encryptedData.at(0) + length, &length);

	return encryptedData;
}

std::vector<uint8_t> HM_LGW::decrypt(std::vector<uint8_t>& data)
{
	std::vector<uint8_t> decryptedData(data.size());
	if(!_ctxDecrypt) return decryptedData;

	int length = 0;
	if(EVP_DecryptUpdate(_ctxDecrypt, &decryptedData.at(0), &length, &data.at(0), data.size()) != 1)
	{
		openSSLPrintError();
		_stopCallbackThread = true;
		return std::vector<uint8_t>();
	}
	EVP_DecryptFinal_ex(_ctxDecrypt, &decryptedData.at(0) + length, &length);

	return decryptedData;
}

std::vector<char> HM_LGW::encryptKeepAlive(std::vector<char>& data)
{
	std::vector<char> encryptedData(data.size());
	if(!_ctxEncryptKeepAlive) return encryptedData;

	int length = 0;
	if(EVP_EncryptUpdate(_ctxEncryptKeepAlive, (uint8_t*)&encryptedData.at(0), &length, (uint8_t*)&data.at(0), data.size()) != 1)
	{
		openSSLPrintError();
		_stopCallbackThread = true;
		return std::vector<char>();
	}
	EVP_EncryptFinal_ex(_ctxEncryptKeepAlive, (uint8_t*)&encryptedData.at(0) + length, &length);

	return encryptedData;
}

std::vector<uint8_t> HM_LGW::decryptKeepAlive(std::vector<uint8_t>& data)
{
	std::vector<uint8_t> decryptedData(data.size());
	if(!_ctxDecryptKeepAlive) return decryptedData;

	int length = 0;
	if(EVP_DecryptUpdate(_ctxDecryptKeepAlive, &decryptedData.at(0), &length, &data.at(0), data.size()) != 1)
	{
		openSSLPrintError();
		_stopCallbackThread = true;
		return std::vector<uint8_t>();
	}
	EVP_DecryptFinal_ex(_ctxDecryptKeepAlive, &decryptedData.at(0) + length, &length);

	return decryptedData;
}

void HM_LGW::sendKeepAlivePacket1()
{
	try
    {
		if(!_initCommandQueue.empty()) return;
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastKeepAlive1 >= 5)
		{
			if(_lastKeepAliveResponse1 < _lastKeepAlive1)
			{
				_lastKeepAliveResponse1 = _lastKeepAlive1;
				_stopped = true;
				return;
			}

			_lastKeepAlive1 = BaseLib::HelperFunctions::getTimeSeconds();
			std::vector<char> packet;
			std::vector<char> payload{ 0, 8 };
			buildPacket(packet, payload);
			_packetIndex++;
			send(packet, false);
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::sendKeepAlivePacket2()
{
	try
    {
		if(!_initCompleteKeepAlive) return;
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastKeepAlive2 >= 10)
		{
			if(_lastKeepAliveResponse2 < _lastKeepAlive2)
			{
				_lastKeepAliveResponse2 = _lastKeepAlive2;
				_stopped = true;
				return;
			}

			_lastKeepAlive2 = BaseLib::HelperFunctions::getTimeSeconds();
			std::vector<char> packet = { 'K', _bl->hf.getHexChar(_packetIndexKeepAlive >> 4), _bl->hf.getHexChar(_packetIndexKeepAlive & 0xF), '\r', '\n' };
			sendKeepAlive(packet, false);
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::sendTimePacket()
{
	try
    {
		const auto timePoint = std::chrono::system_clock::now();
		time_t t = std::chrono::system_clock::to_time_t(timePoint);
		tm* localTime = std::localtime(&t);
		uint32_t time = (uint32_t)t;
		std::vector<char> payload{ 0, 0xE };
		payload.push_back(time >> 24);
		payload.push_back((time >> 16) & 0xFF);
		payload.push_back((time >> 8) & 0xFF);
		payload.push_back(time & 0xFF);
		payload.push_back(localTime->tm_gmtoff / 1800);
		std::vector<char> packet;
		buildPacket(packet, payload);
		_packetIndex++;
		send(packet, false);
		_lastTimePacket = BaseLib::HelperFunctions::getTimeSeconds();
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::listen()
{
    try
    {
    	createInitCommandQueue(); //Called here in a seperate thread so that startListening is not blocking Homegear
    	_listenThreadKeepAlive = std::thread(&HM_LGW::listenKeepAlive, this);

    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);
		_lastTimePacket = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAlive1 = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAliveResponse1 = _lastKeepAlive1;

        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		GD::out.printWarning("Warning: Connection to HM-LGW closed. Trying to reconnect...");
        		reconnect();
        		continue;
        	}
        	std::vector<uint8_t> data;
			try
			{
				do
				{
					if(BaseLib::HelperFunctions::getTimeSeconds() - _lastTimePacket > 1800) sendTimePacket();
					else sendKeepAlivePacket1();
					receivedBytes = _socket->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							GD::out.printError("Could not read from HM-LGW: Too much data.");
							break;
						}
					}
				} while(receivedBytes == bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty()) //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
				{
					continue;
				}
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				GD::out.printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				GD::out.printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		GD::out.printDebug("Debug: Packet received from HM-LGW on port " + _settings->port + ". Raw data:");
        		GD::out.printBinary(data);
        	}

        	processData(data);

        	_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::listenKeepAlive()
{
	try
    {
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);
		_lastKeepAlive2 = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAliveResponse2 = _lastKeepAlive2;

        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		continue;
        	}
        	std::vector<uint8_t> data;
			try
			{
				do
				{
					receivedBytes = _socketKeepAlive->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							GD::out.printError("Could not read from HM-LGW: Too much data.");
							break;
						}
					}
				} while(receivedBytes == bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty())
				{
					if(_socketKeepAlive->connected()) sendKeepAlivePacket2();
					continue;
				}
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				GD::out.printWarning("Warning: " + ex.what());
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				GD::out.printError("Error: " + ex.what());
				continue;
			}
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		GD::out.printDebug("Debug: Packet received from HM-LGW on port " + _settings->portKeepAlive + ". Raw data:");
        		GD::out.printBinary(data);
        	}

        	processDataKeepAlive(data);
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool HM_LGW::aesKeyExchange(std::vector<uint8_t>& data)
{
	try
	{
		std::string hex(&data.at(0), &data.at(0) + data.size());
		int32_t startPos = hex.find('\n');
		if(startPos == std::string::npos)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		startPos += 5;
		int32_t length = hex.find('\n', startPos);
		if(length == std::string::npos)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		length = length - startPos - 1;
		if(length <= 30)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		if(data.at(startPos - 4) == 'V' && data.at(startPos - 1) == ',')
		{
			uint8_t packetIndex = (_bl->hf.getNumber(data.at(startPos - 3)) << 4) + _bl->hf.getNumber(data.at(startPos - 2));
			packetIndex++;
			if(length != 32)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-LGW. Received IV has wrong size.");
				return false;
			}
			_remoteIV.clear();
			std::string ivHex(&data.at(startPos), &data.at(startPos) + length);
			_remoteIV = _bl->hf.getUBinary(ivHex);
			if(_remoteIV.size() != 16)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-LGW. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5)
			{
				GD::out.printDebug("HM-LGW IV is: ");
				GD::out.printBinary(_remoteIV);
			}

			if(EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_remoteIV.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return false;
			}

			std::vector<char> response = { 'V', _bl->hf.getHexChar(packetIndex >> 4), _bl->hf.getHexChar(packetIndex & 0xF), ',' };
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
				response.push_back(_bl->hf.getHexChar(nibble));
			}
			response.push_back(0x0D);
			response.push_back(0x0A);

			if(_bl->debugLevel >= 5)
			{
				GD::out.printDebug("Homegear IV is: ");
				GD::out.printBinary(_myIV);
			}

			if(EVP_DecryptInit_ex(_ctxDecrypt, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_myIV.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return false;
			}

			send(response, true);
			_aesExchangeComplete = true;
			return true;
		}
		else if(_remoteIV.empty())
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: Error communicating with HM-LGW. No IV was send from HM-LGW.");
			return false;
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

bool HM_LGW::aesKeyExchangeKeepAlive(std::vector<uint8_t>& data)
{
	try
	{
		std::string hex(&data.at(0), &data.at(0) + data.size());
		int32_t startPos = hex.find('\n');
		if(startPos == std::string::npos)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		startPos += 5;
		int32_t length = hex.find('\n', startPos);
		if(length == std::string::npos)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		length = length - startPos - 1;
		if(length <= 30)
		{
			GD::out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		if(data.at(startPos - 4) == 'V' && data.at(startPos - 1) == ',')
		{
			_packetIndexKeepAlive = (_bl->hf.getNumber(data.at(startPos - 3)) << 4) + _bl->hf.getNumber(data.at(startPos - 2));
			_packetIndexKeepAlive++;
			if(length != 32)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-LGW. Received IV has wrong size.");
				return false;
			}
			_remoteIVKeepAlive.clear();
			std::string ivHex(&data.at(startPos), &data.at(startPos) + length);
			_remoteIVKeepAlive = _bl->hf.getUBinary(ivHex);
			if(_remoteIVKeepAlive.size() != 16)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-LGW. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5)
			{
				GD::out.printDebug("HM-LGW IV for keep alive packets is: ");
				GD::out.printBinary(_remoteIVKeepAlive);
			}

			if(EVP_EncryptInit_ex(_ctxEncryptKeepAlive, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_remoteIVKeepAlive.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return false;
			}

			std::vector<char> response = { 'V', _bl->hf.getHexChar(_packetIndexKeepAlive >> 4), _bl->hf.getHexChar(_packetIndexKeepAlive & 0xF), ',' };
			std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<int32_t> distribution(0, 15);
			_myIVKeepAlive.clear();
			for(int32_t i = 0; i < 32; i++)
			{
				int32_t nibble = distribution(generator);
				if((i % 2) == 0)
				{
					_myIVKeepAlive.push_back(nibble << 4);
				}
				else
				{
					_myIVKeepAlive.at(i / 2) |= nibble;
				}
				response.push_back(_bl->hf.getHexChar(nibble));
			}
			response.push_back(0x0D);
			response.push_back(0x0A);

			if(_bl->debugLevel >= 5)
			{
				GD::out.printDebug("Homegear IV for keep alive packets is: ");
				GD::out.printBinary(_myIVKeepAlive);
			}

			if(EVP_DecryptInit_ex(_ctxDecryptKeepAlive, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_myIVKeepAlive.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return false;
			}

			sendKeepAlive(response, true);
			_aesExchangeKeepAliveComplete = true;
			return true;
		}
		else if(_remoteIVKeepAlive.empty())
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: Error communicating with HM-LGW. No IV was send from HM-LGW.");
			return false;
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void HM_LGW::buildPacket(std::vector<char>& packet, const std::vector<char>& payload)
{
	try
	{
		std::vector<char> unescapedPacket;
		unescapedPacket.push_back(0xFD);
		int32_t size = payload.size() + 1; //Payload size plus message counter size - control byte
		unescapedPacket.push_back(size >> 8);
		unescapedPacket.push_back(size & 0xFF);
		unescapedPacket.push_back(payload.at(0));
		unescapedPacket.push_back(_packetIndex);
		unescapedPacket.insert(unescapedPacket.end(), payload.begin() + 1, payload.end());
		uint16_t crc = _crc.calculate(unescapedPacket);
		unescapedPacket.push_back(crc >> 8);
		unescapedPacket.push_back(crc & 0xFF);
		escapePacket(unescapedPacket, packet);
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::escapePacket(const std::vector<char>& unescapedPacket, std::vector<char>& escapedPacket)
{
	try
	{
		escapedPacket.clear();
		if(unescapedPacket.empty()) return;
		escapedPacket.push_back(unescapedPacket[0]);
		for(uint32_t i = 1; i < unescapedPacket.size(); i++)
		{
			if(unescapedPacket[i] == (char)0xFC || unescapedPacket[i] == (char)0xFD)
			{
				escapedPacket.push_back(0xFC);
				escapedPacket.push_back(unescapedPacket[i] & (char)0x7F);
			}
			else escapedPacket.push_back(unescapedPacket[i]);
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		std::string packets;
		if(!_aesExchangeComplete)
		{
			aesKeyExchange(data);
			return;
		}
		std::vector<uint8_t> decryptedData = decrypt(data);
		if(decryptedData.size() < 8) //8 is minimum size fd
		{
			GD::out.printWarning("Warning: Too small packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(decryptedData));
			return;
		}
		if(!_initCommandQueue.empty() && _packetIndex == 0 && decryptedData.at(0) == 'S')
		{
			processInit(decryptedData);
			return;
		}

		std::vector<uint8_t> packet;
		bool escapeByte = false;
		for(std::vector<uint8_t>::iterator i = decryptedData.begin(); i != decryptedData.end(); ++i)
		{
			if(!packet.empty() && *i == 0xfd)
			{
				GD::out.printDebug(std::string("Debug: Packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet)));
				uint16_t crc = _crc.calculate(packet, true);
				if(packet.at(packet.size() - 2) != (crc >> 8) || packet.at(packet.size() - 1) != (crc & 0xFF))
				{
					GD::out.printError("Error: CRC failed on packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet));
					_stopped = true;
					return;
				}
				else
				{
					if(_initCommandQueue.empty()) parsePacket(packet); else processInit(packet);
				}
				packet.clear();
				escapeByte = false;
			}
			if(*i == 0xfc)
			{
				escapeByte = true;
				continue;
			}
			if(escapeByte)
			{
				packet.push_back(*i | 0x80);
				escapeByte = false;
			}
			else packet.push_back(*i);
		}
		GD::out.printDebug(std::string("Debug: Packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet)));
		uint16_t crc = _crc.calculate(packet, true);
		if(packet.at(packet.size() - 2) != (crc >> 8) || packet.at(packet.size() - 1) != (crc & 0xFF))
		{
			GD::out.printError("Error: CRC failed on packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet));
			_stopped = true;
			return;
		}
		else
		{
			if(_initCommandQueue.empty()) parsePacket(packet); else processInit(packet);
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::processDataKeepAlive(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		std::string packets;
		if(!_aesExchangeKeepAliveComplete)
		{
			aesKeyExchangeKeepAlive(data);
			return;
		}
		std::vector<uint8_t> decryptedData = decryptKeepAlive(data);
		if(decryptedData.empty()) return;
		packets.insert(packets.end(), decryptedData.begin(), decryptedData.end());

		std::istringstream stringStream(packets);
		std::string packet;
		while(std::getline(stringStream, packet))
		{
			if(_initCompleteKeepAlive) parsePacketKeepAlive(packet); else processInitKeepAlive(packet);
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::processInit(std::vector<uint8_t>& packet)
{
	try
	{
		if(_initCommandQueue.empty()) return;
		if(_packetIndex == 0) //Expecting "BidCoS-over-LAN" packet
		{
			std::string packetString(packet.begin(), packet.end());
			std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packetString, ',');
			if(parts.size() != 2 || parts.at(0).size() != 3 || parts.at(0).at(0) != 'S' || parts.at(1).size() < 15 || parts.at(1).compare(0, 15, "BidCoS-over-LAN") != 0)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: First packet from HM-LGW does not start with \"S\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
				return;
			}
			uint8_t packetIndex = (_bl->hf.getNumber(parts.at(0).at(1)) << 4) + _bl->hf.getNumber(parts.at(0).at(2));
			std::vector<char> response = { '>', _bl->hf.getHexChar(packetIndex >> 4), _bl->hf.getHexChar(packetIndex & 0xF), ',', '0', '0', '0', '0', '\r', '\n' };
			send(response, false);
			response.clear();
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
			return;
		}
		if(_packetIndex == 1 && packet.at(5) == 0)
		{
			if(packet.at(3) != 0 || packet.at(4) != 0)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 2 && packet.at(5) == 0)
		{
			if(packet.at(3) != 0 || packet.at(4) != 0)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 3 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 4 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 5 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 6 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 7 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 8 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			const auto timePoint = std::chrono::system_clock::now();
			time_t t = std::chrono::system_clock::to_time_t(timePoint);
			tm* localTime = std::localtime(&t);
			uint32_t time = (uint32_t)t;
			std::vector<char> payload{ 0, 0xE };
			payload.push_back(time >> 24);
			payload.push_back((time >> 16) & 0xFF);
			payload.push_back((time >> 8) & 0xFF);
			payload.push_back(time & 0xFF);
			payload.push_back(localTime->tm_gmtoff / 1800);
			std::vector<char> packet;
			buildPacket(packet, payload);
			_packetIndex++;
			send(packet, false);
			_lastTimePacket = BaseLib::HelperFunctions::getTimeSeconds();
		}
		else if(_packetIndex == 9 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> response;
			buildPacket(response, _initCommandQueue.front());
			_packetIndex++;
			send(response, false);
		}
		else if(_packetIndex == 10 && packet.at(5) == 4)
		{
			if(packet.at(3) != 0 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> payload = _initCommandQueue.front();
			if(_settings->rfKey.empty())
			{
				payload.push_back(0);
			}
			else
			{
				std::vector<char> rfKey = _bl->hf.getBinary(_settings->rfKey);
				payload.insert(payload.end(), rfKey.begin(), rfKey.end());
				payload.push_back(_settings->currentRFKeyIndex);
			}
			std::vector<char> packet;
			buildPacket(packet, payload);
			_packetIndex++;
			send(packet, false);
		}
		else if(_packetIndex == 11 && packet.at(5) == 4)
		{
			if(packet.at(3) != 1 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			std::vector<char> payload = _initCommandQueue.front();
			if(payload.at(1) == 0xF)
			{
				std::vector<char> rfKey = _bl->hf.getBinary(_settings->oldRFKey);
				payload.insert(payload.end(), rfKey.begin(), rfKey.end());
				payload.push_back(_settings->currentRFKeyIndex - 1);
			}
			else
			{
				payload.push_back(_myAddress >> 16);
				payload.push_back((_myAddress >> 8) & 0xFF);
				payload.push_back(_myAddress & 0xFF);
			}
			std::vector<char> packet;
			buildPacket(packet, payload);
			_packetIndex++;
			send(packet, false);
		}
		else if(_packetIndex == 12 && packet.at(5) == 4)
		{
			if(packet.at(3) != 1 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			if(_initCommandQueue.empty())
			{
				GD::out.printInfo("Info: Init queue completed. Sending peers...");
				sendPeers();
			}
			else
			{
				std::vector<char> payload = _initCommandQueue.front();
				payload.push_back(_myAddress >> 16);
				payload.push_back((_myAddress >> 8) & 0xFF);
				payload.push_back(_myAddress & 0xFF);
				std::vector<char> packet;
				buildPacket(packet, payload);
				_packetIndex++;
				send(packet, false);
			}
		}
		else if(_packetIndex == 13 && packet.at(5) == 4)
		{
			if(packet.at(3) != 1 || packet.at(4) != _packetIndex - 1)
			{
				GD::out.printWarning("Warning: Packet from HM-LGW has wrong index.");
			}
			_initCommandQueue.pop_front();
			if(!_initCommandQueue.empty())
			{
				GD::out.printWarning("Warning: Init command queue of HM-LGW is not empty.");
				_initCommandQueue.clear();
			}
			GD::out.printInfo("Info: Init queue completed. Sending peers...");
			sendPeers();
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::processInitKeepAlive(std::string& packet)
{
	try
	{
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packet, ',');
		if(parts.size() != 2 || parts.at(0).size() != 3 || parts.at(0).at(0) != 'S' || parts.at(1).size() < 6 || parts.at(1).compare(0, 6, "SysCom") != 0)
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: First packet from HM-LGW does not start with \"S\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
			return;
		}

		std::vector<char> response = { '>', _bl->hf.getHexChar(_packetIndexKeepAlive >> 4), _bl->hf.getHexChar(_packetIndexKeepAlive & 0xF), ',', '0', '0', '0', '0', '\r', '\n' };
		sendKeepAlive(response, false);
		response = std::vector<char>{ 'L', '0', '0', ',', '0', '2', ',', '0', '0', 'F', 'F', ',', '0', '0', '\r', '\n'};
		sendKeepAlive(response, false);
		_lastKeepAlive2 = BaseLib::HelperFunctions::getTimeSeconds() - 20;
		_lastKeepAliveResponse2 = _lastKeepAlive2;
		_packetIndexKeepAlive = 0;
		_initCompleteKeepAlive = true;
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::parsePacket(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.empty()) return;
		if(packet.at(5) == 4)
		{
			if(packet.at(3) == 0 && packet.size() == 10 && packet.at(6) == 2 && packet.at(7) == 0)
			{
				if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Keep alive response received from HM-LGW on port " + _settings->port + ".");
				_lastKeepAliveResponse1 = BaseLib::HelperFunctions::getTimeSeconds();
			}
		}
		else if(packet.at(5) == 5)
		{
			if(packet.at(3) == 1 && packet.size() >= 20)
			{
				std::vector<uint8_t> binaryPacket({(uint8_t)(packet.size() - 11)});
				binaryPacket.insert(binaryPacket.end(), packet.begin() + 9, packet.end() - 2);
				binaryPacket.push_back(packet.at(8));
				std::shared_ptr<BidCoSPacket> bidCoSPacket(new BidCoSPacket(binaryPacket, true, BaseLib::HelperFunctions::getTime()));
				_lastPacketReceived = BaseLib::HelperFunctions::getTime();
				raisePacketReceived(bidCoSPacket);
			}
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LGW::parsePacketKeepAlive(std::string& packet)
{
	try
	{
		if(packet.empty()) return;
		if(_bl->debugLevel >= 5) GD::out.printDebug(std::string("Debug: Packet received from HM-LGW on port " + _settings->portKeepAlive + ": " + packet));
		if(packet.at(0) == '>'  && (packet.at(1) == 'K' || packet.at(1) == 'L') && packet.size() == 5)
		{
			if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Keep alive response received from HM-LGW on port " + _settings->portKeepAlive + ".");
			std::string index = packet.substr(2, 2);
			if(_bl->hf.getNumber(index, true) == _packetIndexKeepAlive)
			{
				_lastKeepAliveResponse2 = BaseLib::HelperFunctions::getTimeSeconds();
				_packetIndexKeepAlive++;
			}
			if(packet.at(1) == 'L') sendKeepAlivePacket2();
		}
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
