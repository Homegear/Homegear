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

#include "HMW-LGW.h"
#include "../GD.h"

namespace HMWired
{
HMW_LGW::HMW_LGW(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IHMWiredInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "LAN Gateway \"" + settings->id + "\": ");

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
    		_sendMutex.unlock();
    		return;
    	}

		std::shared_ptr<HMWiredPacket> bidCoSPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!bidCoSPacket) return;
		if(bidCoSPacket->messageType() == 0x02 && packet->senderAddress() == _myAddress && bidCoSPacket->controlByte() == 0x80 && bidCoSPacket->payload()->size() == 1 && bidCoSPacket->payload()->at(0) == 0)
		{
			_out.printDebug("Debug: Ignoring ACK packet.", 6);
			_lastPacketSent = BaseLib::HelperFunctions::getTime();
			return;
		}
		if((bidCoSPacket->controlByte() & 0x01) && packet->senderAddress() == _myAddress && (bidCoSPacket->payload()->empty() || (bidCoSPacket->payload()->size() == 1 && bidCoSPacket->payload()->at(0) == 0)))
		{
			_out.printDebug("Debug: Ignoring wake up packet.", 6);
			_lastPacketSent = BaseLib::HelperFunctions::getTime();
			return;
		}

		std::vector<char> packetBytes = bidCoSPacket->byteArraySigned();
		//Allow sender address 0 for firmware updates
		if(bidCoSPacket->senderAddress() != 0 && bidCoSPacket->senderAddress() != _myAddress)
		{
			_out.printError("Error: Can't send packet, because sender address is not mine: " + _bl->hf.getHexString(packetBytes));
			return;
		}
		if(!_initComplete)
		{
			_out.printWarning(std::string("Warning: !!!Not!!! sending packet, because init sequence is not complete: ") + _bl->hf.getHexString(packetBytes));
			return;
		}

		if(_bl->debugLevel >= 4) _out.printInfo("Info: Sending (" + _settings->id + "): " + _bl->hf.getHexString(packetBytes));

		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1, 2, 0, 0 };
			payload.insert(payload.end(), packetBytes.begin() + 1, packetBytes.end());
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() == 9  && responsePacket.at(6) == 8)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				//Resend
				continue;
			}
			else if(responsePacket.size() >= 9  && responsePacket.at(6) != 4)
			{
				//I assume byte 7 with i. e. 0x02 is the message type
				if(responsePacket.at(6) == 0x0D)
				{
					//0x0D is returned, when there is no response to the A003 packet and if the 8002
					//packet doesn't match
					//Example: FD000501BC040D025E14
					_out.printWarning("Warning: AES handshake failed for packet: " + _bl->hf.getHexString(packetBytes));
					return;
				}
				else if(responsePacket.at(6) == 0x03)
				{
					//Example: FD001001B3040302391A8002282BE6FD26EF00C54D
					_out.printDebug("Debug: Packet was sent successfully: " + _bl->hf.getHexString(packetBytes));
				}
				else if(responsePacket.at(6) == 0x0C)
				{
					//Example: FD00140168040C0228128002282BE6FD26EF00938ABE1C163D
					_out.printDebug("Debug: Packet was sent successfully and AES handshake was successful: " + _bl->hf.getHexString(packetBytes));
				}
				if(responsePacket.size() == 9)
				{
					_out.printDebug("Debug: Packet was sent successfully: " + _bl->hf.getHexString(packetBytes));
					break;
				}
				parsePacket(responsePacket);
				break;
			}
			else if(responsePacket.size() == 9  && responsePacket.at(6) == 4)
			{
				//The gateway tries to send the packet three times, when there is no response
				//NACK (0404) is returned
				//NACK is sometimes also returned when the AES handshake wasn't successful (i. e.
				//the handshake after sending a wake up packet)
				_out.printInfo("Info: No answer to packet " + _bl->hf.getHexString(packetBytes));
				return;
			}
			if(j == 2)
			{
				_out.printInfo("Info: No response from HMW-LGW to packet " + _bl->hf.getHexString(packetBytes));
				return;
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

void HMW_LGW::getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t messageCounter, uint8_t responseControlByte, uint8_t responseType)
{
	try
    {
		if(packet.size() < 8 || _stopped) return;
		std::shared_ptr<Request> request(new Request(responseControlByte, responseType));
		_requestsMutex.lock();
		_requests[messageCounter] = request;
		_requestsMutex.unlock();
		request->mutex.try_lock(); //Lock and return immediately
		send(packet, false);
		if(!request->mutex.try_lock_for(std::chrono::milliseconds(10000)))
		{
			_out.printError("Error: No response received to packet: " + _bl->hf.getHexString(packet));
		}
		request->mutex.unlock();
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
		if(!_initComplete) return;
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastKeepAlive >= 5)
		{
			if(_lastKeepAliveResponse < _lastKeepAlive)
			{
				_lastKeepAliveResponse = _lastKeepAlive;
				_stopped = true;
				return;
			}

			_lastKeepAlive = BaseLib::HelperFunctions::getTimeSeconds();
			std::vector<char> packet;
			std::vector<char> payload{ 0, 8 };
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
			if(data.size() == 1 && data.at(0) == 0xFD)
			{
				//Happens sometimes. The rest of the packet comes with the next read.
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
		std::string hex(&data.at(0), &data.at(0) + data.size());
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
			std::string ivHex(&data.at(startPos), &data.at(startPos) + length);
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
		unescapedPacket.push_back(size >> 8);
		unescapedPacket.push_back(size & 0xFF);
		unescapedPacket.push_back(payload.at(0));
		unescapedPacket.push_back(_packetIndex);
		unescapedPacket.insert(unescapedPacket.end(), payload.begin() + 1, payload.end());
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
		if(packet.size() < 8) return;
		_requestsMutex.lock();
		if(_requests.find(packet.at(4)) != _requests.end())
		{
			std::shared_ptr<Request> request = _requests.at(packet.at(4));
			_requestsMutex.unlock();
			if(packet.at(3) == request->getResponseControlByte() && packet.at(5) == request->getResponseType())
			{
				request->response = packet;
				request->mutex.unlock();
				return;
			}
			//E. g.: FD0004000004001938
			else if(packet.size() == 9 && packet.at(6) == 0 && packet.at(5) == 4 && packet.at(3) == 0)
			{
				_out.printError("Error: Something is wrong with your HMW-LGW. You probably need to replace it. Check if it works with a CCU.");
				request->response = packet;
				request->mutex.unlock();
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
		if(!_initComplete && _packetIndex == 0 && decryptedData.at(0) == 'S')
		{
			std::string packetString(&decryptedData.at(0), &decryptedData.at(0) + decryptedData.size());
			if(_bl->debugLevel >= 5)
			{
				std::string temp = packetString;
				_bl->hf.stringReplace(temp, "\r\n", "\\r\\n");
				_out.printDebug(std::string("Debug: Packet received on port " + _settings->port + ": " + temp));
			}
			_requestsMutex.lock();
			if(_requests.find(0) != _requests.end())
			{
				_requests.at(0)->response = decryptedData;
				_requests.at(0)->mutex.unlock();
			}
			_requestsMutex.unlock();
			return;
		}

		std::vector<uint8_t> packet;
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
		processPacket(packet);
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
		/*
		if(packet.empty()) return;
		if(packet.at(5) == 4 && packet.at(3) == 0 && packet.size() == 10 && packet.at(6) == 2 && packet.at(7) == 0)
		{
			if(_bl->debugLevel >= 5) _out.printDebug("Debug: Keep alive response received on port " + _settings->port + ".");
			_lastKeepAliveResponse = BaseLib::HelperFunctions::getTimeSeconds();
		}
		else if((packet.at(5) == 5 || (packet.at(5) == 4 && packet.at(6) != 7)) && packet.at(3) == 1 && packet.size() >= 20)
		{
			std::vector<uint8_t> binaryPacket({(uint8_t)(packet.size() - 11)});
			binaryPacket.insert(binaryPacket.end(), packet.begin() + 9, packet.end() - 2);
			int32_t rssi = packet.at(8); //Range should be from 0x0B to 0x8A. 0x0B is -11dBm 0x8A -138dBm.
			rssi *= -1;
			//Convert to TI CC1101 format
			if(rssi <= -75) rssi = ((rssi + 74) * 2) + 256;
			else rssi = (rssi + 74) * 2;
			binaryPacket.push_back(rssi);
			std::shared_ptr<HMWiredPacket> hmwiredPacket(new HMWiredPacket(binaryPacket, true, BaseLib::HelperFunctions::getTime()));
			//Don't use (packet.at(6) & 1) here. That bit is set for non-AES packets, too
			//packet.at(6) == 3 and packet.at(7) == 0 is set on pairing packets: FD0020018A0503002494840026219BFD00011000AD4C4551303030333835365803FFFFCB99
			if(packet.at(5) == 5 && ((packet.at(6) & 3) == 3 || (packet.at(6) & 5) == 5))
			{
				//Accept pairing packets from HM-TC-IT-WM-W-EU (version 1.0) and maybe other devices.
				//For these devices the handshake is never executed, but the "failed bit" set anyway: Bug
				if(!(hmwiredPacket->controlByte() & 0x4) || hmwiredPacket->messageType() != 0 || hmwiredPacket->payload()->size() != 17)
				{
					_out.printWarning("Warning: AES handshake failed for packet: " + _bl->hf.getHexString(binaryPacket));
					return;
				}
			}
			else if(_bl->debugLevel >= 5 && packet.at(5) == 5 && (packet.at(6) & 3) == 2)
			{
				_out.printDebug("Debug: AES handshake was successful for packet: " + _bl->hf.getHexString(binaryPacket));
			}
			_lastPacketReceived = BaseLib::HelperFunctions::getTime();
			raisePacketReceived(hmwiredPacket);
		}
		*/
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
