#ifdef TI_CC1100

#include "TICC1100.h"

namespace RF
{

TICC1100::TICC1100()
{
	_transfer =  { (uint64_t)0, (uint64_t)0, (uint32_t)0, (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };
}

TICC1100::~TICC1100()
{

}

void TICC1100::init(std::string rfDevice)
{
	_rfDevice = rfDevice;
}

void TICC1100::openDevice()
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
			if(kill(processID, 0) == 0) throw(Exception("Rf device is in use: " + _rfDevice));
			unlink(_lockfile.c_str());
			lockfileDescriptor = open(_lockfile.c_str(), O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if(lockfileDescriptor == -1) throw(Exception("Couldn't create lockfile " + _lockfile));
		}
		dprintf(lockfileDescriptor, "%10i", getpid());
		close(lockfileDescriptor);
		//std::string chmod("chmod 666 " + _lockfile);
		//system(chmod.c_str());

		_fileDescriptor = open(_rfDevice.c_str(), O_RDWR);
		usleep(1000);

		if(_fileDescriptor == -1) throw(Exception("Couldn't open rf device: " + _rfDevice));

		setupDevice();
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::closeDevice()
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::setupDevice()
{
	try
	{
		if(_fileDescriptor == -1) return;

		uint8_t mode = 0;
		uint8_t bits = 8;
		uint32_t speed = 4000000; //4MHz, see page 25 in datasheet

		if(ioctl(_fileDescriptor, SPI_IOC_WR_MODE, &mode)) throw(Exception("Couldn't set spi mode on device " + _rfDevice));
		if(ioctl(_fileDescriptor, SPI_IOC_RD_MODE, &mode)) throw(Exception("Couldn't get spi mode off device " + _rfDevice));

		if(ioctl(_fileDescriptor, SPI_IOC_WR_BITS_PER_WORD, &bits)) throw(Exception("Couldn't set bits per word on device " + _rfDevice));
		if(ioctl(_fileDescriptor, SPI_IOC_RD_BITS_PER_WORD, &bits)) throw(Exception("Couldn't get bits per word off device " + _rfDevice));

		if(ioctl(_fileDescriptor, SPI_IOC_WR_MAX_SPEED_HZ, &bits)) throw(Exception("Couldn't set speed on device " + _rfDevice));
		if(ioctl(_fileDescriptor, SPI_IOC_RD_MAX_SPEED_HZ, &bits)) throw(Exception("Couldn't get speed off device " + _rfDevice));
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::sendPacket(std::shared_ptr<BidCoSPacket> packet)
{
}

void TICC1100::readwrite(std::vector<uint8_t>& data)
{
	try
	{
		_sendMutex.lock();
		_transfer.tx_buf = (uint64_t)&data[0];
		_transfer.rx_buf = (uint64_t)&data[0];
		_transfer.len = (uint32_t)data.size();
		struct spi_ioc_transfer tr = { (uint64_t)&data[0], (uint64_t)&data[0], (uint32_t)data.size(), (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };
		if(!ioctl(_fileDescriptor, SPI_IOC_MESSAGE(1), &tr)) throw(Exception("Couldn't write to device " + _rfDevice));
		_sendMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_sendMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_sendMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_sendMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::startListening()
{
	try
	{
		stopListening();
		openDevice();
		if(_fileDescriptor == -1) throw(Exception("Couldn't listen to rf device, because the file descriptor is not valid: " + _rfDevice));
		_stopped = false;

		//Test
		std::vector<uint8_t> data;
		data.push_back(0x36);
		readwrite(data);

		std::cout << std::hex << std::setw(2) << std::uppercase << std::setfill('0') << (int32_t)data.at(0) << std::endl;

		closeDevice();
		exit(0);
		//End Test

		_listenThread = std::thread(&TICC1100::listen, this);
	}
    catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::stopListening()
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


			closeDevice();
		}
		_stopped = true;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::listen()
{
    try
    {
        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(200));
        		continue;
        	}


        }
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

}
#endif /* TI_CC1100 */
