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

#include "CUNO.h"
#include "../BidCoSPacket.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{

CUNO::CUNO(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IBidCoSInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "CUNO \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 45;
		settings->listenThreadPolicy = SCHED_FIFO;
	}
}

CUNO::~CUNO()
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

void CUNO::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}

		if(packet->payload()->size() > 54)
		{
			if(_bl->debugLevel >= 2) _out.printError("Error: Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}
		if(_updateMode)
		{
			std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
			if(!bidCoSPacket) return;
			if(!bidCoSPacket->isUpdatePacket())
			{
				_out.printInfo("Info: Can't send packet to BidCoS peer with address 0x" + BaseLib::HelperFunctions::getHexString(packet->destinationAddress(), 6) + ", because update mode is enabled.");
				return;
			}
		}

		std::string packetString = packet->hexString();
		if(_bl->debugLevel >= 4) _out.printInfo("Info: Sending (" + _settings->id + "): " + packetString);
		send("As" + packet->hexString() + "\n");
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

void CUNO::enableUpdateMode()
{
	try
	{
		_updateMode = true;
		send("AR\n");
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

void CUNO::disableUpdateMode()
{
	try
	{
		send("Ar\n");
		_updateMode = false;
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

void CUNO::send(std::string data)
{
	try
    {
    	if(data.size() < 3) return; //Otherwise error in printInfo
    	_sendMutex.lock();
    	if(!_socket->connected() || _stopped)
    	{
    		_out.printWarning(std::string("Warning: !!!Not!!! sending: ") + data.substr(2, data.size() - 3));
    		_sendMutex.unlock();
    		return;
    	}
    	_socket->proofwrite(data);
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

void CUNO::startListening()
{
	try
	{
		stopListening();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_socket->setAutoConnect(false);
		_out.printDebug("Connecting to CUNO with hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		_listenThread = std::thread(&CUNO::listen, this);
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

void CUNO::reconnect()
{
	try
	{
		_socket->close();
		_out.printDebug("Connecting to CUNO device with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		send("X21\nAr\n");
		_out.printInfo("Connected to CUNO device with hostname " + _settings->host + " on port " + _settings->port + ".");
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

void CUNO::stopListening()
{
	try
	{
		if(_socket->connected()) send("Ax\nX00\n");
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		_socket->close();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
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

void CUNO::listen()
{
    try
    {
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);

        while(!_stopCallbackThread)
        {
        	if(_stopped || !_socket->connected())
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		if(_stopped) _out.printWarning("Warning: Connection to CUNO closed. Trying to reconnect...");
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
							_out.printError("Could not read from CUNO: Too much data.");
							break;
						}
					}
				} while(receivedBytes == (unsigned)bufferMax);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty()) continue; //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
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
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		_out.printDebug("Debug: Packet received from CUNO. Raw data:");
        		_out.printBinary(data);
        	}

        	processData(data);

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

void CUNO::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		std::string packets;
		packets.insert(packets.end(), data.begin(), data.end());

		std::istringstream stringStream(packets);
		std::string packetHex;
		while(std::getline(stringStream, packetHex))
		{
			if(packetHex.size() > 21) //21 is minimal packet length (=10 Byte + CUNO "A")
        	{
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(packetHex, BaseLib::HelperFunctions::getTime()));
				raisePacketReceived(packet);
        	}
        	else if(!packetHex.empty())
        	{
        		if(packetHex == "LOVF") _out.printWarning("Warning: CUNO with id " + _settings->id + " reached 1% limit. You need to wait, before sending is allowed again.");
        		else _out.printWarning("Warning: Too short packet received: " + packetHex);
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
}
