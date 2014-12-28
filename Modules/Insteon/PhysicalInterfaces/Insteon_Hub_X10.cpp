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

#include "Insteon_Hub_X10.h"
#include "../GD.h"

namespace Insteon
{

InsteonHubX10::InsteonHubX10(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IInsteonInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "Insteon Hub X10 \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);
	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	_lengthLookup[0x50] = 11;
	_lengthLookup[0x51] = 25;
	_lengthLookup[0x52] = 4;
	_lengthLookup[0x53] = 10;
	_lengthLookup[0x54] = 3;
	_lengthLookup[0x55] = 2;
	_lengthLookup[0x56] = 13;
	_lengthLookup[0x57] = 10;
	_lengthLookup[0x58] = 3;
	_lengthLookup[0x59] = 12;
	_lengthLookup[0x60] = 9;
	_lengthLookup[0x61] = 6;
	_lengthLookup[0x62] = 23;
	_lengthLookup[0x63] = 5;
	_lengthLookup[0x64] = 5;
	_lengthLookup[0x65] = 3;
	_lengthLookup[0x66] = 6;
	_lengthLookup[0x67] = 3;
	_lengthLookup[0x68] = 4;
	_lengthLookup[0x69] = 3;
	_lengthLookup[0x6A] = 3;
	_lengthLookup[0x6B] = 4;
	_lengthLookup[0x6C] = 3;
	_lengthLookup[0x6D] = 3;
	_lengthLookup[0x6E] = 3;
	_lengthLookup[0x6F] = 12;
	_lengthLookup[0x70] = 4;
	_lengthLookup[0x71] = 5;
	_lengthLookup[0x72] = 3;
	_lengthLookup[0x73] = 6;
	_lengthLookup[0x75] = 5;
	_lengthLookup[0x76] = 13;
}

InsteonHubX10::~InsteonHubX10()
{
	try
	{
		_stopCallbackThread = true;
		if(_initThread.joinable()) _initThread.join();
		if(_listenThread.joinable()) _listenThread.join();
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

void InsteonHubX10::enablePairingMode()
{
	try
	{
		std::vector<char> requestPacket { 0x02, 0x64, 0x03, 0x00 };
		std::vector<uint8_t> responsePacket;
		getResponse(requestPacket, responsePacket, 0x64);
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

void InsteonHubX10::disablePairingMode()
{
	try
	{
		std::vector<char> requestPacket { 0x02, 0x65 };
		std::vector<uint8_t> responsePacket;
		getResponse(requestPacket, responsePacket, 0x65);
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

void InsteonHubX10::addPeer(int32_t address)
{
	try
	{
		if(address == 0) return;
		_peersMutex.lock();
		if(_pairedPeers.find(address) == _pairedPeers.end()) _pairedPeers.insert(address);
		if(_peers.find(address) != _peers.end())
		{
			_peersMutex.unlock();
			return;
		}
		if(_initComplete)
		{
			PeerInfo* info = &_peers[address];
			info->address = address;
			info->databaseAddressController = getFreeDatabaseAddress();
			_usedDatabaseAddresses.insert(info->databaseAddressController);
			info->databaseAddressResponder = getFreeDatabaseAddress();
			_usedDatabaseAddresses.insert(info->databaseAddressResponder);

			info->flagsController = 0xE2;  //Bit 7 => Record in use, Bit 6 => Controller, Bit 5 => ACK required, Bit 4, 3, 2 => reserved, Bit 1 => Record has been used before, Bit 0 => unused
			info->data1Controller = 0;
			info->data2Controller = 0;
			info->data3Controller = 0;

			info->flagsResponder = 0xA2;
			info->data1Responder = 1;
			info->data2Responder = 0;
			info->data3Responder = 0;

			storePeer(*info);
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

void InsteonHubX10::addPeers(std::vector<int32_t>& addresses)
{
	try
	{
		for(std::vector<int32_t>::iterator i = addresses.begin(); i != addresses.end(); ++i)
		{
			addPeer(*i);
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

void InsteonHubX10::removePeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_pairedPeers.find(address) != _pairedPeers.end()) _pairedPeers.erase(address);
		if(_peers.find(address) == _peers.end())
		{
			_peersMutex.unlock();
			return;
		}
		PeerInfo* info = &_peers.at(address);
		info->flagsController &= 0x7F;
		info->flagsResponder &= 0x7F;
		storePeer(*info);
		_usedDatabaseAddresses.erase(info->databaseAddressController);
		_usedDatabaseAddresses.erase(info->databaseAddressResponder);
		_peers.erase(address);
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

void InsteonHubX10::checkPeers()
{
	try
	{
		_peersMutex.lock();
		for(std::set<int32_t>::iterator i = _pairedPeers.begin(); i != _pairedPeers.end(); ++i)
		{
			if(_peers.find(*i) != _peers.end()) continue;
			if(_initComplete)
			{
				PeerInfo* info = &_peers[*i];
				info->address = *i;
				info->databaseAddressController = getFreeDatabaseAddress();
				_usedDatabaseAddresses.insert(info->databaseAddressController);
				info->databaseAddressResponder = getFreeDatabaseAddress();
				_usedDatabaseAddresses.insert(info->databaseAddressResponder);

				info->flagsController = 0xE2;  //Bit 7 => Record in use, Bit 6 => Controller, Bit 5 => ACK required, Bit 4, 3, 2 => reserved, Bit 1 => Record has been used before, Bit 0 => unused
				info->data1Controller = 0;
				info->data2Controller = 0;
				info->data3Controller = 0;

				info->flagsResponder = 0xA2;
				info->data1Responder = 1;
				info->data2Responder = 0;
				info->data3Responder = 0;

				storePeer(*info);
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

void InsteonHubX10::storePeer(PeerInfo& peerInfo)
{
	try
	{
		std::vector<char> requestPacket { 0x02, 0x76 };
		std::vector<uint8_t> responsePacket;
		requestPacket.push_back(peerInfo.databaseAddressController >> 8);
		requestPacket.push_back(peerInfo.databaseAddressController & 0xFF);
		requestPacket.push_back(peerInfo.flagsController);
		requestPacket.push_back(0); //Group
		requestPacket.push_back(peerInfo.address >> 16);
		requestPacket.push_back((peerInfo.address >> 8) & 0xFF);
		requestPacket.push_back(peerInfo.address & 0xFF);
		requestPacket.push_back(peerInfo.data1Controller);
		requestPacket.push_back(peerInfo.data2Controller);
		requestPacket.push_back(peerInfo.data3Controller);
		for(int32_t j = 0; j < 10; j++)
		{
			responsePacket.clear();
			getResponse(requestPacket, responsePacket, 0x76);
			if(_stopped) return;
			if(responsePacket.size() == 13) break;
			if(j == 9)
			{
				_out.printError("Error: Unknown response to store peer packet. Reconnecting...");
				_stopped = true;
				return;
			}
		}

		requestPacket.clear();
		requestPacket.push_back(0x02);
		requestPacket.push_back(0x76);
		requestPacket.push_back(peerInfo.databaseAddressResponder >> 8);
		requestPacket.push_back(peerInfo.databaseAddressResponder & 0xFF);
		requestPacket.push_back(peerInfo.flagsResponder);
		requestPacket.push_back(1); //Group
		requestPacket.push_back(peerInfo.address >> 16);
		requestPacket.push_back((peerInfo.address >> 8) & 0xFF);
		requestPacket.push_back(peerInfo.address & 0xFF);
		requestPacket.push_back(peerInfo.data1Responder);
		requestPacket.push_back(peerInfo.data2Responder);
		requestPacket.push_back(peerInfo.data3Responder);
		for(int32_t j = 0; j < 10; j++)
		{
			responsePacket.clear();
			getResponse(requestPacket, responsePacket, 0x76);
			if(_stopped) return;
			if(responsePacket.size() == 13) break;
			if(j == 9)
			{
				_out.printError("Error: Unknown response to store peer packet. Reconnecting...");
				_stopped = true;
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

int32_t InsteonHubX10::getFreeDatabaseAddress()
{
	//Only call this method, when _peersMutex is locked
	for(int32_t dbAddress = 0x1FF8; dbAddress > 0; dbAddress -= 8)
	{
		if(_usedDatabaseAddresses.find(dbAddress) == _usedDatabaseAddresses.end()) return dbAddress;
	}
	return -1;
}

void InsteonHubX10::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}

		if(!_initComplete)
    	{
    		_out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->port + "), because the init sequence is not completed: " + packet->hexString());
    		_sendMutex.unlock();
    		return;
    	}

		_lastAction = BaseLib::HelperFunctions::getTime();

		std::shared_ptr<InsteonPacket> insteonPacket(std::dynamic_pointer_cast<InsteonPacket>(packet));
		if(!insteonPacket) return;

		//Don't move this, because packet->hexString() also calculates the checksum!
		_out.printInfo("Info: Sending (" + _settings->id + "): " + packet->hexString());

		std::vector<char> requestPacket { 0x02, 0x62 };
		requestPacket.push_back(insteonPacket->destinationAddress() >> 16);
		requestPacket.push_back((insteonPacket->destinationAddress() >> 8) & 0xFF);
		requestPacket.push_back(insteonPacket->destinationAddress() & 0xFF);
		requestPacket.push_back(((uint8_t)insteonPacket->flags() << 5) + ((uint8_t)insteonPacket->extended() << 4) + (insteonPacket->hopsLeft() << 2) + insteonPacket->hopsMax());
		requestPacket.push_back(insteonPacket->messageType());
		requestPacket.push_back(insteonPacket->messageSubtype());
		requestPacket.insert(requestPacket.end(), insteonPacket->payload()->begin(), insteonPacket->payload()->end());

		std::vector<uint8_t> responsePacket;
		for(int32_t i = 0; i < 20; i++)
		{
			getResponse(requestPacket, responsePacket, 0x62);
			if(responsePacket.size() > 1) break;
			if(i == 19)
			{
				_out.printError("Error: No or wrong response to \"send packet\" request.");
				_stopped = true;
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(240));
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

void InsteonHubX10::getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t responseType)
{
	try
    {
		_requestMutex.lock();
		for(int32_t i = 0; i < 50; i++)
		{
			if(_stopped || _stopCallbackThread) break;
			_request.reset(new Request(responseType));
			std::unique_lock<std::mutex> lock(_request->mutex);
			send(packet, false);
			if(!_request->conditionVariable.wait_for(lock, std::chrono::milliseconds(10000), [&] { return _request->mutexReady; }))
			{
				_out.printError("Error: No response received to packet: " + _bl->hf.getHexString(packet));
			}
			response = _request->response;
			lock.unlock();
			if(response.size() > 1 && response.at(0) != 0x15) break;
			if((response.size() < 1 || response.at(0) != 0x15) && i == 3)
			{
				_out.printError("Error: No or wrong response to packet. Reconnecting...");
				_stopped = true;
				break;
			}
			else if(i == 49)
			{
				_out.printError("Error: Nak received 50 times. Reconnecting...");
				_stopped = true;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(240));
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
    _request.reset();
    _requestMutex.unlock();
}

void InsteonHubX10::send(const std::vector<char>& data, bool printPacket)
{
    try
    {
    	_sendMutex.lock();
    	if(!_socket->connected() || _stopped)
    	{
    		_out.printWarning("Warning: !!!Not!!! sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
    		_sendMutex.unlock();
    		return;
    	}
    	if(_bl->debugLevel >= 5) _out.printDebug("Debug: Sending (Port " + _settings->port + "): " + _bl->hf.getHexString(data));
    	_socket->proofwrite(data);
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
    _sendMutex.unlock();
}

void InsteonHubX10::doInit()
{
	try
	{
		if(!GD::family->getCentral())
		{
			_stopCallbackThread = true;
			_stopped = true;
			_out.printError("Error: Could not get central address. Stopping listening.");
			return;
		}

		if(_stopped) return;

		_centralAddress = GD::family->getCentral()->physicalAddress();

		while(!_stopCallbackThread && !_stopped && !_socket->connected())
		{
			try
			{
				_socket->open();
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

		_initStarted = true;
		/*
		=> 0260
		<= 0260 1eb784 032e9c
		=> 026b48
		<= 026b4806
		*/

		std::vector<char> requestPacket { 0x02, 0x60 };
		std::vector<uint8_t> responsePacket;
		getResponse(requestPacket, responsePacket, 0x60);
		if(responsePacket.size() == 9)
		{
			_myAddress = (responsePacket.at(2) << 16) + (responsePacket.at(3) << 8) + responsePacket.at(4);
			_out.printInfo("Info: Received device type: 0x" + BaseLib::HelperFunctions::getHexString((responsePacket.at(5) << 8) + responsePacket.at(6), 4) + " Firmware version is: 0x" + BaseLib::HelperFunctions::getHexString(responsePacket.at(7), 2));
		}
		else
		{
			_out.printError("Error: Unknown response received to first init packet. Reconnecting...");
			_stopped = true;
			return;
		}

		requestPacket = std::vector<char> { 0x02, 0x6B, 0x48 };
		responsePacket.clear();
		getResponse(requestPacket, responsePacket, 0x6B);
		if(responsePacket.size() != 4)
		{
			_out.printError("Error: Unknown response received to second init packet. Reconnecting...");
			_stopped = true;
			return;
		}

		//Get all peers from hub database
		for(int32_t dbAddress = 0x1ff8; dbAddress > 0; dbAddress -= 8)
		{
			requestPacket.clear();
			requestPacket.push_back(0x02);
			requestPacket.push_back(0x75);
			requestPacket.push_back(dbAddress >> 8);
			requestPacket.push_back(dbAddress & 0xFF);
			for(int32_t j = 0; j < 10; j++)
			{
				responsePacket.clear();
				getResponse(requestPacket, responsePacket, 0x59);
				if(_stopped) return;
				if(responsePacket.size() == 12) break;
				if(j == 9)
				{
					_out.printError("Error: Unknown response to third init packet. Reconnecting...");
					_stopped = true;
					return;
				}
			}

			if(responsePacket.at(4) == 0) break; //No more entries
			if((responsePacket.at(4) & 0x80) == 0) continue; //Record is unset
			_peersMutex.lock();
			int32_t peerAddress = (responsePacket.at(6) << 16) + (responsePacket.at(7) << 8) + responsePacket.at(8);
			PeerInfo* info = &_peers[peerAddress];
			info->address = peerAddress;
			_usedDatabaseAddresses.insert(dbAddress);
			if(responsePacket.at(4) & 0x40) //Controller?
			{
				info->databaseAddressController = dbAddress;
				info->flagsController = responsePacket.at(4);
				info->data1Controller = responsePacket.at(9);
				info->data2Controller = responsePacket.at(10);
				info->data3Controller = responsePacket.at(11);
				GD::out.printDebug("Debug: Controller entry found in hub's database for peer 0x" + BaseLib::HelperFunctions::getHexString(peerAddress, 6));
			}
			else
			{
				info->databaseAddressResponder = dbAddress;
				info->flagsResponder = responsePacket.at(4);
				info->data1Responder = responsePacket.at(9);
				info->data2Responder = responsePacket.at(10);
				info->data3Responder = responsePacket.at(11);
				GD::out.printDebug("Debug: Responder entry found in hub's database for peer 0x" + BaseLib::HelperFunctions::getHexString(peerAddress, 6));
			}
			_peersMutex.unlock();
		}

		_out.printInfo("Info: Synchronizing peer database.");

		checkPeers();

		_initComplete = true;
		_out.printInfo("Info: Init queue completed.");
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

void InsteonHubX10::startListening()
{
	try
	{
		stopListening();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl, _settings->host, _settings->port));
		_socket->setReadTimeout(1000000);
		_out.printDebug("Connecting to Insteon Hub X10 with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		_listenThread = std::thread(&InsteonHubX10::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(GD::bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		_initThread = std::thread(&InsteonHubX10::doInit, this);
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

void InsteonHubX10::reconnect()
{
	try
	{
		_socket->close();
		if(_initThread.joinable()) _initThread.join();
		_initStarted = false;
		_initComplete = false;
		_out.printDebug("Connecting to Insteon Hub with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_out.printInfo("Connected to Insteon Hub with hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_initThread = std::thread(&InsteonHubX10::doInit, this);
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

void InsteonHubX10::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		if(_initThread.joinable()) _initThread.join();
		if(_listenThread.joinable()) _listenThread.join();
		_stopped = true;
		_stopCallbackThread = false;
		_socket->close();
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
		_initStarted = false;
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

void InsteonHubX10::listen()
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
        			receivedBytes = _socket->proofread(&buffer[0], bufferMax);
        			if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read from Insteon Hub: Too much data.");
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
        	if(data.size() < 3 && data.at(0) == 0x02)
			{
				//The rest of the packet comes with the next read.
				continue;
			}
			if(data.empty()) continue;
			if(data.size() > 1000000)
			{
				data.clear();
				continue;
			}

			if(_bl->debugLevel >= 6) _out.printDebug("Debug: Packet received on port " + _settings->port + ". Raw data: " + BaseLib::HelperFunctions::getHexString(data));

			if(processData(data)) data.clear();

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

bool InsteonHubX10::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return true;

		for(int32_t i = 0; i < (signed)data.size();)
		{
			if(data.at(i) == 0x15)
			{
				std::vector<uint8_t> packetBytes(&data.at(i), &data.at(i) + 1);
				processPacket(packetBytes);
				if(i + 1 < (signed)data.size() && data.at(i + 1) == 0x15) i += 2;
				else i += 1;
				continue;
			}
			if(i + 1 >= (signed)data.size())
			{
				if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Too small packet received: " + BaseLib::HelperFunctions::getHexString(data));
				std::vector<uint8_t> data2(&data.at(i), &data.at(0) + data.size());
				data = data2;
				return false;
			}
			uint8_t type = data.at(i + 1);
			int32_t length = 1; //1 to make 100% sure that i is incremented
			if(_lengthLookup.find(type) == _lengthLookup.end())
			{
				//Sometimes "0x02" is missing
				if(_lengthLookup.find(data.at(i)) != _lengthLookup.end())
				{
					type = data.at(i);
					length = _lengthLookup[type];
					//0x62 can be 9 or 23 bytes long
					if(type == 0x62 && i + 5 <= (signed)data.size() && (data.at(i + 5) & 16) == 0) length = 9;
					if(i + length - 1 > (signed)data.size() || (i + length - 1 < (signed)data.size() && data.at(i + length - 1) != 0x02))
					{
						_out.printError("Error: Unknown packet received from Insteon Hub. Discarding whole buffer. Buffer is: " + BaseLib::HelperFunctions::getHexString(data));
						return true;
					}
					else
					{
						_out.printInfo("Info: Fixing data buffer, because first byte is missing.");
						//Fix data
						std::vector<uint8_t> data2 {0x02};
						data2.insert(data2.end(), data.begin(), data.end());
						data = data2;
					}
				}
				else
				{
					_out.printError("Error: Unknown packet received from Insteon Hub. Discarding whole buffer. Buffer is: " + BaseLib::HelperFunctions::getHexString(data));
					return true;
				}
			}
			else
			{
				length = _lengthLookup[type];
				//0x62 can be 9 or 23 bytes long
				if(type == 0x62 && i + 5 <= (signed)data.size() && (data.at(i + 5) & 16) == 0) length = 9;
			}

			if(i + length > (signed)data.size())
			{
				_out.printDebug("Debug: Length (" + std::to_string(length) + ") is larger than buffer. Waiting for next receive. Buffer is: " + BaseLib::HelperFunctions::getHexString(data));
				std::vector<uint8_t> data2(&data.at(i), &data.at(0) + data.size());
				data = data2;
				return false;
			}
			std::vector<uint8_t> packetBytes(&data.at(i), &data.at(i) + length);
			processPacket(packetBytes);
			i += length;
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
    return true;
}

void InsteonHubX10::processPacket(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;

		if(_bl->debugLevel >= 5) _out.printDebug(std::string("Debug: Packet received on port " + _settings->port + ": " + BaseLib::HelperFunctions::getHexString(data)));

		if(_request && (data.size() == 1 || _request->getResponseType() == data.at(1)))
		{
			_request->response = data;
			{
				std::lock_guard<std::mutex> lock(_request->mutex);
				_request->mutexReady = true;
			}
			_request->conditionVariable.notify_one();
			return;
		}

		if(!_initComplete) return;
		if(data.size() < 11) return;
		if(data.at(1) == 0x50 || data.at(1) == 0x51)
		{
			std::vector<uint8_t> binaryPacket(&data.at(2), &data.at(0) + data.size());
			std::shared_ptr<InsteonPacket> insteonPacket(new InsteonPacket(binaryPacket, _settings->id, BaseLib::HelperFunctions::getTime()));
			if(insteonPacket->destinationAddress() == _myAddress) insteonPacket->setDestinationAddress(_centralAddress);
			raisePacketReceived(insteonPacket);
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
