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
#include "../GD.h"

namespace BidCoS
{

HM_CFG_LAN::HM_CFG_LAN(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IBidCoSInterface(settings)
{
	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(!settings)
	{
		GD::out.printCritical("Critical: Error initializing HM-CFG-LAN. Settings pointer is empty.");
		return;
	}
	if(!settings->lanKey.empty())
	{
		_useAES = true;
		GD::out.printInfo("Info: Enabling AES encryption for communication with HM-CFG-LAN.");
	}
	else
	{
		_useAES = false;
		GD::out.printInfo("Info: Disabling AES encryption for communication with HM-CFG-LAN.");
	}

	if(settings->rfKey.empty())
	{
		GD::out.printError("Error: No RF AES key specified in physicalinterfaces.conf on your HM-CFG-LAN for communication with your BidCoS devices.");
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

	if(_oldRFKey.empty() && settings->currentRFKeyIndex > 1)
	{
		GD::out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is larger than \"1\" but \"OldRFKey\" is not specified. Please set your old RF key or set key index to \"1\".");
	}

	if(settings->currentRFKeyIndex > 253)
	{
		GD::out.printError("Error: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is greater than \"253\". That is not allowed.");
		settings->currentRFKeyIndex = 253;
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

std::string HM_CFG_LAN::getPeerInfoPacket(PeerInfo& peerInfo)
{
	try
	{
		std::string packetHex = std::string("+") + BaseLib::HelperFunctions::getHexString(peerInfo.address, 6) + ",";
		if(peerInfo.aesEnabled)
		{
			packetHex += peerInfo.wakeUp ? "03," : "01,";
			packetHex += BaseLib::HelperFunctions::getHexString(peerInfo.keyIndex, 2) + ",";
			packetHex += BaseLib::HelperFunctions::getHexString(peerInfo.getAESChannelMap()) + ",";
		}
		else
		{
			packetHex += peerInfo.wakeUp ? "02," : "00,";
			packetHex += "00,";
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

void HM_CFG_LAN::addPeer(PeerInfo peerInfo)
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

void HM_CFG_LAN::addPeers(std::vector<PeerInfo>& peerInfos)
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

void HM_CFG_LAN::sendPeers()
{
	try
	{
		_peersMutex.lock();
		for(std::map<int32_t, PeerInfo>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			send(getPeerInfoPacket(i->second));
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

void HM_CFG_LAN::removePeer(int32_t address)
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

void HM_CFG_LAN::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
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
			GD::out.printDebug("Debug: HM-CFG-LAN: Ignoring ACK packet.", 6);
			_lastPacketSent = BaseLib::HelperFunctions::getTime();
			return;
		}
		if((bidCoSPacket->controlByte() & 0x01) && packet->senderAddress() == _myAddress && (bidCoSPacket->payload()->empty() || (bidCoSPacket->payload()->size() == 1 && bidCoSPacket->payload()->at(0) == 0)))
		{
			GD::out.printDebug("Debug: HM-CFG-LAN: Ignoring wake up packet.", 6);
			_lastPacketSent = BaseLib::HelperFunctions::getTime();
			return;
		}

		if(!_initComplete)
		{
			GD::out.printWarning(std::string("Warning: !!!Not!!! sending packet, because init sequence is not complete: ") + bidCoSPacket->hexString());
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

void HM_CFG_LAN::send(std::string hexString, bool raw)
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

void HM_CFG_LAN::send(std::vector<char>& data, bool raw)
{
    try
    {
    	if(data.size() < 3) return; //Otherwise error in printInfo
    	std::vector<char> encryptedData;
    	if(_useAES && !raw) encryptedData = encrypt(data);
    	_sendMutex.lock();
    	if(!_socket->connected() || _stopped)
    	{
    		GD::out.printWarning(std::string("Warning: !!!Not!!! sending") + ((_useAES && !raw) ? " (encrypted)" : "") + ": " + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
    		_sendMutex.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5)
        {
            GD::out.printInfo(std::string("Debug: Sending") + ((_useAES && !raw) ? " (encrypted)" : "") + ": " + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
        }
    	(_useAES && !raw) ? _socket->proofwrite(encryptedData) : _socket->proofwrite(data);
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

void HM_CFG_LAN::startListening()
{
	try
	{
		stopListening();
		if(_rfKey.empty())
		{
			GD::out.printError("Error: Cannot start listening , because rfKey is not specified for HM-CFG-LAN.");
			return;
		}
		if(_useAES) openSSLInit();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->verifyCertificate));
		GD::out.printDebug("Connecting to HM-CFG-LAN with Hostname " + _settings->host + " on port " + _settings->port + "...");
		//_socket->open();
		//GD::out.printInfo("Connected to HM-CFG-LAN device with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&HM_CFG_LAN::listen, this);
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

void HM_CFG_LAN::reconnect()
{
	try
	{
		_socket->close();
		if(_useAES) openSSLInit();
		createInitCommandQueue();
		GD::out.printDebug("Connecting to HM-CFG-LAN device with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		GD::out.printInfo("Connected to HM-CFG-LAN device with Hostname " + _settings->host + " on port " + _settings->port + ".");
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
		_socket->close();
		if(_useAES) openSSLCleanup();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
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

void HM_CFG_LAN::createInitCommandQueue()
{
	try
	{
		_initComplete = false;
		_initCommandQueue.clear();

		int32_t i = 0;
		while(!GD::family->getCentral() && i < 30)
		{
			GD::out.printDebug("Debug: HM-CFG-LAN: Waiting for central to load.");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			i++;
		}
		if(!GD::family->getCentral())
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: Could not get central address for HM-CFG-LAN. Stopping listening.");
			return;
		}

		std::vector<char> packet = {'A'};
		_myAddress = GD::family->getCentral()->physicalAddress();
		std::string hexString = BaseLib::HelperFunctions::getHexString(_myAddress, 6) + "\r\n";
		packet.insert(packet.end(), hexString.begin(), hexString.end());
		_initCommandQueue.push_back(packet);

		packet.clear();
		hexString = "C\r\n";
		if(!_rfKey.empty())
		{
			hexString += "Y01," + BaseLib::HelperFunctions::getHexString(_settings->currentRFKeyIndex, 2) + "," + BaseLib::HelperFunctions::getHexString(_rfKey) + "\r\n";
		}
		else hexString += "Y01,00,\r\n";
		packet.insert(packet.end(), hexString.begin(), hexString.end());
		_initCommandQueue.push_back(packet);

		packet.clear();
		if(!_oldRFKey.empty())
		{
			hexString = "Y02," + BaseLib::HelperFunctions::getHexString(_settings->currentRFKeyIndex - 1, 2) + "," + BaseLib::HelperFunctions::getHexString(_oldRFKey) + "\r\n";
		}
		else hexString = "Y02,00,\r\n";
		packet.insert(packet.end(), hexString.begin(), hexString.end());
		_initCommandQueue.push_back(packet);

		packet.clear();
		hexString = "Y03,00,\r\n";
		packet.insert(packet.end(), hexString.begin(), hexString.end());
		_initCommandQueue.push_back(packet);

		packet.clear();
		const auto timePoint = std::chrono::system_clock::now();
		time_t t = std::chrono::system_clock::to_time_t(timePoint);
		tm* localTime = std::localtime(&t);
		uint32_t time = (uint32_t)(t - 946684800) + 1; //I add one second for processing time
		hexString = "T" + BaseLib::HelperFunctions::getHexString(time, 8) + ',' + BaseLib::HelperFunctions::getHexString(localTime->tm_gmtoff / 1800, 2) + ",00,00000000\r\n";
		packet.insert(packet.end(), hexString.begin(), hexString.end());
		_initCommandQueue.push_back(packet);
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

void HM_CFG_LAN::openSSLPrintError()
{
	uint32_t errorCode = ERR_get_error();
	std::vector<char> buffer(256); //At least 120 bytes
	ERR_error_string(errorCode, &buffer.at(0));
	GD::out.printError("Error: " + std::string(&buffer.at(0)));
}

bool HM_CFG_LAN::openSSLInit()
{
	openSSLCleanup();

	if(_settings->lanKey.size() != 32)
	{
		GD::out.printError("Error: The AES key specified in physicalinterfaces.conf for communication with your HM-CFG-LAN has the wrong size.");
		return false;
		//_key.resize(16);
		//MD5((uint8_t*)_settings->lanKey.c_str(), _settings->lanKey.size(), &_key.at(0));
	}
	else
	{
		_key = _bl->hf.getUBinary(_settings->lanKey);
		if(_key.size() != 16)
		{
			GD::out.printError("Error: The AES key specified in physicalinterfaces.conf for communication with your HM-CFG-LAN is not a valid hexadecimal string.");
			return false;
		}
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
	_aesExchangeComplete = false;
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
	_myIV.clear();
	_remoteIV.clear();
	_aesExchangeComplete = false;
}

std::vector<char> HM_CFG_LAN::encrypt(std::vector<char>& data)
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

std::vector<uint8_t> HM_CFG_LAN::decrypt(std::vector<uint8_t>& data)
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

void HM_CFG_LAN::sendKeepAlive()
{
	try
    {
		if(!_initComplete) return;
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastKeepAlive >= 10)
		{
			if(_lastKeepAliveResponse < _lastKeepAlive)
			{
				_lastKeepAliveResponse = _lastKeepAlive;
				_stopped = true;
				return;
			}

			_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
			send(_keepAlivePacket, false);
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

void HM_CFG_LAN::sendTimePacket()
{
	try
    {
		const auto timePoint = std::chrono::system_clock::now();
		time_t t = std::chrono::system_clock::to_time_t(timePoint);
		tm* localTime = std::localtime(&t);
		uint32_t time = (uint32_t)(t - 946684800);
		std::string hexString = "T" + BaseLib::HelperFunctions::getHexString(time, 8) + ',' + BaseLib::HelperFunctions::getHexString(localTime->tm_gmtoff / 1800, 2) + ",00,00000000\r\n";
		send(hexString, false);
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

void HM_CFG_LAN::listen()
{
    try
    {
    	createInitCommandQueue(); //Called here in a seperate thread so that startListening is not blocking Homegear

    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);
		_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAliveResponse = _lastKeepAlive;

        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		GD::out.printWarning("Warning: Connection to HM-CFG-LAN closed. Trying to reconnect...");
        		reconnect();
        		continue;
        	}
        	std::vector<uint8_t> data;
			try
			{
				do
				{
					receivedBytes = _socket->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							GD::out.printError("Could not read from HM-CFG-LAN: Too much data.");
							break;
						}
					}
				} while(receivedBytes == bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty()) //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
				{
					if(_socket->connected())
					{
						if(BaseLib::HelperFunctions::getTimeSeconds() - _lastTimePacket > 1800) sendTimePacket();
						sendKeepAlive();
					}
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
        		GD::out.printDebug("Debug: Packet received from HM-CFG-LAN. Raw data:");
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

bool HM_CFG_LAN::aesKeyExchange(std::vector<uint8_t>& data)
{
	try
	{
		if(data.at(0) == 'V')
		{
			if(!_useAES)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-CFG-LAN. Device requires AES, but no AES key was specified in physicalinterfaces.conf.");
				return false;
			}
			if(data.size() != 35)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-CFG-LAN. Received IV has wrong size.");
				return false;
			}
			_remoteIV.clear();
			std::string ivHex(&data.at(1), &data.at(1) + (data.size() - 3));
			_remoteIV = _bl->hf.getUBinary(ivHex);
			if(_remoteIV.size() != 16)
			{
				_stopCallbackThread = true;
				GD::out.printError("Error: Error communicating with HM-CFG-LAN. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5)
			{
				GD::out.printDebug("HM-CFG-LAN IV is: ");
				GD::out.printBinary(_remoteIV);
			}

			if(EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cfb128(), NULL, &_key.at(0), &_remoteIV.at(0)) != 1)
			{
				_stopCallbackThread = true;
				openSSLPrintError();
				return false;
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
			GD::out.printError("Error: Error communicating with HM-CFG-LAN. AES is enabled but no IV was send from HM-CFG-LAN.");
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

void HM_CFG_LAN::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		std::string packets;
		if(_useAES)
		{
			if(!_aesExchangeComplete)
			{
				aesKeyExchange(data);
				return;
			}
			std::vector<uint8_t> decryptedData = decrypt(data);
			if(decryptedData.empty()) return;
			packets.insert(packets.end(), decryptedData.begin(), decryptedData.end());
		}
		else packets.insert(packets.end(), data.begin(), data.end());

		std::istringstream stringStream(packets);
		std::string packet;
		while(std::getline(stringStream, packet))
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

void HM_CFG_LAN::processInit(std::string& packet)
{
	if(_initCommandQueue.empty() || packet.length() < 10) return;
	if(_initCommandQueue.front().at(0) == 'A') //No init packet has been sent yet
	{
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packet, ',');
		if(parts.size() != 7 || parts.at(0) != "HHM-LAN-IF")
		{
			_stopCallbackThread = true;
			GD::out.printError("Error: First packet from HM-CFG-LAN does not start with \"HHM-LAN-IF\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
			return;
		}
		_startUpTime = BaseLib::HelperFunctions::getTime() - (int64_t)BaseLib::HelperFunctions::getNumber(parts.at(5), true);
		send(_initCommandQueue.front(), false);
		_initCommandQueue.pop_front();
		send(_initCommandQueue.front(), false);
	}
	else if((_initCommandQueue.front().at(0) == 'C' || _initCommandQueue.front().at(0) == 'Y') && packet.at(0) == 'I')
	{
		_initCommandQueue.pop_front();
		send(_initCommandQueue.front(), false);
		if(_initCommandQueue.front().at(0) == 'T')
		{
			_initCommandQueue.pop_front();
			sendPeers();
		}
	}
}

void HM_CFG_LAN::parsePacket(std::string& packet)
{
	try
	{
		if(packet.empty()) return;
		if(_bl->debugLevel >= 5) GD::out.printDebug(std::string("Debug: Packet received from HM-CFG-LAN") + (_useAES ? + " (encrypted)" : "") + ": " + packet);
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packet, ',');
		if(packet.at(0) == 'H' && parts.size() == 7)
		{
			/*
			Index	Meaning
			0		"HM-LAN-IF"
			1		Firmware version
			2		Serial number
			3		Default address?
			4		Address
			5		Time since boot in milliseconds
			6		Number of registered peers
			*/

			_lastKeepAliveResponse = BaseLib::HelperFunctions::getTimeSeconds();
			_startUpTime = BaseLib::HelperFunctions::getTime() - (int64_t)BaseLib::HelperFunctions::getNumber(parts.at(5), true);
		}
		else if(packet.at(0) == 'E' || packet.at(0) == 'R')
		{
			if(parts.size() != 6)
			{
				GD::out.printWarning("Warning: Invalid packet received from HM-CFG-LAN: " + packet);
				return;
			}
			/*
			Index	Meaning
			0		Sender address
			1		Control and status byte
			2		Time received
			3		AES key index?
			4		BidCoS packet
			*/

			int32_t tempNumber = BaseLib::HelperFunctions::getNumber(parts.at(1), true);

			/*
			00: Not set
			01: Packet received, wait for AES handshake
			02: High load
			04: Overload
			*/
			uint8_t statusByte = tempNumber >> 8;
			if(statusByte & 4) GD::out.printError("Error: HM-CFG-LAN reached 1% rule.");
			else if(statusByte & 2) GD::out.printWarning("Warning: HM-CFG-LAN nearly reached 1% rule.");

			/*
			00: Not set
			01: ACK or ACK was sent in response to this packet
			02: Message without BIDI bit was sent
			08: No response after three tries
			21: ?
			2*: ?
			30: AES handshake not successful
			4*: AES handshake successful
			50: AES handshake not successful
			8*: ?
			*/
			uint8_t controlByte = tempNumber & 0xFF;

			if(parts.at(5).size() > 18) //18 is minimal packet length
        	{
				std::vector<uint8_t> binaryPacket({ (uint8_t)(parts.at(5).size() / 2) });
				_bl->hf.getUBinary(parts.at(5), parts.at(5).size() - 1, binaryPacket);
				int32_t rssi = BaseLib::HelperFunctions::getNumber(parts.at(4), true);
				//Convert to TI CC1101 format
				if(rssi <= -75) rssi = ((rssi + 74) * 2) + 256;
				else rssi = (rssi + 74) * 2;
				binaryPacket.push_back(rssi);

				std::shared_ptr<BidCoSPacket> bidCoSPacket(new BidCoSPacket(binaryPacket, true, BaseLib::HelperFunctions::getTime()));
				if(packet.at(0) == 'E' && (statusByte & 1))
				{
					GD::out.printDebug("Debug: Waiting for AES handshake.");
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
					_lastPacketSent = BaseLib::HelperFunctions::getTime();
					return;
				}
				if((controlByte & 0x30) == 0x30 || (controlByte & 0x50) == 0x50)
				{
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
					GD::out.printWarning("Warning: AES handshake was not successful: " + bidCoSPacket->hexString());
					return;
				}
				if(packet.at(0) == 'R')
				{
					if(controlByte & 8)
					{
						GD::out.printWarning("Info: No response to packet after 3 tries: " + bidCoSPacket->hexString());
						return;
					}
					else if(controlByte & 2)
					{
						return;
					}
					else if(!(controlByte & 0x40) && !(controlByte & 0x20) && (controlByte & 1) && (bidCoSPacket->controlByte() & 0x20))
					{
						_lastPacketSent = BaseLib::HelperFunctions::getTime();
						return;
					}
				}
				raisePacketReceived(bidCoSPacket);
        	}
        	else if(!parts.at(5).empty()) GD::out.printWarning("Warning: Too short packet received: " + parts.at(5));
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
