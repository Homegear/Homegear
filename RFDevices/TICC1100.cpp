//TODO REMOVE AFTER TESTING
#define TI_CC1100

#ifdef TI_CC1100

#include "TICC1100.h"

namespace RF
{

TICC1100::TICC1100()
{
	_transfer =  { (uint64_t)0, (uint64_t)0, (uint32_t)0, (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };

	_config =
	{
		0x06, //00: IOCFG2 (GDO2_CFG)
		0x2E, //01: IOCFG1 (GDO1_CFG to High impedance (3-state))
		0x05, //02: IOCFG0 (GDO0_CFG, GDO0 is not connected)
		0x07, //03: FIFOTHR (FIFO threshold to 33 (TX) and 32 (RX)
		0xE9, //04: SYNC1
		0xCA, //05: SYNC0
		0xFF, //06: PKTLEN (Maximum packet length)
		0x0C, //07: PKTCTRL1: CRC_AUTOFLUSH | APPEND_STATUS | NO_ADDR_CHECK
		0x45, //08: PKTCTRL0
		0x00, //09: ADDR
		0x00, //0A: CHANNR
		0x06, //0B: FSCTRL1
		0x00, //0C: FSCTRL0
		0x21, //0D: FREQ2
		0x65, //0E: FREQ1
		0x6A, //0F: FREQ0
		0xC8, //10: MDMCFG4
		0x93, //11: MDMCFG3
		0x03, //12: MDMCFG2
		0x22, //13: MDMCFG1
		0xB9, //14: MDMCFG0
		0x34, //15: DEVIATN
		0x1C, //16: MCSM2
		0x03, //17: MCSM1: IDLE when packet has been received, RX after sending
		0x18, //18: MCSM0
		0x16, //19: FOCCFG
		0x6C, //1A: BSCFG
		0x43, //1B: AGCCTRL2
		0x40, //1C: AGCCTRL1
		0x91, //1D: AGCCTRL0
		0x2F, //1E: WOREVT1
		0x6B, //1F: WOREVT0
		0xF8, //20: WORCRTL
		0x56, //21: FREND1
		0x10, //22: FREND0
		0xAC, //23: FSCAL3
		0x0C, //24: FSCAL2
		0x39, //25: FSCAL1
		0x11, //26: FSCAL0
		0x51, //27: RCCTRL1
		0x00, //28: RCCTRL0
	};

	//setupGPIO(17);
	openGPIO(22);
}

TICC1100::~TICC1100()
{
}

void TICC1100::openGPIO(int32_t gpio)
{
	try
	{
		std::string path = "/sys/class/gpio/gpio" + std::to_string(gpio) + "/value";
		_gpioDescriptor = open(path.c_str(), O_RDONLY);
		if (_gpioDescriptor == -1) throw(Exception("Failed to open gpio " + std::to_string(gpio)));
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

void TICC1100::setGPIOMode(int32_t gpio, GPIOModes::Enum mode)
{
	try
	{
		std::string path = "/sys/class/gpio/gpio" + std::to_string(gpio) + "/direction";
		int32_t fd = open(path.c_str(), O_WRONLY);
		if (fd == -1) throw(Exception("Failed to set direction for gpio pin " + std::to_string(gpio)));
		std::string direction((mode == GPIOModes::INPUT) ? "in" : "out");
		if (write(fd, direction.c_str(), direction.size()) == -1) throw(Exception("Failed to write direction for gpio pin " + std::to_string(gpio)));
		close(fd);
		/*if(!_gpio) return;
		*(_gpio + (gpio / 10)) &= ~(7 << ((gpio % 10) * 3));

		if(mode == GPIOModes::Enum::OUTPUT)
		{
			*(_gpio + (gpio / 10)) |=  (1 << ((gpio % 10) * 3));
		}*/
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

void TICC1100::setupGPIO(int32_t gpio)
{
	try
	{

		int32_t fd = open("/sys/class/gpio/export", O_WRONLY);
		if(fd == -1) throw(Exception("Couldn't export gpio pin " + std::to_string(gpio)));
		std::vector<char> buffer;
		std::string temp(std::to_string(gpio));
		buffer.insert(buffer.end(), temp.begin(), temp.end());
		write(fd, &buffer[0], buffer.size());
		close(fd);
		/*int32_t memFD;
		if((memFD = open("/dev/mem", O_RDWR | O_SYNC)) < 0) throw Exception("Can't open /dev/mem.");

		void *gpioMap = mmap(
			NULL,             //Any adddress in our space will do
			BLOCK_SIZE,       //Map length
			PROT_READ | PROT_WRITE,// Enable reading & writting to mapped memory
			MAP_SHARED,       //Shared with other processes
			memFD,           //File to map
			GPIO_BASE         //Offset to GPIO peripheral
		);

		close(memFD); //No need to keep mem_fd open after mmap

		if (gpioMap == MAP_FAILED) throw Exception("Couldn't map GPIO.");

		_gpio = (volatile unsigned *)gpioMap;*/
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
		std::cout << "Writing: " << std::hex << std::uppercase << std::setfill('0');
		for(std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
		{
			std::cout << std::setw(2) << (int32_t)*i << " ";
		}
		std::cout << std::endl;
		_transfer.tx_buf = (uint64_t)&data[0];
		_transfer.rx_buf = (uint64_t)&data[0];
		_transfer.len = (uint32_t)data.size();
		struct spi_ioc_transfer tr = { (uint64_t)&data[0], (uint64_t)&data[0], (uint32_t)data.size(), (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };
		if(!ioctl(_fileDescriptor, SPI_IOC_MESSAGE(1), &tr)) throw(Exception("Couldn't write to device " + _rfDevice));
		std::cout << "Response: " << std::hex << std::uppercase << std::setfill('0');
		for(std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
		{
			std::cout << std::setw(2) << (int32_t)*i << " ";
		}
		std::cout << std::endl;
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

bool TICC1100::checkStatus(uint8_t statusByte, Status::Enum status)
{
	try
	{
		if(_fileDescriptor == -1) return false;
		if((statusByte & (StatusBitmasks::Enum::CHIP_RDYn | StatusBitmasks::Enum::STATE)) != status) return false;
		return true;
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
    return false;
}

uint8_t TICC1100::readRegister(Registers::Enum registerAddress)
{
	try
	{
		if(_fileDescriptor == -1) return 0;
		std::vector<uint8_t> data({(uint8_t)(registerAddress | RegisterBitmasks::Enum::READ_SINGLE), 0x00});
		for(uint32_t i = 0; i < 5; i++)
		{
			readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.at(0) = (uint8_t)(registerAddress  | RegisterBitmasks::Enum::READ_SINGLE);
			data.at(1) = 0;
			usleep(20);
		}
		return data.at(1);
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
    return 0;
}

std::vector<uint8_t> TICC1100::readRegisters(Registers::Enum startAddress, uint8_t count)
{
	try
	{
		if(_fileDescriptor == -1) return std::vector<uint8_t>();
		std::vector<uint8_t> data({(uint8_t)(startAddress | RegisterBitmasks::Enum::READ_BURST)});
		for(uint32_t i = 0; i < count; i++) data.push_back(0);
		for(uint32_t i = 0; i < 5; i++)
		{
			readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.clear();
			data.at(0) = (uint8_t)(startAddress  | RegisterBitmasks::Enum::READ_BURST);
			for(uint32_t i = 0; i < count; i++) data.push_back(0);
			usleep(20);
		}
		return data;
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
    return std::vector<uint8_t>();
}

uint8_t TICC1100::writeRegister(Registers::Enum registerAddress, uint8_t value, bool check)
{
	try
	{
		if(_fileDescriptor == -1) return 0xFF;
		std::vector<uint8_t> data({(uint8_t)registerAddress, value});
		readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn) || (data.at(1) & StatusBitmasks::Enum::CHIP_RDYn)) throw Exception("Error writing to register " + std::to_string(registerAddress) + ".");

		if(check)
		{
			data.at(0) = registerAddress | RegisterBitmasks::Enum::READ_SINGLE;
			data.at(1) = 0;
			readwrite(data);
			if(data.at(1) != value) throw Exception("Error (check) writing to register " + std::to_string(registerAddress) + ".");
		}
		return data.at(0);
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
    return 0;
}

void TICC1100::writeRegisters(Registers::Enum startAddress, std::vector<uint8_t> values)
{
	try
	{
		if(_fileDescriptor == -1) return;
		std::vector<uint8_t> data({(uint8_t)(startAddress | RegisterBitmasks::Enum::WRITE_BURST) });
		data.insert(data.end(), values.begin(), values.end());
		readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) throw Exception("Error writing to registers " + std::to_string(startAddress) + ".");
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

uint8_t TICC1100::sendCommandStrobe(CommandStrobes::Enum commandStrobe)
{
	try
	{
		if(_fileDescriptor == -1) return 0xFF;
		std::vector<uint8_t> data({(uint8_t)commandStrobe});
		for(uint32_t i = 0; i < 5; i++)
		{
			readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.at(0) = (uint8_t)commandStrobe;
			usleep(20);
		}
		return data.at(0);
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
    return 0;
}

void TICC1100::enableTX()
{
	try
	{
		if(_fileDescriptor == -1) return;

		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
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

void TICC1100::enableRX()
{
	try
	{
		if(_fileDescriptor == -1) return;
		sendCommandStrobe(CommandStrobes::Enum::SRX);
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

void TICC1100::startListening()
{
	try
	{
		stopListening();
		if(_gpioDescriptor == -1) throw(Exception("Couldn't listen to rf device, because the gpio pointer is not valid: " + _rfDevice));
		openDevice();
		if(_fileDescriptor == -1) throw(Exception("Couldn't listen to rf device, because the file descriptor is not valid: " + _rfDevice));
		_stopped = false;

		/*std::vector<uint8_t> result = readRegisters(Registers::Enum::IOCFG2, 41);
		for(std::vector<uint8_t>::const_iterator i = result.begin(); i != result.end(); ++i)
		{
			std::cout << std::hex << std::setw(2) << (int32_t)*i << std::endl;
		}

		closeDevice();
		exit(0);*/

		std::vector<uint8_t> data;
		data.push_back(CommandStrobes::Enum::SRES);
		readwrite(data);

		usleep(1000);

		if(!checkStatus(sendCommandStrobe(CommandStrobes::Enum::SIDLE), Status::Enum::IDLE)) throw Exception("Could not enter idle state.");
		writeRegisters(Registers::Enum::IOCFG2, _config);
		writeRegister(Registers::Enum::TEST2, 0x81, true); //Determined by SmartRF Studio
		writeRegister(Registers::Enum::TEST1, 0x35, true); //Determined by SmartRF Studio
		writeRegister(Registers::Enum::PATABLE, 0xC3, true);

		usleep(1000);

		sendCommandStrobe(CommandStrobes::Enum::SCAL);

		usleep(1000);

		enableRX();

		//Test
		struct pollfd pollstruct {
			(int)_gpioDescriptor,
			(short)(POLLPRI | POLLERR),
			(short)0
		};

		int32_t pollResult;
		int32_t bytesRead;
		std::vector<char> readBuffer({'0'});
		while(true)
		{
			pollResult = poll(&pollstruct, 1, 1000);
			if(pollResult > 0)
			{
				if(lseek(_gpioDescriptor, 0, SEEK_SET) == -1) throw Exception("Could not poll gpio: " + std::to_string(errno));
				bytesRead = read(_gpioDescriptor, &readBuffer[0], 1);
				std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "Bla: " << (int32_t)pollResult << " " << (int32_t)readBuffer.at(0) << std::endl;
				if(!bytesRead || readBuffer.at(0) == 0x30) continue;
				usleep(20);
				sendCommandStrobe(CommandStrobes::Enum::SIDLE);
				usleep(20);
				std::vector<uint8_t> result = readRegisters(Registers::Enum::LQI, 1);
				if(!result.empty()) std::cout << "Result: " << (int32_t)result.at(1) << std::endl;
				uint8_t firstByte = readRegister(Registers::Enum::FIFO);
				std::cout << "Result: " << (int32_t)(firstByte & 0x7F) << std::endl;
				usleep(20);
				sendCommandStrobe(CommandStrobes::Enum::SFRX);
				usleep(20);
				sendCommandStrobe(CommandStrobes::Enum::SIDLE);
				usleep(20);
				sendCommandStrobe(CommandStrobes::Enum::SNOP);
				usleep(100000);
				enableRX();
			}
			else if(pollResult < 0)
			{
				throw Exception("Could not poll gpio: " + std::to_string(errno));
			}
			//timeout
			else if(pollResult == 0) continue;
		}

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
