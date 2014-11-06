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
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "LAN Gateway \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));
	_socketKeepAlive = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(!settings)
	{
		_out.printCritical("Critical: Error initializing HM-LGW. Settings pointer is empty.");
		return;
	}
	if(settings->lanKey.empty())
	{
		_out.printError("Error: No security key specified in physicalinterfaces.conf.");
		return;
	}

	if(settings->rfKey.empty())
	{
		_out.printError("Error: No RF AES key specified in physicalinterfaces.conf for communication with your BidCoS devices.");
	}

	if(!settings->rfKey.empty())
	{
		_rfKey = _bl->hf.getUBinary(settings->rfKey);
		if(_rfKey.size() != 16)
		{
			_out.printError("Error: The RF AES key specified in physicalinterfaces.conf for communication with your BidCoS devices is not a valid hexadecimal string.");
			_rfKey.clear();
		}
	}

	if(!settings->oldRFKey.empty())
	{
		_oldRFKey = _bl->hf.getUBinary(settings->oldRFKey);
		if(_oldRFKey.size() != 16)
		{
			_out.printError("Error: The old RF AES key specified in physicalinterfaces.conf for communication with your BidCoS devices is not a valid hexadecimal string.");
			_oldRFKey.clear();
		}
	}

	if(!_rfKey.empty() && settings->currentRFKeyIndex == 0)
	{
		_out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is \"0\". That means, the default AES key will be used (not yours!).");
		_rfKey.clear();
	}

	if(!_oldRFKey.empty() && settings->currentRFKeyIndex == 1)
	{
		_out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is \"1\" but \"OldRFKey\" is specified. That is not possible. Increase the key index to \"2\".");
		_oldRFKey.clear();
	}

	if(!_oldRFKey.empty() && _rfKey.empty())
	{
		_oldRFKey.clear();
		if(settings->currentRFKeyIndex > 0)
		{
			_out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is greater than \"0\" but no AES key is specified. Setting it to \"0\".");
			settings->currentRFKeyIndex = 0;
		}
	}

	if(_oldRFKey.empty() && settings->currentRFKeyIndex > 1)
	{
		_out.printWarning("Warning: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is larger than \"1\" but \"OldRFKey\" is not specified. Please set your old RF key or set key index to \"1\".");
	}

	if(settings->currentRFKeyIndex > 253)
	{
		_out.printError("Error: The RF AES key index specified in physicalinterfaces.conf for communication with your BidCoS devices is greater than \"253\". That is not allowed.");
		settings->currentRFKeyIndex = 253;
	}
}

HM_LGW::~HM_LGW()
{
	try
	{
		_stopCallbackThread = true;
		if(_initThread.joinable()) _initThread.join();
		if(_listenThread.joinable()) _listenThread.join();
		if(_listenThreadKeepAlive.joinable()) _listenThreadKeepAlive.join();
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

void HM_LGW::enableUpdateMode()
{
	try
	{
		if(!_initComplete || _stopped) return;
		_updateMode = true;
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 0, 7 };
			payload.push_back(0xE9);
			payload.push_back(0xCA);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() >= 9  && responsePacket.at(6) == 1)
			{
				_out.printInfo("Info: Update mode enabled.");
				break;
			}
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not enable update mode.");
				return;
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

void HM_LGW::disableUpdateMode()
{
	try
	{
		if(!_initComplete || _stopped) return;
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 0, 6 };
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() >= 9  && responsePacket.at(6) == 1)
			{
				_out.printInfo("Info: Update mode disabled.");
				break;
			}
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not enable update mode.");
				return;
			}
		}
		_updateMode = false;
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

