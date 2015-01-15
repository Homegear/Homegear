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

#include "HMW-LGW.h"
#include "../GD.h"

namespace HMWired
{
HMW_LGW::HMW_LGW(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IHMWiredInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "HMW-LGW \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(!settings)
	{
		_out.printCritical("Critical: Error initializing HMW-LGW. Settings pointer is empty.");
		return;
	}
	if(settings->lanKey.empty())
	{
		_out.printError("Error: No security key specified in physicalinterfaces.conf.");
		return;
	}
}

HMW_LGW::~HMW_LGW()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		aesCleanup();
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::search(std::vector<int32_t>& foundDevices)
{
	try
	{
		int32_t startTime = BaseLib::HelperFunctions::getTimeSeconds();
		foundDevices.clear();
		_searchResult.clear();
		_searchFinished = false;

		std::vector<char> startPacket;
		std::vector<char> payload{ 0x44, 0x00, (char)0xFF };

		buildPacket(startPacket, payload);
		_packetIndex++;
		send(startPacket, false);

		while(!_searchFinished && BaseLib::HelperFunctions::getTimeSeconds() - startTime < 180)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		if(BaseLib::HelperFunctions::getTimeSeconds() - startTime >= 180) _out.printError("Error: Device search timed out.");

		foundDevices.insert(foundDevices.begin(), _searchResult.begin(), _searchResult.end());
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();

		if(!_initComplete)
    	{
    		_out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->port + "), because the init sequence is not completed: " + packet->hexString());
    		return;
    	}

		std::shared_ptr<HMWiredPacket> hmwiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmwiredPacket) return;
		if(hmwiredPacket->type() == HMWiredPacketType::ackMessage) return; //Ignore ACK packets as they're sent automatically

		std::vector<char> packetBytes = hmwiredPacket->byteArrayLgw();
		if(_bl->debugLevel >= 4) _out.printInfo("Info: Sending (" + _settings->id + "): " + _bl->hf.getHexString(packetBytes));

		if(hmwiredPacket->destinationAddress() == -1) //Broadcast packet
		{
			std::vector<char> requestPacket;
			std::vector<char> payload{ 0x53, 0 };
			payload.insert(payload.end(), packetBytes.begin(), packetBytes.end());
			buildPacket(requestPacket, payload);
			_packetIndex++;
			send(requestPacket, false);
		}
		else if(hmwiredPacket->type() == HMWiredPacketType::ackMessage)
		{
			for(int32_t j = 0; j < 3; j++)
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload { 0x53, (char)0xC8 };
				payload.insert(payload.end(), packetBytes.begin(), packetBytes.end());
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 0x61);
				if(!responsePacket.empty()) break;
				if(j == 2)
				{
					_out.printInfo("Info: No response from HMW-LGW to packet " + _bl->hf.getHexString(packetBytes));
					return;
				}
			}
		}
		else
		{
			for(int32_t j = 0; j < 3; j++) //This causes the packet to be sent 9 times all together.
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload { 0x53, (char)0xC8 };
				payload.insert(payload.end(), packetBytes.begin(), packetBytes.end());
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 0x72);
				if(!responsePacket.empty())
				{
					std::shared_ptr<HMWiredPacket> responseHmwiredPacket(new HMWiredPacket(responsePacket, true, BaseLib::HelperFunctions::getTime(), hmwiredPacket->destinationAddress(), hmwiredPacket->senderAddress()));
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
					raisePacketReceived(responseHmwiredPacket);
					break;
				}
				if(j == 2)
				{
					_out.printInfo("Info: No response from HMW-LGW to packet " + _bl->hf.getHexString(packetBytes));
					return;
				}
			}
		}

		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t messageCounter, uint8_t responseType)
{
	try
    {
		if(packet.size() < 8 || _stopped) return;
		std::shared_ptr<Request> request(new Request(responseType));
		_requestsMutex.lock();
		_requests[messageCounter] = request;
		_requestsMutex.unlock();
		std::unique_lock<std::mutex> lock(request->mutex);
		send(packet, false);
		if(!request->conditionVariable.wait_for(lock, std::chrono::milliseconds(700), [&] { return request->mutexReady; }))
		{
			_out.printError("Error: No response received to packet: " + _bl->hf.getHexString(packet));
		}
		response = request->response;

		_requestsMutex.lock();
		_requests.erase(messageCounter);
		_requestsMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _requestsMutex.unlock();
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _requestsMutex.unlock();
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        _requestsMutex.unlock();
    }
}

