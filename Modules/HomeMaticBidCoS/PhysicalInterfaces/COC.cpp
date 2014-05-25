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

#include "COC.h"
#include "../BidCoSPacket.h"
#include "../../Base/BaseLib.h"

namespace BidCoS
{

COC::COC(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : BidCoSDevice(settings)
{
}

COC::~COC()
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
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			BaseLib::Output::printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_fileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't write to COC device, because the file descriptor is not valid: " + _settings->device));
		if(packet->payload()->size() > 54)
		{
			if(BaseLib::Obj::ins->debugLevel >= 2) BaseLib::Output::printError("Error: Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}
		if(_updateMode)
		{
			std::shared_ptr<BidCoS::BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoS::BidCoSPacket>(packet));
			if(!bidCoSPacket) return;
			if(!bidCoSPacket->isUpdatePacket())
			{
				BaseLib::Output::printInfo("Info: Can't send packet to BidCoS peer with address 0x" + BaseLib::HelperFunctions::getHexString(packet->destinationAddress(), 6) + ", because update mode is enabled.");
				return;
			}
		}

		writeToDevice("As" + packet->hexString() + "\nAr\n", true);
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::enableUpdateMode()
{
	try
	{
		_updateMode = true;
		writeToDevice("AR\n", false);
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::disableUpdateMode()
{
	try
	{
		stopListening();
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		startListening();
		_updateMode = false;
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::openDevice()
{
	try
	{
		if(_fileDescriptor->descriptor > -1) closeDevice();

		_lockfile = "/var/lock" + _settings->device.substr(_settings->device.find_last_of('/')) + ".lock";
		int lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(lockfileDescriptor == -1)
		{
			if(errno != EEXIST)
			{
				BaseLib::Output::printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}

			int processID = 0;
			std::ifstream lockfileStream(_lockfile.c_str());
			lockfileStream >> processID;
			if(getpid() != processID && kill(processID, 0) == 0)
			{
				BaseLib::Output::printCritical("COC device is in use: " + _settings->device);
				return;
			}
			unlink(_lockfile.c_str());
			lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if(lockfileDescriptor == -1)
			{
				BaseLib::Output::printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}
		}
		dprintf(lockfileDescriptor, "%10i", getpid());
		close(lockfileDescriptor);
		//std::string chmod("chmod 666 " + _lockfile);
		//system(chmod.c_str());

		_fileDescriptor = BaseLib::Obj::ins->fileDescriptorManager.add(open(_settings->device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY));
		if(_fileDescriptor->descriptor == -1)
		{
			BaseLib::Output::printCritical("Couldn't open COC device \"" + _settings->device + "\": " + strerror(errno));
			return;
		}

		setupDevice();
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::closeDevice()
{
	try
	{
		BaseLib::Obj::ins->fileDescriptorManager.close(_fileDescriptor);
		unlink(_lockfile.c_str());
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::setupDevice()
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;
		struct termios term;
		term.c_cflag = B38400 | CS8 | CREAD;
		term.c_iflag = 0;
		term.c_oflag = 0;
		term.c_lflag = 0;
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 0;
		cfsetispeed(&term, B38400);
		cfsetospeed(&term, B38400);
		if(tcflush(_fileDescriptor->descriptor, TCIFLUSH) == -1) throw(BaseLib::Exception("Couldn't flush COC device " + _settings->device));
		if(tcsetattr(_fileDescriptor->descriptor, TCSANOW, &term) == -1) throw(BaseLib::Exception("Couldn't set COC device settings: " + _settings->device));

		int flags = fcntl(_fileDescriptor->descriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor->descriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(BaseLib::Exception("Couldn't set COC device to non blocking mode: " + _settings->device));
			}
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string COC::readFromDevice()
{
	try
	{
		if(_stopped) return "";
		if(_fileDescriptor->descriptor == -1)
		{
			BaseLib::Output::printCritical("Couldn't read from COC device, because the file descriptor is not valid: " + _settings->device + ". Trying to reopen...");
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
		FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);

		while((!_stopCallbackThread && localBuffer[0] != '\n' && _fileDescriptor->descriptor > -1))
		{
			FD_ZERO(&readFileDescriptor);
			FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			i = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(i)
			{
				case 0: //Timeout
					if(!_stopCallbackThread) continue;
					else return "";
				case -1:
					BaseLib::Output::printError("Error reading from COC device: " + _settings->device);
					return "";
				case 1:
					break;
				default:
					BaseLib::Output::printError("Error reading from COC device: " + _settings->device);
					return "";
			}
			i = read(_fileDescriptor->descriptor, localBuffer, 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				BaseLib::Output::printError("Error reading from COC device: " + _settings->device);
				return "";
			}
			packet.push_back(localBuffer[0]);
			if(packet.size() > 200)
			{
				BaseLib::Output::printError("COC was disconnected.");
				closeDevice();
				return "";
			}
		}
		return packet;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return "";
}

void COC::writeToDevice(std::string data, bool printSending)
{
    try
    {
    	if(_stopped) return;
        if(_fileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't write to COC device, because the file descriptor is not valid: " + _settings->device));
        int32_t bytesWritten = 0;
        int32_t i;
        if(BaseLib::Obj::ins->debugLevel > 3 && printSending)
        {
            BaseLib::Output::printInfo("Info: Sending: " + data.substr(2, data.size() - 6));
        }
        _sendMutex.lock();
        while(bytesWritten < (signed)data.length())
        {
            i = write(_fileDescriptor->descriptor, data.c_str() + bytesWritten, data.length() - bytesWritten);
            if(i == -1)
            {
                if(errno == EAGAIN) continue;
                throw(BaseLib::Exception("Error writing to COC device (3, " + std::to_string(errno) + "): " + _settings->device));
            }
            bytesWritten += i;
        }
    }
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _sendMutex.unlock();
    _lastPacketSent = BaseLib::HelperFunctions::getTime();
}

void COC::startListening()
{
	try
	{
		stopListening();
		openDevice();
		if(_fileDescriptor->descriptor == -1) return;
		if(gpioDefined(2))
		{
			openGPIO(2, false);
			setGPIO(2, true);
			closeGPIO(2);
		}
		if(gpioDefined(1))
		{
			openGPIO(1, false);
			setGPIO(1, false);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			setGPIO(1, true);
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			closeGPIO(1);
		}
		_stopped = false;
		writeToDevice("X21\nAr\n", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		_listenThread = std::thread(&COC::listen, this);
		BaseLib::Threads::setThreadPriority(_listenThread.native_handle(), 45);
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::stopListening()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		if(_fileDescriptor->descriptor > -1)
		{
			//Other X commands than 00 seem to slow down data processing
			writeToDevice("Ax\nX00\n", false);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			closeDevice();
		}
		_stopped = true;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::listen()
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
        	if(packetHex.size() > 21) //21 is minimal packet length (=10 Byte + COC "A")
        	{
				std::shared_ptr<BidCoS::BidCoSPacket> packet(new BidCoS::BidCoSPacket(packetHex, BaseLib::HelperFunctions::getTime()));
				raisePacketReceived(packet);
        	}
        	else if(!packetHex.empty()) BaseLib::Output::printWarning("Warning: Too short packet received: " + packetHex);
        }
    }
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::setup(int32_t userID, int32_t groupID)
{
    try
    {
    	setDevicePermission(userID, groupID);
    	exportGPIO(1);
		setGPIOPermission(1, userID, groupID, false);
		setGPIODirection(1, GPIODirection::OUT);
		exportGPIO(2);
		setGPIOPermission(2, userID, groupID, false);
		setGPIODirection(2, GPIODirection::OUT);
    }
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
