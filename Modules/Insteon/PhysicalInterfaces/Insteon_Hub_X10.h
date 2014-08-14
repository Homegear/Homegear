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

#ifndef INSTEONHUBX10_H
#define INSTEONHUBX10_H

#include "../InsteonPacket.h"
#include "IInsteonInterface.h"
#include "../../Base/BaseLib.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <set>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

namespace Insteon
{

class InsteonHubX10  : public IInsteonInterface
{
    public:
        InsteonHubX10(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
        virtual ~InsteonHubX10();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        int64_t lastAction() { return _lastAction; }
        virtual bool isOpen() { return _socket->connected(); }

        virtual void addPeer(int32_t address);
		virtual void addPeers(std::vector<int32_t>& addresses);
		virtual void removePeer(int32_t address);

        virtual void enablePairingMode();
        virtual void disablePairingMode();
    protected:
        class Request
        {
        public:
        	std::timed_mutex mutex;
        	std::vector<uint8_t> response;
        	uint8_t getResponseType() { return _responseType; }

        	Request(uint8_t responseType) { _responseType = responseType; };
        	virtual ~Request() {};
        private:
        	uint8_t _responseType;
        };

        std::thread _initThread;
        std::mutex _peersMutex;
        std::map<int32_t, PeerInfo> _peers;
        std::set<int32_t> _pairedPeers;
        std::set<int32_t> _usedDatabaseAddresses;
        int64_t _lastAction = 0;
        std::string _hostname;
        std::string _port;
        std::unique_ptr<BaseLib::SocketOperations> _socket;
        std::mutex _requestMutex;
        std::shared_ptr<Request> _request;
        std::mutex _sendMutex;
        bool _initStarted = false;
        bool _initComplete = false;
        int32_t _centralAddress = 0xFFFFFF;
        std::map<int32_t, int32_t> _lengthLookup;
        const uint32_t _maxLinks = 415;

        void reconnect();
        void doInit();
        bool processData(std::vector<uint8_t>& data);
        void processPacket(std::vector<uint8_t>& packet);
        void getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t responseType);
        void send(const std::vector<char>& packet, bool printPacket);
        void listen();
        void getFileDescriptor(bool& timedout);
        std::shared_ptr<BaseLib::FileDescriptor> getConnection(std::string& hostname, const std::string& port, std::string& ipAddress);
        void checkPeers();
        void storePeer(PeerInfo& peerInfo);
        int32_t getFreeDatabaseAddress();
};

}
#endif
