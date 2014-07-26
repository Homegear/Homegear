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

#include "RTLSDR-LAN.h"
#include "../GD.h"

namespace BidCoS
{

RTLSDR_LAN::RTLSDR_LAN(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IBidCoSInterface(settings)
{
	signal(SIGPIPE, SIG_IGN);
	_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl));
}

RTLSDR_LAN::~RTLSDR_LAN()
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

void RTLSDR_LAN::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	//Dummy function
}

void RTLSDR_LAN::startListening()
{
	try
	{
		stopListening();
		_socket = std::unique_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		GD::out.printDebug("Connecting to RTLSDR-LAN device with Hostname " + _settings->host + " on port " + _settings->port + "...");
		//_socket.open();
		//GD::out.printInfo("Connected to RTLSDR-LAN device with Hostname " + _settings->host + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&RTLSDR_LAN::listen, this);
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

void RTLSDR_LAN::stopListening()
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

void RTLSDR_LAN::listen()
{
    try
    {
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 1024;
		std::vector<char> buffer(bufferMax);
		std::vector<uint8_t> data;
        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
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
							data.clear();
							GD::out.printError("Could not read from RTLSDR-LAN: Too much data.");
							break;
						}
					}
				} while(receivedBytes == bufferMax);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
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
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		GD::out.printDebug("Debug: Packet received from HM-CFG-LAN. Raw data:");
        		GD::out.printBinary(data);
        	}

        	std::shared_ptr<BidCoSPacket> bidCoSPacket(new BidCoSPacket(data, true, BaseLib::HelperFunctions::getTime()));
			raisePacketReceived(bidCoSPacket);
			_lastPacketReceived = BaseLib::HelperFunctions::getTime();
			data.clear();
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