void HM_LGW::addPeer(PeerInfo peerInfo)
{
	try
	{
		if(peerInfo.address == 0) return;
		_peersMutex.lock();
		_peers[peerInfo.address] = peerInfo;
		if(_initComplete) sendPeer(peerInfo);
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
    _peersMutex.unlock();
}

void HM_LGW::addPeers(std::vector<PeerInfo>& peerInfos)
{
	try
	{
		_peersMutex.lock();
		for(std::vector<PeerInfo>::iterator i = peerInfos.begin(); i != peerInfos.end(); ++i)
		{
			if(i->address == 0) continue;
			_peers[i->address] = *i;
			if(_initComplete) sendPeer(*i);
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
    _peersMutex.unlock();
}

void HM_LGW::sendPeers()
{
	try
	{
		_peersMutex.lock();
		for(std::map<int32_t, PeerInfo>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			sendPeer(i->second);
		}
		_initComplete = true; //Init complete is set here within _peersMutex, so there is no conflict with addPeer() and peers are not sent twice
		_out.printInfo("Info: Peer sending completed.");
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
    _peersMutex.unlock();

    //There is no duty cycle => tested with version 1.1.3
    /*std::thread t1(&HM_LGW::dutyCycleTest, this, 0x123456);
	t1.detach();*/
}

void HM_LGW::dutyCycleTest(int32_t destinationAddress)
{
	for(int32_t i = 0; i < 1000000; i++)
    {
    	std::vector<uint8_t> payload { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(i, 0x80, 0x10, _myAddress, destinationAddress, payload));
		sendPacket(packet);
		usleep(10000);
    }
}

void HM_LGW::sendPeer(PeerInfo& peerInfo)
{
	try
	{
		if(GD::bl->debugLevel > 4) GD::out.printDebug("Debug: Sending peer to LGW \"" + _settings->id + "\": Address " + GD::bl->hf.getHexString(peerInfo.address, 6) + ", AES enabled " + std::to_string(peerInfo.aesEnabled) + ", AES map " + GD::bl->hf.getHexString(peerInfo.getAESChannelMap()) + ".");
		for(int32_t i = 0; i < 2; i++) //The CCU sends this packet two times, I don't know why
		{
			//Get current config
			for(int32_t j = 0; j < 40; j++)
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload{ 1, 6 };
				payload.push_back(peerInfo.address >> 16);
				payload.push_back((peerInfo.address >> 8) & 0xFF);
				payload.push_back(peerInfo.address & 0xFF);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(0);
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
				if(responsePacket.size() >= 21  && responsePacket.at(6) == 7) break;
				else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
				{
					//Operation pending
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}
				if(j == 2)
				{
					_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
					return;
				}
			}
		}
		//Reset all channels
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1, 0xA };
			payload.push_back(peerInfo.address >> 16);
			payload.push_back((peerInfo.address >> 8) & 0xFF);
			payload.push_back(peerInfo.address & 0xFF);
			payload.push_back(0);

			for(std::map<int32_t, bool>::iterator k = peerInfo.aesChannels.begin(); k != peerInfo.aesChannels.end(); ++k)
			{
				payload.push_back(k->first);
			}
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() >= 9  && responsePacket.at(6) == 1) break;
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
				return;
			}
		}
		//Get current config again
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1, 6 };
			payload.push_back(peerInfo.address >> 16);
			payload.push_back((peerInfo.address >> 8) & 0xFF);
			payload.push_back(peerInfo.address & 0xFF);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() >= 21  && responsePacket.at(6) == 7) break;
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
				return;
			}
		}
		if(peerInfo.wakeUp)
		{
			//Enable sending of wake up packet or just request config again?
			for(int32_t j = 0; j < 40; j++)
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload{ 1, 6 };
				payload.push_back(peerInfo.address >> 16);
				payload.push_back((peerInfo.address >> 8) & 0xFF);
				payload.push_back(peerInfo.address & 0xFF);
				payload.push_back(0);
				payload.push_back(1);
				payload.push_back(1);
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
				if(responsePacket.size() >= 21  && responsePacket.at(6) == 7) break;
				else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
				{
					//Operation pending
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}
				if(j == 2)
				{
					_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
					return;
				}
			}
		}
		//Set key index and enable sending of wake up packet.
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1, 6 };
			payload.push_back(peerInfo.address >> 16);
			payload.push_back((peerInfo.address >> 8) & 0xFF);
			payload.push_back(peerInfo.address & 0xFF);
			payload.push_back(peerInfo.keyIndex);
			payload.push_back((char)peerInfo.wakeUp);
			payload.push_back((char)peerInfo.wakeUp);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() >= 21  && responsePacket.at(6) == 7) break;
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
				return;
			}
		}
		//Enable AES
		if(peerInfo.aesEnabled)
		{
			//Delete old configuration
			for(int32_t j = 0; j < 40; j++)
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload{ 1, 9 };
				payload.push_back(peerInfo.address >> 16);
				payload.push_back((peerInfo.address >> 8) & 0xFF);
				payload.push_back(peerInfo.address & 0xFF);
				payload.push_back(0);

				bool aesEnabled = false;
				for(std::map<int32_t, bool>::iterator k = peerInfo.aesChannels.begin(); k != peerInfo.aesChannels.end(); ++k)
				{
					if(k->second)
					{
						aesEnabled = true;
						payload.push_back(k->first);
					}
				}
				if(!aesEnabled) break;
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
				if(responsePacket.size() >= 9  && responsePacket.at(6) == 1) break;
				else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
				{
					//Operation pending
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}
				if(j == 2)
				{
					_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
					return;
				}
			}
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

