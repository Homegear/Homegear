/* Copyright 2013 Sathya Laufer
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

#include "CRC_RS485.h"
#include "../GD.h"
#include "../HelperFunctions.h"
#include "../HomeMaticWired/HMWiredPacket.h"

namespace PhysicalDevices
{

CRCRS485::CRCRS485(std::shared_ptr<PhysicalDeviceSettings> settings) : PhysicalDevice(settings)
{
}

CRCRS485::~CRCRS485()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		closeDevice();
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::sendPacket(std::shared_ptr<Packet> packet)
{
	try
	{
		if(!packet)
		{
			HelperFunctions::printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_fileDescriptor == -1) throw(Exception("Couldn't write to CRC RS485 device, because the file descriptor is not valid: " + _settings->device));
		_lastAction = HelperFunctions::getTime();
		if(packet->payload()->size() > 128)
		{
			if(GD::debugLevel >= 2) HelperFunctions::printError("Tried to send packet with payload larger than 128 bytes. That is not supported.");
			return;
		}

		std::vector<uint8_t> data = packet->byteArray();
		writeToDevice(data, true);
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::openDevice()
{
	try
	{
		if(_fileDescriptor != -1) closeDevice();

		_lockfile = "/var/lock" + _settings->device.substr(_settings->device.find_last_of('/')) + ".lock";
		int lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(lockfileDescriptor == -1)
		{
			if(errno != EEXIST)
			{
				HelperFunctions::printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}

			int processID = 0;
			std::ifstream lockfileStream(_lockfile.c_str());
			lockfileStream >> processID;
			if(getpid() != processID && kill(processID, 0) == 0)
			{
				HelperFunctions::printCritical("CRC RS485 device is in use: " + _settings->device);
				return;
			}
			unlink(_lockfile.c_str());
			lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if(lockfileDescriptor == -1)
			{
				HelperFunctions::printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}
		}
		dprintf(lockfileDescriptor, "%10i", getpid());
		close(lockfileDescriptor);
		//std::string chmod("chmod 666 " + _lockfile);
		//system(chmod.c_str());

		_fileDescriptor = open(_settings->device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

		if(_fileDescriptor == -1)
		{
			HelperFunctions::printCritical("Couldn't open CRC RS485 device: " + _settings->device);
			return;
		}

		setupDevice();
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::closeDevice()
{
	try
	{
		if(_fileDescriptor == -1) return;
		close(_fileDescriptor);
		_fileDescriptor = -1;
		unlink(_lockfile.c_str());
	}
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::setupDevice()
{
	try
	{
		if(_fileDescriptor == -1) return;
		struct termios term;
		term.c_cflag = B19200 | CS8 | CREAD | PARENB;
		term.c_iflag = 0;
		term.c_oflag = 0;
		term.c_lflag = 0;
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		cfsetispeed(&term, B19200);
		cfsetospeed(&term, B19200);
		if(tcflush(_fileDescriptor, TCIFLUSH) == -1) throw(Exception("Couldn't flush CRC RS485 device " + _settings->device));
		if(tcsetattr(_fileDescriptor, TCSANOW, &term) == -1) throw(Exception("Couldn't set CRC RS485 device settings: " + _settings->device));

		int flags = fcntl(_fileDescriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(Exception("Couldn't set CRC RS485 device to non blocking mode: " + _settings->device));
			}
		}
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::vector<uint8_t> CRCRS485::readFromDevice()
{
	try
	{
		if(_stopped) return std::vector<uint8_t>();
		if(_fileDescriptor == -1)
		{
			HelperFunctions::printCritical("Couldn't read from CRC RS485 device, because the file descriptor is not valid: " + _settings->device + ". Trying to reopen...");
			closeDevice();
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			openDevice();
			if(!isOpen()) return std::vector<uint8_t>();
		}
		std::vector<uint8_t> packet;
		if(_firstByte && (HelperFunctions::getTime() - _lastAction) < 10)
		{
			packet.push_back(_firstByte);
			_firstByte = 0;
		}
		int32_t timeoutTime = 500000;
		int32_t i;
		bool escapeByte = false;
		uint32_t length = 0;
		std::vector<uint8_t> localBuffer(1);
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(_fileDescriptor, &readFileDescriptor);

		while(!_stopCallbackThread)
		{
			FD_ZERO(&readFileDescriptor);
			FD_SET(_fileDescriptor, &readFileDescriptor);
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = timeoutTime;
			i = select(_fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(i)
			{
				case 0: //Timeout
					if(!packet.empty()) return packet;
					if(!_stopCallbackThread)
					{
						_firstByte = 0;
						continue;
					}
					else return std::vector<uint8_t>();
				case -1:
					HelperFunctions::printError("Error reading from CRC RS485 device: " + _settings->device);
					return std::vector<uint8_t>();
				case 1:
					break;
				default:
					HelperFunctions::printError("Error reading from CRC RS485 device: " + _settings->device);
					return std::vector<uint8_t>();
			}

			i = read(_fileDescriptor, &localBuffer.at(0), 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				HelperFunctions::printError("Error reading from CRC RS485 device: " + _settings->device);
			}
			_lastAction = HelperFunctions::getTime();
			if(!packet.empty() && (localBuffer[0] == 0xFD || localBuffer[0] == 0xFE))
			{
				_firstByte = localBuffer[0];
				HelperFunctions::printWarning("Invalid byte received from CRC RS485 device (collision?): 0x" + HelperFunctions::getHexString(localBuffer[0], 2));
				return packet;
			}
			if(escapeByte)
			{
				escapeByte = false;
				packet.push_back(localBuffer[0] | 0x80);
			}
			else if(localBuffer[0] == 0xFC) escapeByte = true;
			else packet.push_back(localBuffer[0]);
			if(length == 0)
			{
				if(packet.at(0) == 0xFD && packet.size() > 6)
				{
					if((packet.at(5) & 3) == 3 || (packet.at(5) & 8) == 0) length = packet.at(6) + 7; //Discovery packet or packet without sender address
					else if(packet.size() > 10) length = packet.at(10) + 11; //Normal packet
				}
				else if(packet.at(0) == 0xFE && packet.size() > 3) length = packet.at(3);
				else if(packet.at(0) == 0xF8) return packet;
			}
			else if(packet.size() == length) return packet;
			timeoutTime = 7000;
		}
		return packet;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::vector<uint8_t>();
}

void CRCRS485::writeToDevice(std::vector<uint8_t>& packet, bool printPacket)
{
    try
    {
    	if(_stopped || packet.empty()) return;
        if(_fileDescriptor == -1) throw(Exception("Couldn't write to CRC RS485 device, because the file descriptor is not valid: " + _settings->device));
        int32_t bytesWritten = 0;
        int32_t i;
        if(GD::debugLevel > 3 && printPacket)
        {
            HelperFunctions::printInfo("Info: Sending: " + HelperFunctions::getHexString(packet));
        }
        _sendMutex.lock();
        while(bytesWritten < (signed)packet.size())
        {
            i = write(_fileDescriptor, &packet.at(0) + bytesWritten, packet.size() - bytesWritten);
            if(i == -1)
            {
                if(errno == EAGAIN) continue;
                throw(Exception("Error writing to CRC RS485 device (3, " + std::to_string(errno) + "): " + _settings->device));
            }
            bytesWritten += i;
        }
    }
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _sendMutex.unlock();
}

void CRCRS485::startListening()
{
	try
	{
		stopListening();
		openDevice();
		if(_fileDescriptor == -1) return;
		_stopped = false;
		_listenThread = std::thread(&CRCRS485::listen, this);
		HelperFunctions::setThreadPriority(_listenThread.native_handle(), 45);
	}
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::stopListening()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		if(_fileDescriptor != -1) closeDevice();
		_stopped = true;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void CRCRS485::listen()
{
    try
    {
        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(200));
        		if(_stopCallbackThread) return;
        		continue;
        	}
        	std::vector<uint8_t> rawPacket = readFromDevice();

			std::shared_ptr<HMWired::HMWiredPacket> packet(new HMWired::HMWiredPacket(rawPacket, HelperFunctions::getTime()));
			if(packet->type() != HMWired::HMWiredPacketType::none)
			{
				std::thread t(&CRCRS485::callCallback, this, packet);
				HelperFunctions::setThreadPriority(t.native_handle(), 45);
				t.detach();
			}
        }
    }
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
