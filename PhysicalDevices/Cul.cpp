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

#include "Cul.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace PhysicalDevices
{

Cul::Cul(std::shared_ptr<PhysicalDeviceSettings> settings) : PhysicalDevice(settings)
{
}

Cul::~Cul()
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

void Cul::sendPacket(std::shared_ptr<Packet> packet)
{
	try
	{
		if(!packet)
		{
			HelperFunctions::printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_fileDescriptor == -1) throw(Exception("Couldn't write to CUL device, because the file descriptor is not valid: " + _settings->device));
		if(packet->payload()->size() > 54)
		{
			if(GD::debugLevel >= 2) HelperFunctions::printError("Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}

		writeToDevice("As" + packet->hexString() + "\r\n", true);
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

void Cul::openDevice()
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
				HelperFunctions::printCritical("CUL device is in use: " + _settings->device);
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
			HelperFunctions::printCritical("Couldn't open CUL device: " + _settings->device);
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

void Cul::closeDevice()
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

void Cul::setupDevice()
{
	try
	{
		if(_fileDescriptor == -1) return;
		struct termios term;
		term.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
		term.c_iflag = IGNPAR;
		term.c_oflag = 0;
		term.c_lflag = 0;
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		cfsetispeed(&term, B9600);
		cfsetospeed(&term, B9600);
		if(tcflush(_fileDescriptor, TCIFLUSH) == -1) throw(Exception("Couldn't flush CUL device " + _settings->device));
		if(tcsetattr(_fileDescriptor, TCSANOW, &term) == -1) throw(Exception("Couldn't set CUL device settings: " + _settings->device));

		int flags = fcntl(_fileDescriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(Exception("Couldn't set CUL device to non blocking mode: " + _settings->device));
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

std::string Cul::readFromDevice()
{
	try
	{
		if(_stopped) return "";
		if(_fileDescriptor == -1)
		{
			HelperFunctions::printCritical("Couldn't read from CUL device, because the file descriptor is not valid: " + _settings->device + ". Trying to reopen...");
			closeDevice();
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			openDevice();
			if(!isOpen()) return "";
		}
		std::string packet;
		int32_t i;
		char localBuffer[1];
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(_fileDescriptor, &readFileDescriptor);

		while((!_stopCallbackThread && localBuffer[0] != '\n'))
		{
			FD_ZERO(&readFileDescriptor);
			FD_SET(_fileDescriptor, &readFileDescriptor);
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			i = select(_fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(i)
			{
				case 0: //Timeout
					if(!_stopCallbackThread) continue;
					else return "";
				case -1:
					HelperFunctions::printError("Error reading from CUL device: " + _settings->device);
					return "";
				case 1:
					break;
				default:
					HelperFunctions::printError("Error reading from CUL device: " + _settings->device);
					return "";
			}

			i = read(_fileDescriptor, localBuffer, 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				HelperFunctions::printError("Error reading from CUL device: " + _settings->device);
				return "";
			}
			packet.push_back(localBuffer[0]);
			if(packet.size() > 200)
			{
				HelperFunctions::printError("CUL was disconnected.");
				closeDevice();
				return "";
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
	return "";
}

void Cul::writeToDevice(std::string data, bool printSending)
{
    try
    {
    	if(_stopped) return;
        if(_fileDescriptor == -1) throw(Exception("Couldn't write to CUL device, because the file descriptor is not valid: " + _settings->device));
        int32_t bytesWritten = 0;
        int32_t i;
        //struct timeval timeout;
        //timeout.tv_sec = 0;
        //timeout.tv_usec = 200000;
        //fd_set writeFileDescriptor;
        if(GD::debugLevel > 3 && printSending)
        {
            HelperFunctions::printInfo("Info: Sending: " + data.substr(2, data.size() - 4));
        }
        _sendMutex.lock();
        //FD_ZERO(&writeFileDescriptor);
        //FD_SET(_fileDescriptor, &writeFileDescriptor);
        while(bytesWritten < (signed)data.length())
        {
            /*i = select(_fileDescriptor + 1, NULL, &writeFileDescriptor, NULL, &timeout);
            switch(i)
            {
                case 0:
                    if(GD::debugLevel >= 3) HelperFunctions::printMessage( "Warning: Writing to CUL device timed out: " + _culDevice << std::endl;
                    break;
                case -1:
                    throw(Exception("Error writing to CUL device (1): " + _culDevice));
                    break;
                case 1:
                    break;
                default:
                    throw(Exception("Error writing to CUL device (2): " + _culDevice));

            }*/
            i = write(_fileDescriptor, data.c_str() + bytesWritten, data.length() - bytesWritten);
            if(i == -1)
            {
                if(errno == EAGAIN) continue;
                throw(Exception("Error writing to CUL device (3, " + std::to_string(errno) + "): " + _settings->device));
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

void Cul::startListening()
{
	try
	{
		stopListening();
		openDevice();
		if(_fileDescriptor == -1) return;
		_stopped = false;
		writeToDevice("Ax\r\n", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		writeToDevice("X20\r\n", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		writeToDevice("Ar\r\n", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		_listenThread = std::thread(&Cul::listen, this);
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

void Cul::stopListening()
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
			writeToDevice("X00\r\n", false);
			writeToDevice("Ar\r\n", false);
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

void Cul::listen()
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
        	std::string packetHex = readFromDevice();
        	if(packetHex.size() > 21) //21 is minimal packet length (=10 Byte + CUL "A")
        	{
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(packetHex, HelperFunctions::getTime()));
				std::thread t(&Cul::callCallback, this, packet);
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