void HMW_LGW::send(std::string hexString, bool raw)
{
	try
    {
		if(hexString.empty()) return;
		std::vector<char> data(&hexString.at(0), &hexString.at(0) + hexString.size());
		send(data, raw);
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::send(const std::vector<char>& data, bool raw)
{
    try
    {
    	if(data.size() < 3) return; //Otherwise error in printInfo
    	std::vector<char> encryptedData;
    	if(!raw) encryptedData = encrypt(data);
    	_sendMutex.lock();
    	if(!_socket->connected() || _stopped)
    	{
    		_out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
    		_sendMutex.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5)
        {
            _out.printDebug("Debug: Sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
        }
    	(!raw) ? _socket->proofwrite(encryptedData) : _socket->proofwrite(data);
    	 _sendMutex.unlock();
    	 return;
    }
    catch(const BaseLib::SocketOperationException& ex)
    {
    	_out.printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stopped = true;
    _sendMutex.unlock();
}

void HMW_LGW::startListening()
{
	try
	{
		stopListening();
		_searchFinished = true;
		aesInit();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_socket->setReadTimeout(1000000);
		_out.printDebug("Connecting to HMW-LGW with hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		_listenThread = std::thread(&HMW_LGW::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::reconnect()
{
	try
	{
		_socket->close();
		aesInit();
		_requestsMutex.lock();
		_requests.clear();
		_requestsMutex.unlock();
		_initComplete = false;
		_searchFinished = true;
		_out.printDebug("Connecting to HMW-LGW with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_out.printInfo("Connected to HMW-LGW with hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		_stopCallbackThread = false;
		_socket->close();
		aesCleanup();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
		_requestsMutex.lock();
		_requests.clear();
		_requestsMutex.unlock();
		_initComplete = false;
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool HMW_LGW::aesInit()
{
	aesCleanup();

	if(_settings->lanKey.empty())
	{
		_out.printError("Error: No AES key specified in physicalinterfaces.conf for communication with your HMW-LGW.");
		return false;
	}

	gcry_error_t result;
	gcry_md_hd_t md5Handle = nullptr;
	if((result = gcry_md_open(&md5Handle, GCRY_MD_MD5, 0)) != GPG_ERR_NO_ERROR)
	{
		_out.printError("Could not initialize MD5 handle: " + _bl->hf.getGCRYPTError(result));
		return false;
	}
	gcry_md_write(md5Handle, _settings->lanKey.c_str(), _settings->lanKey.size());
	gcry_md_final(md5Handle);
	uint8_t* digest = gcry_md_read(md5Handle, GCRY_MD_MD5);
	if(!digest)
	{
		_out.printError("Could not generate MD5 of lanKey: " + _bl->hf.getGCRYPTError(result));
		gcry_md_close(md5Handle);
		return false;
	}
	if(gcry_md_get_algo_dlen(GCRY_MD_MD5) != 16) _out.printError("Could not generate MD5 of lanKey: Wront digest size.");
	_key.clear();
	_key.insert(_key.begin(), digest, digest + 16);
	gcry_md_close(md5Handle);

	if((result = gcry_cipher_open(&_encryptHandle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE)) != GPG_ERR_NO_ERROR)
	{
		_encryptHandle = nullptr;
		_out.printError("Error initializing cypher handle for encryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}
	if(!_encryptHandle)
	{
		_out.printError("Error cypher handle for encryption is nullptr.");
		return false;
	}
	if((result = gcry_cipher_setkey(_encryptHandle, &_key.at(0), _key.size())) != GPG_ERR_NO_ERROR)
	{
		aesCleanup();
		_out.printError("Error: Could not set key for encryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}

	if((result = gcry_cipher_open(&_decryptHandle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE)) != GPG_ERR_NO_ERROR)
	{
		_decryptHandle = nullptr;
		_out.printError("Error initializing cypher handle for decryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}
	if(!_decryptHandle)
	{
		_out.printError("Error cypher handle for decryption is nullptr.");
		return false;
	}
	if((result = gcry_cipher_setkey(_decryptHandle, &_key.at(0), _key.size())) != GPG_ERR_NO_ERROR)
	{
		aesCleanup();
		_out.printError("Error: Could not set key for decryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}

	_aesInitialized = true;
	_aesExchangeComplete = false;
	return true;
}

void HMW_LGW::aesCleanup()
{
	if(!_aesInitialized) return;
	_aesInitialized = false;
	if(_decryptHandle) gcry_cipher_close(_decryptHandle);
	if(_encryptHandle) gcry_cipher_close(_encryptHandle);
	_decryptHandle = nullptr;
	_encryptHandle = nullptr;
	_myIV.clear();
	_remoteIV.clear();
	_aesExchangeComplete = false;
}

std::vector<char> HMW_LGW::encrypt(const std::vector<char>& data)
{
	std::vector<char> encryptedData(data.size());
	if(!_encryptHandle) return encryptedData;

	gcry_error_t result;
	if((result = gcry_cipher_encrypt(_encryptHandle, &encryptedData.at(0), data.size(), &data.at(0), data.size())) != GPG_ERR_NO_ERROR)
	{
		GD::out.printError("Error encrypting data: " + _bl->hf.getGCRYPTError(result));
		_stopCallbackThread = true;
		return std::vector<char>();
	}
	return encryptedData;
}

std::vector<uint8_t> HMW_LGW::decrypt(std::vector<uint8_t>& data)
{
	std::vector<uint8_t> decryptedData(data.size());
	if(!_decryptHandle) return decryptedData;
	gcry_error_t result;
	if((result = gcry_cipher_decrypt(_decryptHandle, &decryptedData.at(0), data.size(), &data.at(0), data.size())) != GPG_ERR_NO_ERROR)
	{
		GD::out.printError("Error decrypting data: " + _bl->hf.getGCRYPTError(result));
		_stopCallbackThread = true;
		return std::vector<uint8_t>();
	}
	return decryptedData;
}

void HMW_LGW::sendKeepAlivePacket()
{
	try
    {
		if(!_initComplete) return; //Don't send keep alive during search
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastKeepAlive >= 20)
		{
			if(!_searchFinished)
			{
				_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
				_lastKeepAliveResponse = _lastKeepAlive;
				return;
			}
			if(_lastKeepAliveResponse < _lastKeepAlive)
			{
				_lastKeepAliveResponse = _lastKeepAlive;
				_stopped = true;
				return;
			}

			_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
			std::vector<char> packet;
			std::vector<char> payload{ 0x4B };
			buildPacket(packet, payload);
			_packetIndex++;
			send(packet, false);
		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::listen()
{
    try
    {
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);
		_lastTimePacket = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAliveResponse = _lastKeepAlive;

		std::vector<uint8_t> data;
        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		_out.printWarning("Warning: Connection closed. Trying to reconnect...");
        		reconnect();
        		continue;
        	}
			try
			{
				do
				{
					sendKeepAlivePacket();
					receivedBytes = _socket->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read from HMW-LGW: Too much data.");
							break;
						}
					}
				} while(receivedBytes == (unsigned)bufferMax);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty()) //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
				{
					continue;
				}
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				_out.printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				_out.printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			if(data.empty()) continue;
			if(data.size() > 1000000)
			{
				data.clear();
				continue;
			}

        	if(_bl->debugLevel >= 6)
        	{
        		_out.printDebug("Debug: Packet received on port " + _settings->port + ". Raw data:");
        		_out.printBinary(data);
        	}

        	processData(data);
        	data.clear();

        	_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool HMW_LGW::aesKeyExchange(std::vector<uint8_t>& data)
{
	try
	{
		std::string hex((char*)&data.at(0), data.size());
		if(_bl->debugLevel >= 5)
		{
			std::string temp = hex;
			_bl->hf.stringReplace(temp, "\r\n", "\\r\\n");
			_out.printDebug(std::string("Debug: AES key exchange packet received on port " + _settings->port + ": " + temp));
		}
		int32_t startPos = hex.find('\n');
		if(startPos == (signed)std::string::npos)
		{
			_out.printError("Error: Error communicating with HMW-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		startPos += 5;
		int32_t length = hex.find('\n', startPos);
		if(length == (signed)std::string::npos)
		{
			_out.printError("Error: Error communicating with HMW-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		length = length - startPos - 1;
		if(length <= 30)
		{
			_out.printError("Error: Error communicating with HMW-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		if(data.at(startPos - 4) == 'V' && data.at(startPos - 1) == ',')
		{
			uint8_t packetIndex = (_math.getNumber(data.at(startPos - 3)) << 4) + _math.getNumber(data.at(startPos - 2));
			packetIndex++;
			if(length != 32)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HMW-LGW. Received IV has wrong size.");
				return false;
			}
			_remoteIV.clear();
			std::string ivHex((char*)&data.at(startPos), length);
			_remoteIV = _bl->hf.getUBinary(ivHex);
			if(_remoteIV.size() != 16)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HMW-LGW. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5) _out.printDebug("HMW-LGW IV is: " + _bl->hf.getHexString(_remoteIV));

			gcry_error_t result;
			if((result = gcry_cipher_setiv(_encryptHandle, &_remoteIV.at(0), _remoteIV.size())) != GPG_ERR_NO_ERROR)
			{
				_stopCallbackThread = true;
				aesCleanup();
				_out.printError("Error: Could not set IV for encryption: " + _bl->hf.getGCRYPTError(result));
				return false;
			}

			std::vector<char> response = { 'V', _bl->hf.getHexChar(packetIndex >> 4), _bl->hf.getHexChar(packetIndex & 0xF), ',' };
			std::random_device rd;
			std::default_random_engine generator(rd());
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

			if(_bl->debugLevel >= 5) _out.printDebug("Homegear IV is: " + _bl->hf.getHexString(_myIV));

			if((result = gcry_cipher_setiv(_decryptHandle, &_myIV.at(0), _myIV.size())) != GPG_ERR_NO_ERROR)
			{
				_stopCallbackThread = true;
				aesCleanup();
				_out.printError("Error: Could not set IV for decryption: " + _bl->hf.getGCRYPTError(result));
				return false;
			}

			send(response, true);
			_aesExchangeComplete = true;
			return true;
		}
		else if(_remoteIV.empty())
		{
			_stopCallbackThread = true;
			_out.printError("Error: Error communicating with HMW-LGW. No IV was send from HMW-LGW.");
			return false;
		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void HMW_LGW::buildPacket(std::vector<char>& packet, const std::vector<char>& payload)
{
	try
	{
		std::vector<char> unescapedPacket;
		unescapedPacket.push_back(0xFD);
		int32_t size = payload.size() + 1; //Payload size plus message counter size - control byte
		unescapedPacket.push_back(size);
		unescapedPacket.push_back(_packetIndex);
		unescapedPacket.insert(unescapedPacket.end(), payload.begin(), payload.end());
		escapePacket(unescapedPacket, packet);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::escapePacket(const std::vector<char>& unescapedPacket, std::vector<char>& escapedPacket)
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
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::processPacket(std::vector<uint8_t>& packet)
{
	try
	{
		_out.printDebug(std::string("Debug: Packet received from HMW-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet)));
		if(packet.size() < 4) return;
		_requestsMutex.lock();
		if(_requests.find(packet.at(2)) != _requests.end())
		{
			std::shared_ptr<Request> request = _requests.at(packet.at(2));
			_requestsMutex.unlock();
			if(packet.at(3) == request->getResponseType())
			{
				request->response = packet;
				{
					std::lock_guard<std::mutex> lock(request->mutex);
					request->mutexReady = true;
				}
				request->conditionVariable.notify_one();
				return;
			}
		}
		else _requestsMutex.unlock();
		if(_initComplete) parsePacket(packet);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		if(!_aesExchangeComplete)
		{
			aesKeyExchange(data);
			return;
		}
		std::vector<uint8_t> decryptedData = decrypt(data);
		if(decryptedData.size() < 4) //4 is minimum size
		{
			_out.printWarning("Warning: Too small packet received on port " + _settings->port + ": " + _bl->hf.getHexString(decryptedData));
			return;
		}
		if(!_initComplete)
		{
			std::string packetString((char*)&decryptedData.at(0), decryptedData.size());
			if(_bl->debugLevel >= 5)
			{
				std::string temp = packetString;
				_bl->hf.stringReplace(temp, "\r\n", "\\r\\n");
				_out.printDebug(std::string("Debug: Packet received on port " + _settings->port + ": " + temp));
			}
			if(packetString.size() < 5 || packetString.at(0) != 'S')
			{
				_stopped = true;
				_out.printError("Error: First packet does not start with \"S\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
				return;
			}
			_initComplete = true;
			std::vector<char> response = { '>', packetString.at(1), packetString.at(2), ',', '0', '0', '0', '0', '\r', '\n' };
			send(response, false);
			return;
		}

		std::vector<uint8_t> packet;
		if(!_packetBuffer.empty()) packet = _packetBuffer;
		bool escapeByte = false;
		for(std::vector<uint8_t>::iterator i = decryptedData.begin(); i != decryptedData.end(); ++i)
		{
			if(!packet.empty() && *i == 0xfd)
			{
				processPacket(packet);
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

		if(packet.size() < 5) _packetBuffer = packet;
		else
		{
			processPacket(packet);
			_packetBuffer.clear();
		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMW_LGW::parsePacket(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.empty()) return;
		if(packet.at(3) == 0x61 && packet.size() == 5)
		{
			if(packet.at(4) == 0)
			{
				if(_bl->debugLevel >= 5) _out.printDebug("Debug: Keep alive response received on port " + _settings->port + ".");
				_lastKeepAliveResponse = BaseLib::HelperFunctions::getTimeSeconds();
			}
			else if(packet.at(4) == 1)
			{
				_out.printDebug("Debug: ACK response received on port " + _settings->port + ".");
			}
			else if(packet.at(4) == 2)
			{
				_out.printWarning("Warning: NACK received.");
			}
			else
			{
				_out.printWarning("Warning: Unknown ACK received.");
			}
		}
		else if(packet.at(3) == 0x63) //Discovery finished
		{
			_searchFinished = true;
		}
		else if(packet.at(3) == 0x64) //Device found
		{
			if(packet.size() >= 8)
			{
				int32_t address = (packet[4] << 24) + (packet[5] << 16) + (packet[6] << 8) + packet[7];
				_searchResult.push_back(address);
				GD::out.printMessage("Peer found with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8));
			}
			else GD::out.printError("Error: \"Device found\" packet with wrong size received.");
		}
		else if(packet.at(3) == 0x65) //Packet received
		{
			std::shared_ptr<HMWiredPacket> hmwiredPacket(new HMWiredPacket(packet, true, BaseLib::HelperFunctions::getTime()));
			_lastPacketReceived = BaseLib::HelperFunctions::getTime();
			raisePacketReceived(hmwiredPacket);
		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
