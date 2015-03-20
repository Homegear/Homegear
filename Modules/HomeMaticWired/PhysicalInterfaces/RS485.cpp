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

#include "RS485.h"
#include "../HMWiredPacket.h"
#include "../GD.h"

namespace HMWired
{

RS485::RS485(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IHMWiredInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "RS485 Module \"" + settings->id + "\": ");

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 45;
		settings->listenThreadPolicy = SCHED_FIFO;
	}
}

RS485::~RS485()
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

void RS485::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_fileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't write to RS485 serial device, because the file descriptor is not valid: " + _settings->device));
		_lastAction = BaseLib::HelperFunctions::getTime();
		if(packet->payload()->size() > 132)
		{
			if(_bl->debugLevel >= 2) _out.printError("Tried to send packet with payload larger than 128 bytes. That is not supported.");
			return;
		}

		std::shared_ptr<HMWiredPacket> hmWiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmWiredPacket) return;
		std::vector<uint8_t> data = hmWiredPacket->byteArray();
		writeToDevice(data, true);
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

void RS485::sendPacket(std::vector<uint8_t>& rawPacket)
{
	try
	{
		if(rawPacket.empty())
		{
			_out.printWarning("Warning: Packet is empty.");
			return;
		}
		if(_fileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't write to RS485 serial device, because the file descriptor is not valid: " + _settings->device));
		_lastAction = BaseLib::HelperFunctions::getTime();
		if(rawPacket.size() > 132)
		{
			if(_bl->debugLevel >= 2) _out.printError("Tried to send packet with payload larger than 128 bytes. That is not supported.");
			return;
		}

		writeToDevice(rawPacket, true);
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

void RS485::openDevice()
{
	try
	{
		if(_fileDescriptor->descriptor != -1) closeDevice();

		_lockfile = "/var/lock" + _settings->device.substr(_settings->device.find_last_of('/')) + ".lock";
		int lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(lockfileDescriptor == -1)
		{
			if(errno != EEXIST)
			{
				_out.printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}

			int processID = 0;
			std::ifstream lockfileStream(_lockfile.c_str());
			lockfileStream >> processID;
			if(getpid() != processID && kill(processID, 0) == 0)
			{
				_out.printCritical("RS485 serial device is in use: " + _settings->device);
				return;
			}
			unlink(_lockfile.c_str());
			lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if(lockfileDescriptor == -1)
			{
				_out.printCritical("Couldn't create lockfile " + _lockfile + ": " + strerror(errno));
				return;
			}
		}
		dprintf(lockfileDescriptor, "%10i", getpid());
		close(lockfileDescriptor);
		//std::string chmod("chmod 666 " + _lockfile);
		//system(chmod.c_str());

		_fileDescriptor = _bl->fileDescriptorManager.add(open(_settings->device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY ));

		if(_fileDescriptor->descriptor == -1)
		{
			_out.printCritical("Couldn't open RS485 serial device \"" + _settings->device + "\": " + strerror(errno));
			return;
		}

		setupDevice();
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

void RS485::closeDevice()
{
	try
	{
		_bl->fileDescriptorManager.close(_fileDescriptor);
		unlink(_lockfile.c_str());
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

void RS485::setup(int32_t userID, int32_t groupID)
{
	try
	{
		setDevicePermission(userID, groupID);
		if(_settings->gpio.find(1) != _settings->gpio.end())
		{
			exportGPIO(1);
			setGPIOPermission(1, userID, groupID, false);
			setGPIODirection(1, GPIODirection::OUT);
		}
		if(_settings->gpio.find(2) != _settings->gpio.end())
		{
			exportGPIO(2);
			setGPIOPermission(2, userID, groupID, false);
			setGPIODirection(2, GPIODirection::OUT);
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

void RS485::setupDevice()
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;
		memset(&_termios, 0, sizeof(termios));
		_termios.c_cflag = B19200 | CS8 | CREAD | PARENB;
		_termios.c_iflag = 0;
		_termios.c_oflag = 0;
		_termios.c_lflag = 0;
		_termios.c_cc[VMIN] = 0;
		_termios.c_cc[VTIME] = 0;
		cfsetispeed(&_termios, B19200);
		cfsetospeed(&_termios, B19200);
		if(tcflush(_fileDescriptor->descriptor, TCIFLUSH) == -1) throw(BaseLib::Exception("Couldn't flush RS485 serial device " + _settings->device));
		if(tcsetattr(_fileDescriptor->descriptor, TCSANOW, &_termios) == -1) throw(BaseLib::Exception("Couldn't set RS485 serial device settings: " + _settings->device));

		int flags = fcntl(_fileDescriptor->descriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor->descriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(BaseLib::Exception("Couldn't set RS485 serial device to non blocking mode: " + _settings->device));
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

void RS485::search(std::vector<int32_t>& foundDevices)
{
	try
	{
		int32_t startTime = BaseLib::HelperFunctions::getTimeSeconds();
		foundDevices.clear();
		_searchResponse = 0;
		_searchMode = true;
		int32_t addressMask = 0;
		bool backwards = false;
		uint32_t address = 0;
		uint32_t address2 = 0;
		int64_t time = 0;
		int32_t retries = 0;
		std::pair<uint32_t, std::shared_ptr<HMWiredPacket>> packet;
		while(true)
		{
			if(BaseLib::HelperFunctions::getTimeSeconds() - startTime > 180)
			{
				GD::out.printError("Error: Device search timed out.");
				break;
			}
			std::vector<uint8_t> payload;
			if(packet.second && packet.second->addressMask() == addressMask && (uint32_t)packet.second->destinationAddress() == address)
			{
				if(packet.first < 3) packet.first++;
				else
				{
					GD::out.printError("Error: Prevented deadlock while searching for HomeMatic Wired devices.");
					address++;
					backwards = true;
				}
			}
			else
			{
				packet.first = 0;
				packet.second.reset(new HMWiredPacket(HMWiredPacketType::discovery, 0, address, false, 0, 0, addressMask, payload));
			}
			time = BaseLib::HelperFunctions::getTime();
			sendPacket(packet.second);

			int32_t j = 0;
			while(j * 3 < (signed)_settings->responseDelay)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				if(_searchResponse >= time) break;
				j++;
			}
			if(_searchResponse >= time)
			{
				_searchResponse = 0;
				retries = 0;
				if(addressMask < 31)
				{
					backwards = false;
					addressMask++;
				}
				else
				{
					GD::out.printMessage("Peer found with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8));
					if(address > 0) foundDevices.push_back(address);
					backwards = true;
					address++;
					address2 = address;
					int32_t shifts = 0;
					while(!(address2 & 1))
					{
						address2 >>= 1;
						addressMask--;
						shifts++;
					}
					address = address2 << shifts;
				}
			}
			else
			{
				if(retries < 2) retries++;
				else
				{
					if(addressMask == 0 && (address & 0x80000000)) break;
					retries = 0;
					if(addressMask == 0) break;
					if(backwards)
					{
						//Example:
						//Input:
						//0x8C      0x00      0d21
						//10001100  00000000  10101
						//Output:
						//90        0x00      0d19
						//10010000  00000000  10011
						address2 = address;
						int32_t shifts = 0;
						while(!(address2 & 1))
						{
							address2 >>= 1;
							shifts++;
						}
						address2++;
						while(!(address2 & 1))
						{
							address2 >>= 1;
							shifts++;
							addressMask--;
						}
						address = address2 << shifts;
					}
					else address |= (1 << (31 - addressMask));
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _searchMode = false;
}

std::vector<uint8_t> RS485::readFromDevice()
{
	try
	{
		if(_stopped) return std::vector<uint8_t>();
		if(_fileDescriptor->descriptor == -1)
		{
			_out.printCritical("Couldn't read from RS485 serial device, because the file descriptor is not valid: " + _settings->device + ". Trying to reopen...");
			closeDevice();
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			openDevice();
			if(!isOpen()) return std::vector<uint8_t>();
		}
		std::vector<uint8_t> packet;
		std::vector<uint8_t> escapedPacket;
		if(_firstByte && (BaseLib::HelperFunctions::getTime() - _lastAction) < 10)
		{
			packet.push_back(_firstByte);
			escapedPacket.push_back(_firstByte);
			_firstByte = 0;
		}
		int32_t timeoutTime = 500000;
		int32_t i;
		bool escapeByte = false;
		_receivingSending = false;
		uint32_t length = 0;
		std::vector<uint8_t> localBuffer(1);
		fd_set readFileDescriptor;

		while(!_stopCallbackThread)
		{
			FD_ZERO(&readFileDescriptor);
			FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = timeoutTime;
			i = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(i == 0) //Timeout
			{
					if(!packet.empty()) break;
					if(!_stopCallbackThread)
					{
						_firstByte = 0;
						continue;
					}
					else break;
			}
			else if(i == -1)
			{
					_out.printError("Error reading from RS485 serial device: " + _settings->device);
					break;
			}
			else if(i != 1)
			{
					_out.printError("Error reading from RS485 serial device: " + _settings->device);
					break;
			}
			if(!_sending) _sendMutex.try_lock(); //Don't change to "lock", because it is called for each received byte!
			else if(!_settings->oneWay)
			{
				_sendingMutex.try_lock();
				_receivingSending = true;
			}
			i = read(_fileDescriptor->descriptor, &localBuffer.at(0), 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				_out.printError("Error reading from RS485 serial device: " + _settings->device);
				break;
			}
			if(i == 0 || (packet.empty() && localBuffer[0] == 0)) break;
			_lastAction = BaseLib::HelperFunctions::getTime();
			if(!packet.empty() && (localBuffer[0] == 0xFD || localBuffer[0] == 0xFE))
			{
				_firstByte = localBuffer[0];
				_out.printWarning("Invalid byte received from RS485 serial device (collision?): 0x" + BaseLib::HelperFunctions::getHexString(localBuffer[0], 2));
				break;
			}
			if(_receivingSending) escapedPacket.push_back(localBuffer[0]);
			if(escapeByte)
			{
				escapeByte = false;
				packet.push_back(localBuffer[0] | 0x80);
			}
			else if(localBuffer[0] == 0xFC && !packet.empty()) escapeByte = true;
			else packet.push_back(localBuffer[0]);
			if(length == 0 && !packet.empty())
			{
				if(packet.at(0) == 0xFD)
				{
					if(packet.size() > 6)
					{
						if((packet.at(5) & 3) == 3 || (packet.at(5) & 8) == 0) length = packet.at(6) + 7; //Discovery packet or packet without sender address
						else if(packet.size() > 10) length = packet.at(10) + 11; //Normal packet
					}
				}
				else if(packet.at(0) == 0xFE) { if(packet.size() > 3) length = packet.at(3); }
				else if(packet.at(0) == 0xF8)
				{
					_out.printInfo("Info: Response received to discovery packet.");
					packet.clear();
					_searchResponse = BaseLib::HelperFunctions::getTime();
					break;
				}
				else if(_searchMode) //Devices sometimes receive nonsense instead of 0xF8
				{
					_out.printInfo("Info: Response received to discovery packet.");
					_out.printWarning("Warning: Correcting wrong response: " + BaseLib::HelperFunctions::getHexString(packet) + ". This is normal for RS485 modules when searching for new devices.");
					packet.clear();
					_searchResponse = BaseLib::HelperFunctions::getTime();
					break;
				}
			}
			else if(packet.size() == length) break;
			timeoutTime = _settings->timeout * 1000;
		}
		if(_receivingSending)
		{
			_receivedSentPacket = escapedPacket;
			packet.clear();
			_sendingMutex.unlock();
			while(_sending) std::this_thread::sleep_for(std::chrono::microseconds(500));
			_receivingSending = false;
		}
		else _sendMutex.unlock();
		return packet;
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _receivingSending = false;
    _sendingMutex.unlock();
    _sendMutex.unlock();
	return std::vector<uint8_t>();
}

void RS485::writeToDevice(std::vector<uint8_t>& packet, bool printPacket)
{
    try
    {
    	if(_stopped || packet.empty()) return;
        if(_fileDescriptor->descriptor == -1) throw(BaseLib::Exception("Couldn't write to RS485 serial device, because the file descriptor is not valid: " + _settings->device));
        _sendMutex.lock();
        //Before _sending is set to true wait for the last sending to finish. Without this line
        //the new sending is sometimes not detected when two packets are sent at the same time.
        while(_receivingSending) std::this_thread::sleep_for(std::chrono::microseconds(500));
        if(_bl->debugLevel > 4) _out.printDebug("Debug: RS485 device: Got lock for sending... (Packet: " + BaseLib::HelperFunctions::getHexString(packet) + ")");
        _lastPacketSent = BaseLib::HelperFunctions::getTime(); //Sending takes some time, so we set _lastPacketSent two times
        _sending = true;
        /*if(_settings->oneWay && gpioDefined(1))
        {
        	if(!gpioOpen(1))
			{
				closeGPIO(1);
				openGPIO(1, false);
				if(!gpioOpen(1))
				{
					_out.printError("Error: Could not send RS485 packet. GPIO to enable sending is could not be opened.");
					_sending = false;
					_sendMutex.unlock();
					return;
				}
			}
        	setGPIO(1, !((bool)_settings->enableRXValue));
        }*/
		int32_t bytesWritten = 0;
		_receivedSentPacket.clear();
		if(_bl->debugLevel > 3 && printPacket) _out.printInfo("Info: Sending: " + BaseLib::HelperFunctions::getHexString(packet));
		while(bytesWritten < (signed)packet.size())
		{
			int32_t i = write(_fileDescriptor->descriptor, &packet.at(0) + bytesWritten, packet.size() - bytesWritten);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				throw(BaseLib::Exception("Error writing to RS485 serial device (3, " + std::to_string(errno) + "): " + _settings->device));
			}
			bytesWritten += i;
		}
		if(_settings->oneWay)
		{
			fsync(_fileDescriptor->descriptor);
		}
		else
		{
			for(int32_t i = 0; i < 50; i++)
			{
				if(_receivingSending || !_receivedSentPacket.empty()) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if(!_sendingMutex.try_lock_for(std::chrono::milliseconds(200)) && GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Debug: Could not get sendMutex lock.");
			}
			if(_receivedSentPacket.empty()) _out.printWarning("Error sending HomeMatic Wired packet: No sending detected.");
			else if(_receivedSentPacket != packet) _out.printWarning("Error sending HomeMatic Wired packet: Collision (received packet was: " + BaseLib::HelperFunctions::getHexString(_receivedSentPacket) + ")");
			_sendingMutex.unlock();
		}
		/*if(_settings->oneWay && gpioDefined(1))
		{
			if(!gpioOpen(1))
			{
				closeGPIO(1);
				openGPIO(1, false);
				if(!gpioOpen(1))
				{
					_sending = false;
					_sendMutex.unlock();
					return;
				}
			}
			setGPIO(1, (bool)_settings->enableRXValue);
		}*/
    }
    catch(const std::exception& ex)
    {
    	_sendingMutex.unlock();
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_sendingMutex.unlock();
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_sendingMutex.unlock();
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _lastPacketSent = BaseLib::HelperFunctions::getTime();
    _sending = false;
    _sendMutex.unlock();
}

void RS485::startListening()
{
	try
	{
		stopListening();
		openDevice();
		if(_fileDescriptor->descriptor == -1) return;
		if(gpioDefined(1))
		{
			openGPIO(1, false);
			setGPIO(1, (bool)_settings->enableRXValue);
			if(!_settings->oneWay) closeGPIO(1);
		}
		if(gpioDefined(2))
		{
			openGPIO(2, false);
			setGPIO(2, (bool)_settings->enableTXValue);
			closeGPIO(2);
		}
		_stopped = false;
		_listenThread = std::thread(&RS485::listen, this);
		BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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

void RS485::stopListening()
{
	try
	{
		if(_listenThread.joinable())
		{
			_stopCallbackThread = true;
			_listenThread.join();
		}
		_stopCallbackThread = false;
		if(_fileDescriptor->descriptor != -1) closeDevice();
		if(gpioDefined(1) && _settings->oneWay) closeGPIO(1);
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

void RS485::listen()
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
        	if(rawPacket.empty()) continue;
			std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(rawPacket, BaseLib::HelperFunctions::getTime()));
			if(packet->type() != HMWiredPacketType::none) raisePacketReceived(packet);
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