void HM_LGW::setAES(PeerInfo peerInfo, int32_t channel)
{
	try
	{
		if(!_initComplete || _stopped) return;
		try
		{
			_peersMutex.lock();
			_peers[peerInfo.address] = peerInfo;
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
		_peersMutex.unlock();
		if(peerInfo.aesChannels.find(channel) == peerInfo.aesChannels.end()) return;
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1 };
			if(peerInfo.aesChannels.at(channel)) payload.push_back(9);
			else payload.push_back(0xA);
			payload.push_back(peerInfo.address >> 16);
			payload.push_back((peerInfo.address >> 8) & 0xFF);
			payload.push_back(peerInfo.address & 0xFF);
			payload.push_back(0);
			payload.push_back(channel);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() >= 9  && responsePacket.at(6) == 1) break;
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not set AES for peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
				return;
			}
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

void HM_LGW::setWakeUp(PeerInfo peerInfo)
{
	try
	{
		if(!_initComplete || _stopped) return;
		try
		{
			_peersMutex.lock();
			_peers[peerInfo.address] = peerInfo;
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
		_peersMutex.unlock();
		for(int32_t j = 0; j < 40; j++)
		{
			std::vector<uint8_t> responsePacket;
			std::vector<char> requestPacket;
			std::vector<char> payload{ 1, 6 };
			payload.push_back(peerInfo.address >> 16);
			payload.push_back((peerInfo.address >> 8) & 0xFF);
			payload.push_back(peerInfo.address & 0xFF);
			payload.push_back(peerInfo.keyIndex);
			payload.push_back((char)peerInfo.wakeUp); //CCU2 sets this for wake up, too. No idea, what the meaning is.
			payload.push_back((char)peerInfo.wakeUp); //This actually enables the sending of the wake up packet
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() >= 21  && responsePacket.at(6) == 7) break;
			else if(responsePacket.size() == 9 && responsePacket.at(6) == 8)
			{
				//Operation pending
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}
			if(j == 2)
			{
				_out.printError("Error: Could not add peer with address 0x" + _bl->hf.getHexString(peerInfo.address, 6));
				return;
			}
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

void HM_LGW::removePeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end()) _peers.erase(address);
		if(_initComplete)
		{
			for(int32_t i = 0; i < 3; i++)
			{
				std::vector<uint8_t> responsePacket;
				std::vector<char> requestPacket;
				std::vector<char> payload{ 1, 7 };
				payload.push_back(address >> 16);
				payload.push_back((address >> 8) & 0xFF);
				payload.push_back(address & 0xFF);
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
				if(responsePacket.size() >= 13  && responsePacket.at(6) == 7) break;
				if(i == 2)
				{
					_out.printError("Error: Could not remove peer with address 0x" + _bl->hf.getHexString(address, 6));
				}
			}
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
    _peersMutex.unlock();
}

void HM_LGW::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
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

		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
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
				_out.printInfo("Info: No response from HM-LGW to packet " + _bl->hf.getHexString(packetBytes));
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

void HM_LGW::getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t messageCounter, uint8_t responseControlByte, uint8_t responseType)
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

void HM_LGW::send(const std::vector<char>& data, bool raw)
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
    		_out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->portKeepAlive + "): " + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
    		_sendMutexKeepAlive.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5)
        {
            _out.printDebug(std::string("Debug: Sending (Port " + _settings->portKeepAlive + "): ") + std::string(&data.at(0), &data.at(0) + (data.size() - 2)));
        }
    	(!raw) ? _socketKeepAlive->proofwrite(encryptedData) : _socketKeepAlive->proofwrite(data);
    	 _sendMutexKeepAlive.unlock();
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
    _sendMutexKeepAlive.unlock();
}

