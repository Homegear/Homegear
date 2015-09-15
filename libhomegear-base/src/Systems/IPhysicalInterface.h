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

#ifndef IPHYSICALINTERFACE_H_
#define IPHYSICALINTERFACE_H_

#include "../IEvents.h"
#include "PhysicalInterfaceSettings.h"
#include "../Managers/FileDescriptorManager.h"

#include <list>
#include <thread>
#include <mutex>
#include <map>
#include <condition_variable>

#include <dirent.h>

namespace BaseLib
{

class Obj;

namespace Systems
{

class Packet;

class IPhysicalInterface : public IEventsEx
{
public:
	struct GPIODirection
	{
			enum Enum
			{
					IN,
					OUT
			};
	};

	struct GPIOEdge
	{
			enum Enum
			{
					RISING,
					FALLING,
					BOTH
			};
	};

	//Event handling
	class IPhysicalInterfaceEventSink : public IEventSinkBase
	{
	public:
		virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<Packet> packet) = 0;
	};
	//End event handling

	IPhysicalInterface(BaseLib::Obj* baseLib);
	IPhysicalInterface(BaseLib::Obj* baseLib, std::shared_ptr<PhysicalInterfaceSettings> settings);

	virtual ~IPhysicalInterface();

	virtual void startListening();
	virtual void stopListening();
	virtual void enableUpdateMode();
	virtual void disableUpdateMode();
	virtual void sendPacket(std::shared_ptr<Packet> packet) = 0;
	virtual bool isOpen() { return _fileDescriptor && _fileDescriptor->descriptor > -1; }
	virtual uint32_t responseDelay() { return _settings->responseDelay; }
	virtual int64_t lastPacketSent() { return _lastPacketSent; }
	virtual int64_t lastPacketReceived() { return _lastPacketReceived; }
	virtual void setup(int32_t userID, int32_t groupID) {}
	virtual std::string getType() { return _settings->type; }
	virtual std::string getID() { return _settings->id; }
	virtual bool isDefault() { return _settings->isDefault; }
	virtual bool isNetworkDevice() { return _settings->device.empty() && !_settings->host.empty() && !_settings->port.empty(); }
protected:
	BaseLib::Obj* _bl = nullptr;
	std::shared_ptr<PhysicalInterfaceSettings> _settings;
	std::thread _listenThread;
	std::thread _callbackThread;
	bool _stopCallbackThread = false;
	static const int32_t _packetBufferSize = 1000;
	std::mutex _packetBufferMutex;
	int32_t _packetBufferHead = 0;
	int32_t _packetBufferTail = 0;
	std::shared_ptr<Packet> _packetBuffer[_packetBufferSize];
	std::mutex _packetProcessingThreadMutex;
	std::thread _packetProcessingThread;
	bool _packetProcessingPacketAvailable = false;
	std::condition_variable _packetProcessingConditionVariable;
	bool _stopPacketProcessingThread = false;
	std::string _lockfile;
	std::mutex _sendMutex;
	bool _stopped = false;
	std::shared_ptr<FileDescriptor> _fileDescriptor;
	std::map<uint32_t, std::shared_ptr<FileDescriptor>> _gpioDescriptors;
	int64_t _lastPacketSent = -1;
	int64_t _lastPacketReceived = -1;
	int64_t _maxPacketProcessingTime = 1000;
	bool _updateMode = false;

	//Event handling
	virtual void raisePacketReceived(std::shared_ptr<Packet> packet);
	//End event handling
	void processPackets();
	virtual void setDevicePermission(int32_t userID, int32_t groupID);
	virtual void openGPIO(uint32_t index, bool readOnly);
	virtual void getGPIOPath(uint32_t index);
	virtual void closeGPIO(uint32_t index);
	virtual bool getGPIO(uint32_t index);
	virtual void setGPIO(uint32_t index, bool value);
	virtual void setGPIOPermission(uint32_t index, int32_t userID, int32_t groupID, bool readOnly);
	virtual void exportGPIO(uint32_t index);
	virtual void setGPIODirection(uint32_t index, GPIODirection::Enum direction);
	virtual void setGPIOEdge(uint32_t index, GPIOEdge::Enum edge);
	virtual bool gpioDefined(uint32_t);
	virtual bool gpioOpen(uint32_t);
};

}
}
#endif
