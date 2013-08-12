//TODO REMOVE AFTER TESTING
#define TI_CC1100

#ifdef TI_CC1100

#include "TICC1100.h"
#include "../GD.h"

namespace RF
{

TICC1100::TICC1100()
{
	_transfer =  { (uint64_t)0, (uint64_t)0, (uint32_t)0, (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };

	_config = //Read from HM-CC-VD
	{
		0x46, //00: IOCFG2 (GDO2_CFG)
		0x2E, //01: IOCFG1 (GDO1_CFG to High impedance (3-state))
		0x2E, //02: IOCFG0 (GDO0_CFG, GDO0 is not connected)
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
		0xF8, //14: MDMCFG0
		0x34, //15: DEVIATN
		0x07, //16: MCSM2
		0x00, //17: MCSM1: IDLE when packet has been received, RX after sending
		0x18, //18: MCSM0
		0x16, //19: FOCCFG
		0x6C, //1A: BSCFG
		0x43, //1B: AGCCTRL2
		0x40, //1C: AGCCTRL1
		0x91, //1D: AGCCTRL0
		0x87, //1E: WOREVT1
		0x6B, //1F: WOREVT0
		0xF8, //20: WORCRTL
		0x56, //21: FREND1
		0x10, //22: FREND0
		0xA9, //23: FSCAL3
		0x0A, //24: FSCAL2
		0x00, //25: FSCAL1
		0x11, //26: FSCAL0
		0x41, //27: RCCTRL1
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

void TICC1100::sendPacket(std::shared_ptr<BidCoSPacket> packet, bool CCA)
{
	try
	{
		if(_fileDescriptor == -1) return;
		if(packet->payload()->size() > 54)
		{
			if(GD::debugLevel >= 2) std::cerr << "Tried to send packet larger than 64 bytes. That is not supported." << std::endl;
			return;
		}

		std::vector<uint8_t> decodedPacket = packet->byteArray();
		std::vector<uint8_t> encodedPacket(decodedPacket.size());
		encodedPacket[0] = decodedPacket[0];
		encodedPacket[1] = (~decodedPacket[1]) ^ 0x89;
		uint32_t i = 2;
		for(; i < decodedPacket[0]; i++)
		{
			encodedPacket[i] = (encodedPacket[i - 1] + 0xDC) ^ decodedPacket[i];
		}
		encodedPacket[i] = decodedPacket[i] ^ decodedPacket[2];

		_txMutex.lock();
		_sending = true;
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		if(CCA)
		{
			writeRegister(Registers::Enum::PKTCTRL1, 0x4C);
			writeRegister(Registers::Enum::AGCCTRL1, 0x68);
			writeRegister(Registers::Enum::WOREVT1, 0x2F);
			sendCommandStrobe(CommandStrobes::Enum::SFRX);
			sendCommandStrobe(CommandStrobes::Enum::SRX);

			for(uint32_t i = 0; i < 5500; i++)
			{
				readRegisters(Registers::Enum::PKTSTATUS, 1).at(1);
				usleep(100);
			}

			sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		}
		sendCommandStrobe(CommandStrobes::Enum::SFTX);
		if(CCA)
		{
			sendCommandStrobe(CommandStrobes::Enum::STX);
			usleep(361000);
		}
		writeRegisters(Registers::Enum::FIFO, encodedPacket);
		if(!CCA) sendCommandStrobe(CommandStrobes::Enum::STX);

		if(GD::debugLevel >= 4) std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << " Sending: " << packet->hexString() << std::endl;

		struct pollfd pollstruct {
			(int)_gpioDescriptor,
			(short)(POLLPRI | POLLERR),
			(short)0
		};

		int32_t pollResult;
		int32_t bytesRead;
		std::vector<char> readBuffer({'0'});
		for(uint32_t i = 0; i < 5; i++)
		{
			pollResult = poll(&pollstruct, 1, 50);
			if(pollResult > 0)
			{
				if(lseek(_gpioDescriptor, 0, SEEK_SET) == -1) throw Exception("Could not poll gpio: " + std::to_string(errno));
				bytesRead = read(_gpioDescriptor, &readBuffer[0], 1);
				if(!bytesRead || readBuffer.at(0) == 0x30) continue; //Packet is being received. Wait for GDO high
				if(GD::debugLevel >= 5) std::cout << "Debug: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << ": Packet successfully sent." << std::endl;
				break;
			}
			else if(pollResult < 0)
			{
				throw Exception("Could not poll gpio: " + std::to_string(errno));
			}
			//timeout
			else if(pollResult == 0) continue;
			if(i == 4 && GD::debugLevel >= 3) std::cerr << "Warning: Sending of packet timed out." << std::endl;
		}
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		if(CCA) //Reset registers to original values
		{
			writeRegister(Registers::Enum::PKTCTRL1, 0x0C);
			writeRegister(Registers::Enum::AGCCTRL1, 0x40);
			writeRegister(Registers::Enum::WOREVT1, 0x87);
		}
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
    _sending = false;
	_txMutex.unlock();
}

void TICC1100::readwrite(std::vector<uint8_t>& data)
{
	try
	{
		_sendMutex.lock();
		_transfer.tx_buf = (uint64_t)&data[0];
		_transfer.rx_buf = (uint64_t)&data[0];
		_transfer.len = (uint32_t)data.size();
		/*std::cout << "Sending: " << std::hex << std::setfill('0');
		for(std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
		{
			std::cout << std::setw(2) << (int32_t)*i;
		}
		std::cout << std::dec << std::endl;*/
		if(!ioctl(_fileDescriptor, SPI_IOC_MESSAGE(1), &_transfer)) throw(Exception("Couldn't write to device " + _rfDevice));
		/*std::cout << "Received: " << std::hex << std::setfill('0');
		for(std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
		{
			std::cout << std::setw(2) << (int32_t)*i;
		}
		std::cout << std::dec << std::endl;*/
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
		data.resize(count + 1, 0);
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

void TICC1100::writeRegisters(Registers::Enum startAddress, std::vector<uint8_t>& values)
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

void TICC1100::enableRX(bool flushRXFIFO)
{
	try
	{
		if(_fileDescriptor == -1) return;
		_txMutex.lock();
		if(flushRXFIFO) sendCommandStrobe(CommandStrobes::Enum::SFRX);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
		_txMutex.unlock();
	}
    catch(const std::exception& ex)
    {
    	_txMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_txMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_txMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void TICC1100::initChip()
{
	try
	{
		if(_fileDescriptor == -1)
		{
			if(GD::debugLevel >= 2) std::cerr << "Error: Could not initialize TI CC1100. The spi device's file descriptor is not valid." << std::endl;
			return;
		}
		reset();

		int32_t index = 0;
		for(std::vector<uint8_t>::const_iterator i = _config.begin(); i != _config.end(); ++i)
		{
			writeRegister((Registers::Enum)index, *i, true);
			index++;
		}
		writeRegister(Registers::Enum::FSTEST, 0x59, true);
		writeRegister(Registers::Enum::TEST2, 0x81, true); //Determined by SmartRF Studio
		writeRegister(Registers::Enum::TEST1, 0x35, true); //Determined by SmartRF Studio
		writeRegister(Registers::Enum::PATABLE, 0xC3, true);

		sendCommandStrobe(CommandStrobes::Enum::SFRX);
		usleep(20);

		enableRX(true);
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

void TICC1100::reset()
{
	try
	{
		if(_fileDescriptor == -1) return;
		sendCommandStrobe(CommandStrobes::Enum::SRES);

		usleep(70); //Measured on HM-CC-VD
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

bool TICC1100::crcOK()
{
	try
	{
		if(_fileDescriptor == -1) return false;
		std::vector<uint8_t> result = readRegisters(Registers::Enum::LQI, 1);
		if((result.size() == 2) && (result.at(1) & 0x80)) return true;
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

void TICC1100::startListening()
{
	try
	{
		stopListening();
		if(_gpioDescriptor == -1) throw(Exception("Couldn't listen to rf device, because the gpio pointer is not valid: " + _rfDevice));
		openDevice();
		if(_fileDescriptor == -1) throw(Exception("Couldn't listen to rf device, because the file descriptor is not valid: " + _rfDevice));
		_stopped = false;

		initChip();

		_stopCallbackThread = false;
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
		if(_fileDescriptor != -1) closeDevice();
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
    	struct pollfd pollstruct {
			(int)_gpioDescriptor,
			(short)(POLLPRI | POLLERR),
			(short)0
		};

		int32_t pollResult;
		int32_t bytesRead;
		std::vector<char> readBuffer({'0'});
		bool firstPacket = true;

        while(!_stopCallbackThread)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(200));
        		continue;
        	}

			pollResult = poll(&pollstruct, 1, 1000);
			if(pollResult > 0 && !_sending)
			{
				if(lseek(_gpioDescriptor, 0, SEEK_SET) == -1) throw Exception("Could not poll gpio: " + std::to_string(errno));
				bytesRead = read(_gpioDescriptor, &readBuffer[0], 1);
				if(!bytesRead || readBuffer.at(0) == 0x30) continue; //Packet is being received. Wait for GDO high
				if(crcOK())
				{
					uint8_t firstByte = readRegister(Registers::Enum::FIFO);
					std::vector<uint8_t> encodedData = readRegisters(Registers::Enum::FIFO, firstByte + 1); //Read packet + RSSI
					std::vector<uint8_t> decodedData(encodedData.size());
					if(encodedData.size() >= 9 && encodedData.size() < 40) //Ignore too big packets. The first packet after initializing for example.
					{
						decodedData[0] = firstByte;
						decodedData[1] = (~encodedData[1]) ^ 0x89;
						uint32_t i = 2;
						for(; i < firstByte; i++)
						{
							decodedData[i] = (encodedData[i - 1] + 0xDC) ^ encodedData[i];
						}
						decodedData[i] = encodedData[i] ^ decodedData[2];
						decodedData[i + 1] = encodedData[i + 1]; //RSSI

						std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(decodedData, true));
						std::thread t(&TICC1100::callCallback, this, packet);
						t.detach();
					}
				}

				enableRX(true);
			}
			else if(pollResult < 0)
			{
				throw Exception("Could not poll gpio: " + std::to_string(errno));
			}
			//timeout
			else if(pollResult == 0) continue;
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
