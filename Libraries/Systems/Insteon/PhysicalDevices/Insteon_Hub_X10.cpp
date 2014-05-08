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
#include "../../../GD/GD.h"
#include "../InsteonPacket.h"

namespace PhysicalDevices
{

InsteonHubX10::InsteonHubX10(std::shared_ptr<PhysicalDeviceSettings> settings) : PhysicalDevice(settings)
{
	signal(SIGPIPE, SIG_IGN);
}

InsteonHubX10::~InsteonHubX10()
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonHubX10::sendPacket(std::shared_ptr<Packet> packet)
{
	try
	{
		if(!packet)
		{
			Output::printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = HelperFunctions::getTime();

		std::shared_ptr<Insteon::InsteonPacket> insteonPacket(std::dynamic_pointer_cast<Insteon::InsteonPacket>(packet));
		if(!insteonPacket) return;
		std::vector<char> data = insteonPacket->byteArray();
		send(data, true);
		_lastPacketSent = HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonHubX10::send(std::vector<char>& packet, bool printPacket)
{
    try
    {
    	_sendMutex.lock();
    	int32_t written = _socket.proofwrite(packet);
    }
    catch(RPC::SocketOperationException& ex)
    {
    	Output::printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _sendMutex.unlock();
}

void InsteonHubX10::startListening()
{
	try
	{
		stopListening();
		_socket = RPC::SocketOperations(_settings->hostname, _settings->port, _settings->ssl, _settings->verifyCertificate);
		Output::printDebug("Connecting to Insteon Hub X10 with Hostname " + _settings->hostname + " on port " + _settings->port + "...");
		_socket.open();
		Output::printInfo("Connected to Insteon Hub X10 with Hostname " + _settings->hostname + " on port " + _settings->port + ".");
		_stopped = false;
		_listenThread = std::thread(&InsteonHubX10::listen, this);
		Threads::setThreadPriority(_listenThread.native_handle(), 45);
	}
    catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		_socket.close();
		_stopped = true;
		_sendMutex.unlock(); //In case it is deadlocked - shouldn't happen of course
	}
	catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonHubX10::listen()
{
    try
    {
    	uint32_t receivedBytes;
    	int32_t bufferMax = 1024 * 1024;
		std::vector<char> buffer(bufferMax);
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
				receivedBytes = _socket.proofread(&buffer[0], bufferMax);
			}
			catch(RPC::SocketTimeOutException& ex) { continue; }
			catch(RPC::SocketClosedException& ex)
			{
				Output::printWarning("Warning: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(30000));
				continue;
			}
			catch(RPC::SocketOperationException& ex)
			{
				Output::printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(30000));
				continue;
			}
        	if(receivedBytes == 0) continue;
        	if(receivedBytes == bufferMax)
        	{
        		Output::printError("Could not read from Insteon Hub X10: Too much data.");
        		continue;
        	}

			std::shared_ptr<Insteon::InsteonPacket> packet(new Insteon::InsteonPacket(buffer, receivedBytes, HelperFunctions::getTime()));
			std::thread t(&InsteonHubX10::callCallback, this, packet);
			Threads::setThreadPriority(t.native_handle(), 45);
			t.detach();
			_lastPacketReceived = HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
