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

#include "RawLAN.h"
#include "../GD.h"

namespace HMWired
{

RawLAN::RawLAN(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : BaseLib::Systems::IPhysicalInterface(GD::bl, settings)
{
	signal(SIGPIPE, SIG_IGN);
	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 0;
		settings->listenThreadPolicy = SCHED_OTHER;
	}
}

RawLAN::~RawLAN()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
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

void RawLAN::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			GD::out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();
		if(packet->payload()->size() > 132)
		{
			if(_bl->debugLevel >= 2) GD::out.printError("Tried to send packet with payload larger than 128 bytes. That is not supported.");
			return;
		}

		std::shared_ptr<HMWiredPacket> hmWiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmWiredPacket) return;
		std::vector<char> data = hmWiredPacket->byteArraySigned();
		send(data, true);
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

void RawLAN::send(std::vector<char>& packet, bool printPacket)
{
    try
    {
    	_sendMutex.lock();
    	_lastPacketSent = BaseLib::HelperFunctions::getTime(); //Sending takes some time, so we set _lastPacketSent two times
    	if(_bl->debugLevel > 3 && printPacket) GD::out.printInfo("Info: Sending: " + BaseLib::HelperFunctions::getHexString(packet));
    	int32_t written = _socket->proofwrite(packet);
    	_lastPacketSent = BaseLib::HelperFunctions::getTime();
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
    _sendMutex.unlock();
}

void RawLAN::startListening()
{
	try
	{
		stopListening();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		GD::out.printDebug("Connecting to raw RS485 LAN device with Hostname " + _settings->host + " on port " + _settings->port + "...");
		//_socket->open();
		//GD::out.printInfo("Connected to raw RS485 LAN device with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&RawLAN::listen, this);
		BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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

void RawLAN::stopListening()
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

void RawLAN::listen()
{
    try
    {
    	uint32_t receivedBytes;
    	int32_t bufferMax = 1024;
		std::vector<char> buffer(bufferMax);
		std::vector<uint8_t> data;
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
				receivedBytes = _socket->proofread(&buffer[0], bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				if(!data.empty())
				{
					if(data.size() == 1 && data.at(0) != 0xF8)
					{
						GD::out.printDebug("Debug: Correcting wrong response to 0xF8: " + BaseLib::HelperFunctions::getHexString(data));
						data.at(0) = 0xF8;
					}
					std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(data, BaseLib::HelperFunctions::getTime(), true));
					raisePacketReceived(packet);
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
					data.clear();
					_socket->setReadTimeout(5000000);
				}
				continue;
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				GD::out.printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				GD::out.printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
        	if(receivedBytes == 0) continue;
        	if(receivedBytes == bufferMax)
        	{
        		GD::out.printError("Could not read from raw RS485 LAN device: Too much data.");
        		continue;
        	}

        	if(!data.empty() && (buffer.at(0) == 0xFD || buffer.at(0) == 0xFE || buffer.at(0) == 0xF8))
        	{
        		if(data.size() == 1 && data.at(0) != 0xF8)
				{
					GD::out.printDebug("Debug: Correcting wrong response to 0xF8: " + BaseLib::HelperFunctions::getHexString(data));
					data.at(0) = 0xF8;
				}
        		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(data, BaseLib::HelperFunctions::getTime(), true));
        		raisePacketReceived(packet);
        		_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        		data.clear();
        	}

        	_socket->setReadTimeout(10000);
        	data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
        	if(_bl->debugLevel >= 6)
        	{
        		GD::out.printDebug("Debug: Packet received from RS485 raw LAN device. Data:");
        		GD::out.printBinary(data);
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

}
