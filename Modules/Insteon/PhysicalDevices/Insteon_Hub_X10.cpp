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

#include "Insteon_Hub_X10.h"
#include "../GD.h"

namespace Insteon
{

InsteonHubX10::InsteonHubX10(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : BaseLib::Systems::IPhysicalInterface(GD::bl, settings)
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

void InsteonHubX10::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();

		std::shared_ptr<InsteonPacket> insteonPacket(std::dynamic_pointer_cast<InsteonPacket>(packet));
		if(!insteonPacket) return;

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
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
		if(_stopped) return;
		_requestMutex.lock();
		_request.reset(new Request(responseType));
		_request->mutex.try_lock(); //Lock and return immediately
		send(packet, false);
		if(!_request->mutex.try_lock_for(std::chrono::milliseconds(10000)))
		{
			_out.printError("Error: No response received to packet: " + _bl->hf.getHexString(packet));
		}
		_request->mutex.unlock();
		response = _request->response;
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
    	int32_t written = _socket->proofwrite(data);
    }
    catch(BaseLib::SocketOperationException& ex)
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
		int32_t i = 0;
		while(!_stopCallbackThread && !GD::family->getCentral() && i < 30)
		{
			_out.printDebug("Debug: Waiting for central to load.");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			i++;
		}
		if(_stopCallbackThread) return;
		if(!GD::family->getCentral())
		{
			_stopCallbackThread = true;
			_out.printError("Error: Could not get central address. Stopping listening.");
			return;
		}

		if(_stopped) return;

		_initStarted = true;
		/*
		=> 0260
		<= 0260 1eb784 032e9c
		=> 026b48
		<= 026b4806
		*/

		std::vector<char> requestPacket { 0x02, 0x60 };
		std::vector<uint8_t> responsePacket;
		for(int32_t i = 0; i < 20; i++)
		{
			getResponse(requestPacket, responsePacket, 0x60);
			if(responsePacket.size() == 9)
			{
				_myAddress = (responsePacket.at(2) << 16) + (responsePacket.at(3) << 8) + responsePacket.at(4);
				_out.printInfo("Info: Received device type: 0x" + BaseLib::HelperFunctions::getHexString((responsePacket.at(5) << 8) + responsePacket.at(6), 4) + " Firmware version is: 0x" + BaseLib::HelperFunctions::getHexString(responsePacket.at(7), 2));
				break;
			}
			if(i == 19)
			{
				_out.printError("Error: No or wrong response to first init packet. Reconnecting...");
				_stopped = true;
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		requestPacket = std::vector<char> { 0x02, 0x6B, 0x48 };
		responsePacket.clear();
		for(int32_t i = 0; i < 20; i++)
		{
			getResponse(requestPacket, responsePacket, 0x6B);
			if(responsePacket.size() == 4) break;
			if(i == 19)
			{
				_out.printError("Error: No or wrong response to first init packet. Reconnecting...");
				_stopped = true;
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		_out.printInfo("Info: Init queue completed.");
		_initComplete = true;
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
		_out.printDebug("Connecting to Insteon Hub X10 with Hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_out.printInfo("Connected to Insteon Hub X10 with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&InsteonHubX10::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(GD::bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		_socket->close();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
		_initStarted = false;
		_initComplete = false;
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
				_out.printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
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

void InsteonHubX10::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;

		for(int32_t i = 0; i < data.size();)
		{
			if(data.at(i) == 0x15)
			{
				std::vector<uint8_t> packetBytes(&data.at(i), &data.at(i) + 1);
				processPacket(packetBytes);
				if(i + 1 <= data.size() && data.at(i + 1) == 0x15) i += 2;
				else i += 1;
				continue;
			}
			if(_lengthLookup.find(data.at(i + 1)) == _lengthLookup.end())
			{
				_out.printError("Error: Unknown packet received from Insteon Hub. Discarding whole buffer. Buffer is: " + BaseLib::HelperFunctions::getHexString(data));
				return;
			}
			uint8_t type = data.at(i + 1);
			int32_t length = _lengthLookup[type];
			//0x62 can be 9 or 23 bytes long
			if(type == 0x62 && i + 5 <= data.size() && (data.at(i + 5) & 16) == 0) length = 9;
			if(i + length > data.size())
			{
				_out.printError("Error: Length (" + std::to_string(length) + ") is larger than buffer. Buffer is: " + BaseLib::HelperFunctions::getHexString(data));
				return;
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
			_request->mutex.unlock();
			return;
		}

		if(data.size() < 11) return;
		if(data.at(1) == 0x50 || data.at(1) == 0x51)
		{
			std::vector<uint8_t> binaryPacket(&data.at(2), &data.at(0) + data.size());
			std::shared_ptr<InsteonPacket> insteonPacket(new InsteonPacket(binaryPacket, BaseLib::HelperFunctions::getTime()));
			raisePacketReceived(insteonPacket, false);
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