void HM_LGW::doInit()
{
	try
	{
		_packetIndex = 0;

		if(!GD::family->getCentral())
		{
			_stopCallbackThread = true;
			_out.printError("Error: Could not get central address. Stopping listening.");
			return;
		}

		_myAddress = GD::family->getCentral()->physicalAddress();

		if(_stopped) return;
		//BidCoS-over-LAN" packet
		std::shared_ptr<Request> request(new Request(0, 0));
		_requestsMutex.lock();
		_requests[0] = request;
		_requestsMutex.unlock();
		request->mutex.try_lock(); //Lock and return immediately
		_initStarted = true;
		if(!request->mutex.try_lock_for(std::chrono::milliseconds(30000)))
		{
			_out.printError("Error: No init packet received.");
			_stopped = true;
			request->mutex.unlock();
			return;
		}
		request->mutex.unlock();
		std::string packetString(request->response.begin(), request->response.end());
		_requestsMutex.lock();
		_requests.erase(0);
		_requestsMutex.unlock();
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packetString, ',');
		if(parts.size() != 2 || parts.at(0).size() != 3 || parts.at(0).at(0) != 'S' || parts.at(1).size() < 15 || parts.at(1).compare(0, 15, "BidCoS-over-LAN") != 0)
		{
			_stopped = true;
			_out.printError("Error: First packet does not start with \"S\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
			return;
		}
		uint8_t packetIndex = (_math.getNumber(parts.at(0).at(1)) << 4) + _math.getNumber(parts.at(0).at(2));
		std::vector<char> response = { '>', _bl->hf.getHexChar(packetIndex >> 4), _bl->hf.getHexChar(packetIndex & 0xF), ',', '0', '0', '0', '0', '\r', '\n' };
		send(response, false);

		while(!_initCompleteKeepAlive && !_stopCallbackThread && !_stopped)
    	{
    		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    	}

		//I think the gateway needs a moment here
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		//There are two paths depending if GW sends a "Co_CPU_BL" or not.
		bool cpuBLPacket = false;

		//1st packet
		if(_stopped) return;
		std::vector<uint8_t> responsePacket;
		std::vector<char> requestPacket;
		std::vector<char> payload{ 0, 3 };
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, 0, 0, 0);
		if(responsePacket.size() == 17)
		{
			packetString.clear();
			packetString.insert(packetString.end(), responsePacket.begin() + 6, responsePacket.end() - 2);
			if(packetString == "Co_CPU_BL")
			{
				_out.printDebug("Debug: Co_CPU_BL packet received.");
				cpuBLPacket = true;
			}
		}
		else if(responsePacket.size() == 18)
		{
			packetString.clear();
			packetString.insert(packetString.end(), responsePacket.begin() + 6, responsePacket.end() - 2);
			if(packetString != "Co_CPU_App")
			{
				_out.printError("Error: Unknown packet received in response to init packet. Reconnecting...");
				_stopped = true;
				return;
			}
			else _out.printDebug("Debug: Co_CPU_App packet received.");
		}
		else
		{
			_out.printError("Error: Unknown packet received in response to init packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//2nd packet
		if(cpuBLPacket)
		{
			for(int32_t i = 0; i < 3; i++)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				if(_stopped) return;
				responsePacket.clear();
				requestPacket.clear();
				buildPacket(requestPacket, payload);
				_packetIndex++;
				getResponse(requestPacket, responsePacket, 0, 0, 0);
				if(responsePacket.size() == 18)
				{
					packetString.clear();
					packetString.insert(packetString.end(), responsePacket.begin() + 6, responsePacket.end() - 2);
					if(packetString == "Co_CPU_App")
					{
						_out.printDebug("Debug: Co_CPU_App packet received.");
						break;
					}
					else
					{
						_out.printError("Error: Unknown packet received in response to init packet. Reconnecting...");
						_stopped = true;
						return;
					}
				}
				else if(responsePacket.size() == 17)
				{
					packetString.clear();
					packetString.insert(packetString.end(), responsePacket.begin() + 6, responsePacket.end() - 2);
					if(packetString == "Co_CPU_BL")
					{
						_out.printDebug("Debug: Co_CPU_BL packet received a second time.");
						cpuBLPacket = true;
						continue;
					}
					else
					{
						_out.printError("Error: Unknown packet received in response to init packet. Reconnecting...");
						_stopped = true;
						return;
					}
				}
				else
				{
					_out.printError("Error: Unknown packet received in response to init packet. Reconnecting...");
					_stopped = true;
					return;
				}
			}
		}

		//3rd packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		payload.push_back(0);
		payload.push_back(2);
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//4th packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		payload.push_back(0);
		payload.push_back(0xA);
		payload.push_back(0);
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//5th packet
		if(cpuBLPacket)
		{
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(0xA);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		//6th packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		payload.push_back(0);
		payload.push_back(9);
		payload.push_back(0);
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//7th packet
		if(cpuBLPacket)
		{
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(9);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}

			//8th packet
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(0xA);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		//9th packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		const auto timePoint = std::chrono::system_clock::now();
		time_t t = std::chrono::system_clock::to_time_t(timePoint);
		tm* localTime = std::localtime(&t);
		uint32_t time = (uint32_t)t;
		payload.push_back(0);
		payload.push_back(0xE);
		payload.push_back(time >> 24);
		payload.push_back((time >> 16) & 0xFF);
		payload.push_back((time >> 8) & 0xFF);
		payload.push_back(time & 0xFF);
		payload.push_back(localTime->tm_gmtoff / 1800);
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//10th packet
		if(cpuBLPacket)
		{
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(9);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		//11th packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		payload.push_back(1);
		payload.push_back(3);
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
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//12th packet
		if(_settings->currentRFKeyIndex > 1 && !_settings->oldRFKey.empty())
		{
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(1);
			payload.push_back(0xF);
			std::vector<char> rfKey = _bl->hf.getBinary(_settings->oldRFKey);
			payload.insert(payload.end(), rfKey.begin(), rfKey.end());
			payload.push_back(_settings->currentRFKeyIndex - 1);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		//13th packet
		if(_stopped) return;
		responsePacket.clear();
		requestPacket.clear();
		payload.clear();
		payload.push_back(1);
		payload.push_back(0);
		payload.push_back(_myAddress >> 16);
		payload.push_back((_myAddress >> 8) & 0xFF);
		payload.push_back(_myAddress & 0xFF);
		buildPacket(requestPacket, payload);
		_packetIndex++;
		getResponse(requestPacket, responsePacket, _packetIndex - 1, 1, 4);
		if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
		{
			if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
			_stopped = true;
			return;
		}

		if(!cpuBLPacket)
		{
			//Packet with message counter 7
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(0xA);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}

			//Packet with message counter 8
			if(_stopped) return;
			responsePacket.clear();
			requestPacket.clear();
			payload.clear();
			payload.push_back(0);
			payload.push_back(9);
			payload.push_back(0);
			buildPacket(requestPacket, payload);
			_packetIndex++;
			getResponse(requestPacket, responsePacket, _packetIndex - 1, 0, 4);
			if(responsePacket.size() < 9 || responsePacket.at(6) == 4)
			{
				if(responsePacket.size() >= 9) _out.printError("Error: NACK received in response to init sequence packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		_out.printInfo("Info: Init queue completed. Sending peers...");
		if(_stopped) return;
		sendPeers();
		if(_stopped) return;
		disableUpdateMode();
		return;
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
}

void HM_LGW::startListening()
{
	try
	{
		stopListening();
		if(_rfKey.empty())
		{
			_out.printError("Error: Cannot start listening, because rfKey is not specified.");
			return;
		}
		aesInit();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_socket->setReadTimeout(1000000);
		_socketKeepAlive = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->portKeepAlive, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_socketKeepAlive->setReadTimeout(1000000);
		_out.printDebug("Connecting to HM-LGW with hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		_listenThread = std::thread(&HM_LGW::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		_listenThreadKeepAlive = std::thread(&HM_LGW::listenKeepAlive, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThreadKeepAlive.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		_initThread = std::thread(&HM_LGW::doInit, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _initThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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

void HM_LGW::reconnect()
{
	try
	{
		_socket->close();
		_socketKeepAlive->close();
		if(_initThread.joinable()) _initThread.join();
		aesInit();
		_requestsMutex.lock();
		_requests.clear();
		_requestsMutex.unlock();
		_initStarted = false;
		_initComplete = false;
		_initCompleteKeepAlive = false;
		_firstPacket = true;
		_out.printDebug("Connecting to HM-LGW with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_socketKeepAlive->open();
		_out.printInfo("Connected to HM-LGW with hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_initThread = std::thread(&HM_LGW::doInit, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _initThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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

void HM_LGW::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		if(_initThread.joinable()) _initThread.join();
		if(_listenThread.joinable()) _listenThread.join();
		if(_listenThreadKeepAlive.joinable()) _listenThreadKeepAlive.join();
		_stopCallbackThread = false;
		_socket->close();
		_socketKeepAlive->close();
		aesCleanup();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
		_sendMutexKeepAlive.unlock();
		_requestsMutex.lock();
		_requests.clear();
		_requestsMutex.unlock();
		_initStarted = false;
		_initComplete = false;
		_initCompleteKeepAlive = false;
		_firstPacket = true;
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

bool HM_LGW::aesInit()
{
	aesCleanup();

	if(_settings->lanKey.empty())
	{
		_out.printError("Error: No AES key specified in physicalinterfaces.conf for communication with your HM-LGW.");
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

	if((result = gcry_cipher_open(&_encryptHandleKeepAlive, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE)) != GPG_ERR_NO_ERROR)
	{
		_encryptHandleKeepAlive = nullptr;
		_out.printError("Error initializing cypher handle for keep alive encryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}
	if(!_encryptHandleKeepAlive)
	{
		_out.printError("Error cypher handle for keep alive encryption is nullptr.");
		return false;
	}
	if((result = gcry_cipher_setkey(_encryptHandleKeepAlive, &_key.at(0), _key.size())) != GPG_ERR_NO_ERROR)
	{
		aesCleanup();
		_out.printError("Error: Could not set key for keep alive encryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}

	if((result = gcry_cipher_open(&_decryptHandleKeepAlive, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE)) != GPG_ERR_NO_ERROR)
	{
		_decryptHandleKeepAlive = nullptr;
		_out.printError("Error initializing cypher handle for keep alive decryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}
	if(!_decryptHandleKeepAlive)
	{
		_out.printError("Error cypher handle for keep alive decryption is nullptr.");
		return false;
	}
	if((result = gcry_cipher_setkey(_decryptHandleKeepAlive, &_key.at(0), _key.size())) != GPG_ERR_NO_ERROR)
	{
		aesCleanup();
		_out.printError("Error: Could not set key for keep alive decryption: " + _bl->hf.getGCRYPTError(result));
		return false;
	}

	_aesInitialized = true;
	_aesExchangeComplete = false;
	_aesExchangeKeepAliveComplete = false;
	return true;
}

void HM_LGW::aesCleanup()
{
	if(!_aesInitialized) return;
	_aesInitialized = false;
	if(_decryptHandle) gcry_cipher_close(_decryptHandle);
	if(_encryptHandle) gcry_cipher_close(_encryptHandle);
	if(_decryptHandleKeepAlive) gcry_cipher_close(_decryptHandleKeepAlive);
	if(_encryptHandleKeepAlive) gcry_cipher_close(_encryptHandleKeepAlive);
	_decryptHandle = nullptr;
	_encryptHandle = nullptr;
	_decryptHandleKeepAlive = nullptr;
	_encryptHandleKeepAlive = nullptr;
	_myIV.clear();
	_remoteIV.clear();
	_myIVKeepAlive.clear();
	_remoteIVKeepAlive.clear();
	_aesExchangeComplete = false;
	_aesExchangeKeepAliveComplete = false;
}

std::vector<char> HM_LGW::encrypt(const std::vector<char>& data)
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

std::vector<uint8_t> HM_LGW::decrypt(std::vector<uint8_t>& data)
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

std::vector<char> HM_LGW::encryptKeepAlive(std::vector<char>& data)
{
	std::vector<char> encryptedData(data.size());
	if(!_encryptHandleKeepAlive) return encryptedData;
	gcry_error_t result;
	if((result = gcry_cipher_encrypt(_encryptHandleKeepAlive, &encryptedData.at(0), data.size(), &data.at(0), data.size())) != GPG_ERR_NO_ERROR)
	{
		GD::out.printError("Error encrypting keep alive data: " + _bl->hf.getGCRYPTError(result));
		_stopCallbackThread = true;
		return std::vector<char>();
	}
	return encryptedData;
}

std::vector<uint8_t> HM_LGW::decryptKeepAlive(std::vector<uint8_t>& data)
{
	std::vector<uint8_t> decryptedData(data.size());
	if(!_decryptHandleKeepAlive) return decryptedData;
	gcry_error_t result;
	if((result = gcry_cipher_decrypt(_decryptHandleKeepAlive, &decryptedData.at(0), data.size(), &data.at(0), data.size())) != GPG_ERR_NO_ERROR)
	{
		GD::out.printError("Error decrypting keep alive data: " + _bl->hf.getGCRYPTError(result));
		_stopCallbackThread = true;
		return std::vector<uint8_t>();
	}
	return decryptedData;
}

void HM_LGW::sendKeepAlivePacket1()
{
	try
    {
		if(!_initComplete) return;
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

void HM_LGW::listen()
{
    try
    {
    	while(!_initStarted && !_stopCallbackThread)
    	{
    		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    	}

    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);
		_lastTimePacket = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAlive1 = BaseLib::HelperFunctions::getTimeSeconds();
		_lastKeepAliveResponse1 = _lastKeepAlive1;

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
					if(BaseLib::HelperFunctions::getTimeSeconds() - _lastTimePacket > 1800) sendTimePacket();
					else sendKeepAlivePacket1();
					receivedBytes = _socket->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read from HM-LGW: Too much data.");
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

void HM_LGW::listenKeepAlive()
{
	try
    {
		while(!_initStarted && !_stopCallbackThread)
    	{
    		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    	}

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
							_out.printError("Could not read from HM-LGW: Too much data.");
							break;
						}
					}
				} while(receivedBytes == (unsigned)bufferMax);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty())
				{
					if(_socketKeepAlive->connected()) sendKeepAlivePacket2();
					continue;
				}
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				_out.printWarning("Warning: " + ex.what());
				continue;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				_out.printError("Error: " + ex.what());
				continue;
			}
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		_out.printDebug("Debug: Packet received on port " + _settings->portKeepAlive + ". Raw data:");
        		_out.printBinary(data);
        	}

        	processDataKeepAlive(data);
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

bool HM_LGW::aesKeyExchange(std::vector<uint8_t>& data)
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
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		startPos += 5;
		int32_t length = hex.find('\n', startPos);
		if(length == (signed)std::string::npos)
		{
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		length = length - startPos - 1;
		if(length <= 30)
		{
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		if(data.at(startPos - 4) == 'V' && data.at(startPos - 1) == ',')
		{
			uint8_t packetIndex = (_math.getNumber(data.at(startPos - 3)) << 4) + _math.getNumber(data.at(startPos - 2));
			packetIndex++;
			if(length != 32)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HM-LGW. Received IV has wrong size.");
				return false;
			}
			_remoteIV.clear();
			std::string ivHex(&data.at(startPos), &data.at(startPos) + length);
			_remoteIV = _bl->hf.getUBinary(ivHex);
			if(_remoteIV.size() != 16)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HM-LGW. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5) _out.printDebug("HM-LGW IV is: " + _bl->hf.getHexString(_remoteIV));

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
			_out.printError("Error: Error communicating with HM-LGW. No IV was send from HM-LGW.");
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

bool HM_LGW::aesKeyExchangeKeepAlive(std::vector<uint8_t>& data)
{
	try
	{
		std::string hex(&data.at(0), &data.at(0) + data.size());
		if(_bl->debugLevel >= 5)
		{
			std::string temp = hex;
			_bl->hf.stringReplace(temp, "\r\n", "\\r\\n");
			_out.printDebug(std::string("Debug: AES key exchange packet received on port " + _settings->portKeepAlive + ": " + temp));
		}
		int32_t startPos = hex.find('\n');
		if(startPos == (signed)std::string::npos)
		{
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		startPos += 5;
		int32_t length = hex.find('\n', startPos);
		if(length == (signed)std::string::npos)
		{
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		length = length - startPos - 1;
		if(length <= 30)
		{
			_out.printError("Error: Error communicating with HM-LGW. Initial handshake packet has wrong format.");
			return false;
		}
		if(data.at(startPos - 4) == 'V' && data.at(startPos - 1) == ',')
		{
			_packetIndexKeepAlive = (_math.getNumber(data.at(startPos - 3)) << 4) + _math.getNumber(data.at(startPos - 2));
			_packetIndexKeepAlive++;
			if(length != 32)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HM-LGW. Received IV has wrong size.");
				return false;
			}
			_remoteIVKeepAlive.clear();
			std::string ivHex(&data.at(startPos), &data.at(startPos) + length);
			_remoteIVKeepAlive = _bl->hf.getUBinary(ivHex);
			if(_remoteIVKeepAlive.size() != 16)
			{
				_stopCallbackThread = true;
				_out.printError("Error: Error communicating with HM-LGW. Received IV is not in hexadecimal format.");
				return false;
			}
			if(_bl->debugLevel >= 5) _out.printDebug("HM-LGW IV for keep alive packets is: " + _bl->hf.getHexString(_remoteIVKeepAlive));

			gcry_error_t result;
			if((result = gcry_cipher_setiv(_encryptHandleKeepAlive, &_remoteIVKeepAlive.at(0), _remoteIVKeepAlive.size())) != GPG_ERR_NO_ERROR)
			{
				_stopCallbackThread = true;
				aesCleanup();
				_out.printError("Error: Could not set IV for keep alive encryption: " + _bl->hf.getGCRYPTError(result));
				return false;
			}

			std::vector<char> response = { 'V', _bl->hf.getHexChar(_packetIndexKeepAlive >> 4), _bl->hf.getHexChar(_packetIndexKeepAlive & 0xF), ',' };
			std::random_device rd;
			std::default_random_engine generator(rd());
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

			if(_bl->debugLevel >= 5) _out.printDebug("Homegear IV for keep alive packets is: " + _bl->hf.getHexString(_myIVKeepAlive));

			if((result = gcry_cipher_setiv(_decryptHandleKeepAlive, &_myIVKeepAlive.at(0), _myIVKeepAlive.size())) != GPG_ERR_NO_ERROR)
			{
				_stopCallbackThread = true;
				aesCleanup();
				_out.printError("Error: Could not set IV for keep alive decryption: " + _bl->hf.getGCRYPTError(result));
				return false;
			}

			sendKeepAlive(response, true);
			_aesExchangeKeepAliveComplete = true;
			return true;
		}
		else if(_remoteIVKeepAlive.empty())
		{
			_stopCallbackThread = true;
			_out.printError("Error: Error communicating with HM-LGW. No IV was send from HM-LGW.");
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

void HM_LGW::processPacket(std::vector<uint8_t>& packet)
{
	try
	{
		_out.printDebug(std::string("Debug: Packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet)));
		if(packet.size() < 8) return;
		uint16_t crc = _crc.calculate(packet, true);
		if((packet.at(packet.size() - 2) != (crc >> 8) || packet.at(packet.size() - 1) != (crc & 0xFF)))
		{
			if(_firstPacket)
			{
				_firstPacket = false;
				return;
			}
			_out.printError("Error: CRC failed on packet received from HM-LGW on port " + _settings->port + ": " + _bl->hf.getHexString(packet));
			_stopped = true;
			return;
		}
		else
		{
			_firstPacket = false;
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
					_out.printError("Error: Something is wrong with your HM-LGW. You probably need to replace it. Check if it works with a CCU.");
					request->response = packet;
					request->mutex.unlock();
					return;
				}
			}
			else _requestsMutex.unlock();
			if(_initComplete) parsePacket(packet);
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

void HM_LGW::processData(std::vector<uint8_t>& data)
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
		if(decryptedData.size() < 8) //8 is minimum size fd
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
			if(_bl->debugLevel >= 5) _out.printDebug(std::string("Debug: Packet received on port " + _settings->portKeepAlive + ": " + packet));
			if(_initCompleteKeepAlive) parsePacketKeepAlive(packet); else processInitKeepAlive(packet);
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

void HM_LGW::processInitKeepAlive(std::string& packet)
{
	try
	{
		if(packet.empty()) return;
		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(packet, ',');
		if(parts.size() != 2 || parts.at(0).size() != 3 || parts.at(0).at(0) != 'S' || parts.at(1).size() < 6 || parts.at(1).compare(0, 6, "SysCom") != 0)
		{
			_stopCallbackThread = true;
			_out.printError("Error: First packet does not start with \"S\" or has wrong structure. Please check your AES key in physicalinterfaces.conf. Stopping listening.");
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

void HM_LGW::parsePacket(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.empty()) return;
		if(packet.at(5) == 4 && packet.at(3) == 0 && packet.size() == 10 && packet.at(6) == 2 && packet.at(7) == 0)
		{
			if(_bl->debugLevel >= 5) _out.printDebug("Debug: Keep alive response received on port " + _settings->port + ".");
			_lastKeepAliveResponse1 = BaseLib::HelperFunctions::getTimeSeconds();
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
			std::shared_ptr<BidCoSPacket> bidCoSPacket(new BidCoSPacket(binaryPacket, true, BaseLib::HelperFunctions::getTime()));
			//Don't use (packet.at(6) & 1) here. That bit is set for non-AES packets, too
			//packet.at(6) == 3 and packet.at(7) == 0 is set on pairing packets: FD0020018A0503002494840026219BFD00011000AD4C4551303030333835365803FFFFCB99
			if(packet.at(5) == 5 && ((packet.at(6) & 3) == 3 || (packet.at(6) & 5) == 5))
			{
				//Accept pairing packets from HM-TC-IT-WM-W-EU (version 1.0) and maybe other devices.
				//For these devices the handshake is never executed, but the "failed bit" set anyway: Bug
				if(!(bidCoSPacket->controlByte() & 0x4) || bidCoSPacket->messageType() != 0 || bidCoSPacket->payload()->size() != 17)
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
			bool wakeUp = packet.at(5) == 5 && (packet.at(6) & 0x10);
			raisePacketReceived(bidCoSPacket);
			if(wakeUp) //Wake up was sent
			{
				std::vector<uint8_t> payload;
				payload.push_back(0x00);
				std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(bidCoSPacket->messageCounter(), 0x80, 0x02, bidCoSPacket->senderAddress(), _myAddress, payload));
				ok->setTimeReceived(bidCoSPacket->timeReceived() + 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				raisePacketReceived(ok);
			}
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

void HM_LGW::parsePacketKeepAlive(std::string& packet)
{
	try
	{
		if(packet.empty()) return;
		if(packet.at(0) == '>'  && (packet.at(1) == 'K' || packet.at(1) == 'L') && packet.size() == 5)
		{
			if(_bl->debugLevel >= 5) _out.printDebug("Debug: Keep alive response received on port " + _settings->portKeepAlive + ".");
			std::string index = packet.substr(2, 2);
			if(BaseLib::Math::getNumber(index, true) == _packetIndexKeepAlive)
			{
				_lastKeepAliveResponse2 = BaseLib::HelperFunctions::getTimeSeconds();
				_packetIndexKeepAlive++;
			}
			if(packet.at(1) == 'L') sendKeepAlivePacket2();
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
