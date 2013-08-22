#include "Cul.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RF
{

Cul::Cul()
{

}

void Cul::init(std::string rfDevice)
{
	_rfDevice = rfDevice;
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

void Cul::sendPacket(std::shared_ptr<BidCoSPacket> packet)
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
		if(_fileDescriptor == -1) throw(Exception("Couldn't write to CUL device, because the file descriptor is not valid: " + _rfDevice));
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

void Cul::openDevice()
{
	try
	{
		if(_fileDescriptor != -1) closeDevice();

		_lockfile = "/var/lock" + _rfDevice.substr(_rfDevice.find_last_of('/')) + ".lock";
		int lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(lockfileDescriptor == -1)
		{
			if(errno != EEXIST) throw(Exception("Couldn't create lockfile " + _lockfile));

			int processID = 0;
			std::ifstream lockfileStream(_lockfile.c_str());
			lockfileStream >> processID;
			if(kill(processID, 0) == 0) throw(Exception("CUL device is in use: " + _rfDevice));
			unlink(_lockfile.c_str());
			lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if(lockfileDescriptor == -1) throw(Exception("Couldn't create lockfile " + _lockfile));
		}
		dprintf(lockfileDescriptor, "%10i", getpid());
		close(lockfileDescriptor);
		//std::string chmod("chmod 666 " + _lockfile);
		//system(chmod.c_str());

		_fileDescriptor = open(_rfDevice.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

		if(_fileDescriptor == -1) throw(Exception("Couldn't open CUL device: " + _rfDevice));

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
		if(tcflush(_fileDescriptor, TCIFLUSH) == -1) throw(Exception("Couldn't flush CUL device " + _rfDevice));
		if(tcsetattr(_fileDescriptor, TCSANOW, &term) == -1) throw(Exception("Couldn't set CUL device settings: " + _rfDevice));

		int flags = fcntl(_fileDescriptor, F_GETFL);
		if(!(flags & O_NONBLOCK))
		{
			if(fcntl(_fileDescriptor, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				throw(Exception("Couldn't set CUL device to non blocking mode: " + _rfDevice));
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
		if(_fileDescriptor == -1) throw(Exception("Couldn't read from CUL device, because the file descriptor is not valid: " + _rfDevice));
		std::string packet;
		int32_t i;
		char localBuffer[1];
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(_fileDescriptor, &readFileDescriptor);

		while(localBuffer[0] != '\n')
		{
			//Timeout needs to be set every time, so don't put it outside of the while loop
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			i = select(_fileDescriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			switch(i)
			{
				case 0: //Timeout
					FD_ZERO(&readFileDescriptor);
					FD_SET(_fileDescriptor, &readFileDescriptor);
					timeout.tv_sec = 3;
					if(!_stopCallbackThread) continue;
					else return "";
					break;
				case -1:
					throw(Exception("Error reading from CUL device: " + _rfDevice));
					break;
				case 1:
					break;
				default:
					throw(Exception("Error reading from CUL device: " + _rfDevice));

			}

			i = read(_fileDescriptor, localBuffer, 1);
			if(i == -1)
			{
				if(errno == EAGAIN) continue;
				throw(Exception("Error reading from CUL device: " + _rfDevice));
			}
			packet.push_back(localBuffer[0]);
		}
		return packet;
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
	return "";
}

void Cul::writeToDevice(std::string data, bool printSending)
{
    try
    {
    	if(_stopped) return;
        if(_fileDescriptor == -1) throw(Exception("Couldn't write to CUL device, because the file descriptor is not valid: " + _rfDevice));
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
                throw(Exception("Error writing to CUL device (3, " + std::to_string(errno) + "): " + _rfDevice));
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
		if(_fileDescriptor == -1) throw(Exception("Couldn't listen to CUL device, because the file descriptor is not valid: " + _rfDevice));
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
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				packet->import(packetHex);
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
