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

#ifdef SPIINTERFACES
#include "TICC1100.h"
#include "../MAXPacket.h"
#include "../GD.h"

namespace MAX
{

TICC1100::TICC1100(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IPhysicalInterface(GD::bl, settings)
{
	try
	{
		_out.init(GD::bl);
		_out.setPrefix(GD::out.getPrefix() + "TI CC110X \"" + settings->id + "\": ");

		if(settings->listenThreadPriority == -1)
		{
			settings->listenThreadPriority = 45;
			settings->listenThreadPolicy = SCHED_FIFO;
		}
		if(settings->oscillatorFrequency < 0) settings->oscillatorFrequency = 26000000;
		if(settings->txPowerSetting < 0) settings->txPowerSetting = 0xC0;
		if(settings->interruptPin != 0 && settings->interruptPin != 2)
		{
			if(settings->interruptPin > 0) _out.printWarning("Warning: Setting for interruptPin for device CC1100 in physicalinterfaces.conf is invalid.");
			settings->interruptPin = 2;
		}

		_transfer =  { (uint64_t)0, (uint64_t)0, (uint32_t)0, (uint32_t)4000000, (uint16_t)0, (uint8_t)8, (uint8_t)0, (uint32_t)0 };

		setConfig();
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

TICC1100::~TICC1100()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		closeDevice();
		closeGPIO(1);
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

void TICC1100::setConfig()
{
	if(_settings->oscillatorFrequency == 26000000)
	{
		_config =
		{
			(_settings->interruptPin == 2) ? (uint8_t)0x46 : (uint8_t)0x5B, //00: IOCFG2 (GDO2_CFG)
			0x2E, //01: IOCFG1 (GDO1_CFG to High impedance (3-state))
			(_settings->interruptPin == 0) ? (uint8_t)0x46 : (uint8_t)0x5B, //02: IOCFG0 (GDO0_CFG)
			0x07, //03: FIFOTHR (FIFO threshold to 33 (TX) and 32 (RX)
			0xC6, //04: SYNC1
			0x26, //05: SYNC0
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
			0x30, //17: MCSM1: IDLE when packet has been received, RX after sending
			0x18, //18: MCSM0
			0x16, //19: FOCCFG
			0x6C, //1A: BSCFG
			0x03, //1B: AGCCTRL2
			0x40, //1C: AGCCTRL1
			0x91, //1D: AGCCTRL0
			0x87, //1E: WOREVT1
			0x6B, //1F: WOREVT0
			0xF8, //20: WORCRTL
			0x56, //21: FREND1
			0x10, //22: FREND0
			0xE9, //23: FSCAL3
			0x2A, //24: FSCAL2
			0x00, //25: FSCAL1
			0x1F, //26: FSCAL0
			0x41, //27: RCCTRL1
			0x00, //28: RCCTRL0
		};
	}
	else if(_settings->oscillatorFrequency == 27000000)
	{
		_config =
		{
			(_settings->interruptPin == 2) ? (uint8_t)0x46 : (uint8_t)0x5B, //00: IOCFG2 (GDO2_CFG)
			0x2E, //01: IOCFG1 (GDO1_CFG to High impedance (3-state))
			(_settings->interruptPin == 0) ? (uint8_t)0x46 : (uint8_t)0x5B, //02: IOCFG0 (GDO0_CFG)
			0x07, //03: FIFOTHR (FIFO threshold to 33 (TX) and 32 (RX)
			0xC6, //04: SYNC1
			0x26, //05: SYNC0
			0xFF, //06: PKTLEN (Maximum packet length)
			0x0C, //07: PKTCTRL1: CRC_AUTOFLUSH | APPEND_STATUS | NO_ADDR_CHECK
			0x45, //08: PKTCTRL0
			0x00, //09: ADDR
			0x00, //0A: CHANNR
			0x06, //0B: FSCTRL1
			0x00, //0C: FSCTRL0
			0x20, //0D: FREQ2
			0x28, //0E: FREQ1
			0xC5, //0F: FREQ0
			0xC8, //10: MDMCFG4
			0x84, //11: MDMCFG3
			0x03, //12: MDMCFG2
			0x22, //13: MDMCFG1
			0xE5, //14: MDMCFG0
			0x34, //15: DEVIATN
			0x07, //16: MCSM2
			0x30, //17: MCSM1: IDLE when packet has been received, RX after sending
			0x18, //18: MCSM0
			0x16, //19: FOCCFG
			0x6C, //1A: BSCFG
			0x03, //1B: AGCCTRL2
			0x40, //1C: AGCCTRL1
			0x91, //1D: AGCCTRL0
			0x87, //1E: WOREVT1
			0x6B, //1F: WOREVT0
			0xF8, //20: WORCRTL
			0x56, //21: FREND1
			0x10, //22: FREND0
			0xE9, //23: FSCAL3
			0x2A, //24: FSCAL2
			0x00, //25: FSCAL1
			0x1F, //26: FSCAL0
			0x41, //27: RCCTRL1
			0x00, //28: RCCTRL0
		};
	}
	else _out.printError("Error: Unknown value for \"oscillatorFrequency\" in physicalinterfaces.conf. Valid values are 26000000 and 27000000.");
}

void TICC1100::openDevice()
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
				_out.printCritical("Rf device is in use: " + _settings->device);
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

		_fileDescriptor = _bl->fileDescriptorManager.add(open(_settings->device.c_str(), O_RDWR | O_NONBLOCK));
		usleep(1000);

		if(_fileDescriptor->descriptor == -1)
		{
			_out.printCritical("Couldn't open rf device \"" + _settings->device + "\": " + strerror(errno));
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

void TICC1100::closeDevice()
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

void TICC1100::setup(int32_t userID, int32_t groupID)
{
	try
	{
		_out.printDebug("Debug: CC1100: Setting device permissions");
		setDevicePermission(userID, groupID);
		_out.printDebug("Debug: CC1100: Exporting GPIO");
		exportGPIO(1);
		_out.printDebug("Debug: CC1100: Setting GPIO permissions");
		setGPIOPermission(1, userID, groupID, true);
		_out.printDebug("Debug: CC1100: Setting GPIO direction");
		setGPIODirection(1, GPIODirection::IN);
		_out.printDebug("Debug: CC1100: Settings GPIO edge");
		setGPIOEdge(1, GPIOEdge::BOTH);
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

void TICC1100::setupDevice()
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;

		uint8_t mode = 0;
		uint8_t bits = 8;
		uint32_t speed = 4000000; //4MHz, see page 25 in datasheet

		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_WR_MODE, &mode)) throw(BaseLib::Exception("Couldn't set spi mode on device " + _settings->device));
		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_RD_MODE, &mode)) throw(BaseLib::Exception("Couldn't get spi mode off device " + _settings->device));

		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_WR_BITS_PER_WORD, &bits)) throw(BaseLib::Exception("Couldn't set bits per word on device " + _settings->device));
		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_RD_BITS_PER_WORD, &bits)) throw(BaseLib::Exception("Couldn't get bits per word off device " + _settings->device));

		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_WR_MAX_SPEED_HZ, &speed)) throw(BaseLib::Exception("Couldn't set speed on device " + _settings->device));
		if(ioctl(_fileDescriptor->descriptor, SPI_IOC_RD_MAX_SPEED_HZ, &speed)) throw(BaseLib::Exception("Couldn't get speed off device " + _settings->device));
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

