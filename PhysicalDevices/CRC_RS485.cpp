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

namespace PhysicalDevices
{

CRCRS485::CRCRS485()
{

}

void CRCRS485::init(std::string rfDevice)
{
	_rfDevice = rfDevice;
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

void CRCRS485::sendPacket(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(!packet)
		{
			HelperFunctions::printWarning("Warning: Packet was nullptr.");
			return;
		}
		bool deviceWasClosed = false;
		if(_fileDescriptor == -1)
		{
			deviceWasClosed = true;
			openDevice();
		}
		if(_fileDescriptor == -1) throw(Exception("Couldn't write to CRC RS485 device, because the file descriptor is not valid: " + _rfDevice));
		if(packet->payload()->size() > 54)
		{
			if(GD::debugLevel >= 2) HelperFunctions::printError("Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}

		writeToDevice("As" + packet->hexString() + "\r\n", true);

		if(deviceWasClosed) closeDevice();
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

		_lockfile = "/var/lock" + _rfDevice.substr(_rfDevice.find_last_of('/')) + ".lock";
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
				HelperFunctions::printCritical("CRC RS485 device is in use: " + _rfDevice);
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

		_fileDescriptor = open(_rfDevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

		if(_fileDescriptor == -1)
		{
			HelperFunctions::printCritical("Couldn't open CRC RS485 device: " + _rfDevice);
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
		term.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
		term.c_iflag = IGNPAR;
		term.c_oflag = 0;
		term.c_lflag = 0;
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		cfsetispeed(&term, B19200);
		cfsetospeed(&term, B19200);
		if(tcflush(_fileDescriptor, TCIFLUSH) == -1) throw(Exception("Couldn't flush CRC RS485 device " + _rfDevice));
		if(tcsetattr(_fileDescriptor, TCSANOW, &term) == -1) throw(Exception("Couldn't set CRC RS485 device settings: " + _rfDevice));

		int flags = fcntl(_fileDescriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(Exception("Couldn't set CRC RS485 device to non blocking mode: " + _rfDevice));
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
		if(_fileDescriptor == -1)
		{
			HelperFunctions::printCritical("Couldn't read from CRC RS485 device, because the file descriptor is not valid: " + _rfDevice + ". Trying to reopen...");
			closeDevice();
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			openDevice();
			if(!isOpen()) return std::vector<uint8_t>();
		}
		std::vector<uint8_t> packet;
		int32_t timeoutTime = 500000;
		int32_t i;
		std::vector<uint8_t> localBuffer(1);
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(_fileDescriptor, &readFileDescriptor);

		while((!_stopCallbackThread && localBuffer.at(0) != '\n'))
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
					if(!_stopCallbackThread) continue;
					else return std::vector<uint8_t>();
				case -1:
					HelperFunctions::printError("Error reading from CRC RS485 device: " + _rfDevice);
					return std::vector<uint8_t>();
				case 1:
					break;
				default:
					HelperFunctions::printError("Error reading from CRC RS485 device: " + _rfDevice);
					return std::vector<uint8_t>();
			}

			i = read(_fileDescriptor, &localBuffer.at(0), 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				HelperFunctions::printError("Error reading from CRC RS485 device: " + _rfDevice);
			}
			timeoutTime = 5000;
			packet.push_back(localBuffer[0]);
			if(packet.size() > 200)
			{
				HelperFunctions::printError("CRC RS485 was disconnected.");
				closeDevice();
				return std::vector<uint8_t>();
			}
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

void CRCRS485::writeToDevice(std::string data, bool printSending)
{
    try
    {
    	if(_stopped) return;
        if(_fileDescriptor == -1) throw(Exception("Couldn't write to CRC RS485 device, because the file descriptor is not valid: " + _rfDevice));
        int32_t bytesWritten = 0;
        int32_t i;
        if(GD::debugLevel > 3 && printSending)
        {
            HelperFunctions::printInfo("Info: Sending: " + data.substr(2, data.size() - 4));
        }
        _sendMutex.lock();
        while(bytesWritten < (signed)data.length())
        {
            i = write(_fileDescriptor, data.c_str() + bytesWritten, data.length() - bytesWritten);
            if(i == -1)
            {
                if(errno == EAGAIN) continue;
                throw(Exception("Error writing to CRC RS485 device (3, " + std::to_string(errno) + "): " + _rfDevice));
            }
            bytesWritten += i;
        }

        _sendMutex.unlock();
    }
    catch(const std::exception& ex)
    {
    	_sendMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_sendMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_sendMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
		if(_fileDescriptor != -1)
		{
			//Other X commands than 00 seem to slow down data processing
			//writeToDevice("X00\r\n", false);
			//writeToDevice("Ar\r\n", false);
			closeDevice();
		}
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
        	std::vector<uint8_t> packet = readFromDevice();
        	if(packet.size() > 20) //20 is minimal packet length (=10 Byte)
        	{
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(packetHex, HelperFunctions::getTime()));
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