void TICC1100::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_fileDescriptor->descriptor == -1 || _gpioDescriptors[1]->descriptor == -1 || _stopped) return;
		if(packet->payload()->size() > 54)
		{
			_out.printError("Error: Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}
		std::shared_ptr<MAXPacket> maxPacket(std::dynamic_pointer_cast<MAXPacket>(packet));
		if(!maxPacket) return;
		std::vector<uint8_t> packetBytes = maxPacket->byteArray();

		int64_t timeBeforeLock = BaseLib::HelperFunctions::getTime();
		_sendingPending = true;
		_txMutex.lock();
		_sendingPending = false;
		if(_stopCallbackThread || _fileDescriptor->descriptor == -1 || _gpioDescriptors[1]->descriptor == -1 || _stopped)
		{
			_txMutex.unlock();
			return;
		}
		_sending = true;
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		sendCommandStrobe(CommandStrobes::Enum::SFTX);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
		if(_lastPacketSent - timeBeforeLock > 100)
		{
			_out.printWarning("Warning: You're sending too many packets at once. Sending MAX! packets takes a looong time!");
		}
		if(maxPacket->getBurst())
		{
			sendCommandStrobe(CommandStrobes::Enum::STX);
			usleep(1000000);
		}
		writeRegisters(Registers::Enum::FIFO, packetBytes);
		if(!maxPacket->getBurst()) sendCommandStrobe(CommandStrobes::Enum::STX);

		if(_bl->debugLevel > 3)
		{
			if(packet->timeSending() > 0)
			{
				_out.printInfo("Info: Sending (" + _settings->id + ", WOR: " + (maxPacket->getBurst() ? "yes" : "no") + "): " + packet->hexString() + " Planned sending time: " + BaseLib::HelperFunctions::getTimeString(packet->timeSending()));
			}
			else
			{
				_out.printInfo("Info: Sending (" + _settings->id + ", WOR: " + (maxPacket->getBurst() ? "yes" : "no") + "): " + packet->hexString());
			}
		}

		//Unlocking of _txMutex takes place in mainThread
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

void TICC1100::readwrite(std::vector<uint8_t>& data)
{
	try
	{
		_sendMutex.lock();
		_transfer.tx_buf = (uint64_t)&data[0];
		_transfer.rx_buf = (uint64_t)&data[0];
		_transfer.len = (uint32_t)data.size();
		if(_bl->debugLevel >= 6) _out.printDebug("Debug: Sending: " + _bl->hf.getHexString(data));
		if(!ioctl(_fileDescriptor->descriptor, SPI_IOC_MESSAGE(1), &_transfer))
		{
			_sendMutex.unlock();
			_out.printError("Couldn't write to device " + _settings->device + ": " + std::string(strerror(errno)));
			return;
		}
		if(_bl->debugLevel >= 6) _out.printDebug("Debug: Received: " + _bl->hf.getHexString(data));
		_sendMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_sendMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_sendMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_sendMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool TICC1100::checkStatus(uint8_t statusByte, Status::Enum status)
{
	try
	{
		if(_fileDescriptor->descriptor == -1 || _gpioDescriptors[1]->descriptor == -1) return false;
		if((statusByte & (StatusBitmasks::Enum::CHIP_RDYn | StatusBitmasks::Enum::STATE)) != status) return false;
		return true;
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
    return false;
}

uint8_t TICC1100::readRegister(Registers::Enum registerAddress)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return 0;
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
    return 0;
}

std::vector<uint8_t> TICC1100::readRegisters(Registers::Enum startAddress, uint8_t count)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return std::vector<uint8_t>();
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
    return std::vector<uint8_t>();
}

uint8_t TICC1100::writeRegister(Registers::Enum registerAddress, uint8_t value, bool check)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return 0xFF;
		std::vector<uint8_t> data({(uint8_t)registerAddress, value});
		readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn) || (data.at(1) & StatusBitmasks::Enum::CHIP_RDYn)) throw BaseLib::Exception("Error writing to register " + std::to_string(registerAddress) + ".");

		if(check)
		{
			data.at(0) = registerAddress | RegisterBitmasks::Enum::READ_SINGLE;
			data.at(1) = 0;
			readwrite(data);
			if(data.at(1) != value) throw BaseLib::Exception("Error (check) writing to register " + std::to_string(registerAddress) + ".");
		}
		return data.at(0);
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
    return 0;
}

void TICC1100::writeRegisters(Registers::Enum startAddress, std::vector<uint8_t>& values)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;
		std::vector<uint8_t> data({(uint8_t)(startAddress | RegisterBitmasks::Enum::WRITE_BURST) });
		data.insert(data.end(), values.begin(), values.end());
		readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) throw BaseLib::Exception("Error writing to registers " + std::to_string(startAddress) + ".");
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

uint8_t TICC1100::sendCommandStrobe(CommandStrobes::Enum commandStrobe)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return 0xFF;
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
    return 0;
}

void TICC1100::enableRX(bool flushRXFIFO)
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;
		_txMutex.lock();
		if(flushRXFIFO) sendCommandStrobe(CommandStrobes::Enum::SFRX);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
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
    _txMutex.unlock();
}

void TICC1100::initChip()
{
	try
	{
		if(_fileDescriptor->descriptor == -1)
		{
			_out.printError("Error: Could not initialize TI CC1100. The spi device's file descriptor is not valid.");
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
		writeRegister(Registers::Enum::PATABLE, _settings->txPowerSetting, true);

		sendCommandStrobe(CommandStrobes::Enum::SFRX);
		usleep(20);

		enableRX(true);
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

void TICC1100::reset()
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return;
		sendCommandStrobe(CommandStrobes::Enum::SRES);

		usleep(70); //Measured on HM-CC-VD
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

bool TICC1100::crcOK()
{
	try
	{
		if(_fileDescriptor->descriptor == -1) return false;
		std::vector<uint8_t> result = readRegisters(Registers::Enum::LQI, 1);
		if((result.size() == 2) && (result.at(1) & 0x80)) return true;
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
    return false;
}

void TICC1100::startListening()
{
	try
	{
		stopListening();
		openGPIO(1, true);
		if(!_gpioDescriptors[1] || _gpioDescriptors[1]->descriptor == -1) throw(BaseLib::Exception("Couldn't listen to rf device, because the gpio pointer is not valid: " + _settings->device));
		openDevice();
		if(!_fileDescriptor || _fileDescriptor->descriptor == -1) return;
		_stopped = false;

		initChip();

		_firstPacket = true;
		_stopCallbackThread = false;
		_listenThread = std::thread(&TICC1100::mainThread, this);
		BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		IPhysicalInterface::startListening();

		//For sniffing update packets
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		//enableUpdateMode();
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
		if(_fileDescriptor->descriptor != -1) closeDevice();
		closeGPIO(1);
		_stopped = true;
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

void TICC1100::endSending()
{
	try
	{
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		sendCommandStrobe(CommandStrobes::Enum::SFRX);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
		_sending = false;
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
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

void TICC1100::mainThread()
{
    try
    {
		int32_t pollResult;
		int32_t bytesRead;
		std::vector<char> readBuffer({'0'});

        while(!_stopCallbackThread && _fileDescriptor->descriptor > -1 && _gpioDescriptors[1]->descriptor > -1)
        {
        	if(_stopped)
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(200));
        		continue;
        	}
        	pollfd pollstruct {
				(int)_gpioDescriptors[1]->descriptor,
				(short)(POLLPRI | POLLERR),
				(short)0
			};

			pollResult = poll(&pollstruct, 1, 100);
			/*if(pollstruct.revents & POLLERR)
			{
				_out.printWarning("Warning: Error polling GPIO. Reopening...");
				closeGPIO();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				openGPIO(_settings->gpio1);
			}*/
			if(pollResult > 0)
			{
				if(lseek(_gpioDescriptors[1]->descriptor, 0, SEEK_SET) == -1) throw BaseLib::Exception("Could not poll gpio: " + std::string(strerror(errno)));
				bytesRead = read(_gpioDescriptors[1]->descriptor, &readBuffer[0], 1);
				if(!bytesRead) continue;
				if(readBuffer.at(0) == 0x30)
				{
					if(!_sending) _txMutex.try_lock(); //We are receiving, don't send now
					continue; //Packet is being received. Wait for GDO high
				}
				if(_sending) endSending();
				else
				{
					//sendCommandStrobe(CommandStrobes::Enum::SIDLE);
					std::shared_ptr<MAXPacket> packet;
					if(crcOK())
					{
						uint8_t firstByte = readRegister(Registers::Enum::FIFO);
						std::vector<uint8_t> packetBytes = readRegisters(Registers::Enum::FIFO, firstByte + 1); //Read packet + RSSI
						packetBytes[0] = firstByte;
						if(packetBytes.size() >= 9) packet.reset(new MAXPacket(packetBytes, true, BaseLib::HelperFunctions::getTime()));
						else _out.printWarning("Warning: Too small packet received: " + BaseLib::HelperFunctions::getHexString(packetBytes));
					}
					else _out.printDebug("Debug: MAX! packet received, but CRC failed.");
					if(!_sendingPending)
					{
						sendCommandStrobe(CommandStrobes::Enum::SFRX);
						sendCommandStrobe(CommandStrobes::Enum::SRX);
					}
					if(packet)
					{
						if(_firstPacket) _firstPacket = false;
						else raisePacketReceived(packet);
					}
				}
				_txMutex.unlock(); //Packet sent or received, now we can send again
			}
			else if(pollResult < 0)
			{
				_txMutex.unlock();
				_out.printError("Error: Could not poll gpio: " + std::string(strerror(errno)) + ". Reopening...");
				closeGPIO(1);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				openGPIO(1, true);
			}
			//pollResult == 0 is timeout
        }
    }
    catch(const std::exception& ex)
    {
    	_txMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_txMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_txMutex.unlock();
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    try
    {
		if(!_stopCallbackThread && (_fileDescriptor->descriptor == -1 || _gpioDescriptors[1]->descriptor == -1))
		{
			_out.printError("Connection to TI CC1101 closed inexpectedly... Trying to reconnect...");
			_stopCallbackThread = true; //Set to true, so that sendPacket aborts
			_txMutex.unlock(); //Make sure _txMutex is unlocked
			std::thread thread(&TICC1100::startListening, this);
			thread.detach();
		}
		else _txMutex.unlock(); //Make sure _txMutex is unlocked
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
#endif
