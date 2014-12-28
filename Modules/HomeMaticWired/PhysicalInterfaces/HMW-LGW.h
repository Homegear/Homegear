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

#ifndef HMW_LGW_H
#define HMW_LGW_H

#include "../HMWiredPacket.h"
#include "IHMWiredInterface.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include <gcrypt.h>

namespace HMWired
{

class HMW_LGW  : public IHMWiredInterface
{
    public:
        HMW_LGW(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
        virtual ~HMW_LGW();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        int64_t lastAction() { return _lastAction; }
        virtual bool isOpen() { return _initComplete && _socket->connected(); }

        virtual bool autoResend() { return true; }
        virtual void search(std::vector<int32_t>& foundDevices);
    protected:
        class Request
        {
        public:
        	std::mutex mutex;
        	std::condition_variable conditionVariable;
        	bool mutexReady = false;
        	std::vector<uint8_t> response;
        	uint8_t getResponseType() { return _responseType; }

        	Request(uint8_t responseType) { _responseType = responseType; };
        	virtual ~Request() {};
        private:
        	uint8_t _responseType;
        };

        BaseLib::Math _math;
        int64_t _lastAction = 0;
        std::string _hostname;
        std::string _port;
        std::unique_ptr<BaseLib::SocketOperations> _socket;
        std::mutex _requestsMutex;
        std::map<uint8_t, std::shared_ptr<Request>> _requests;
        std::mutex _sendMutex;
        bool _initComplete = false;
        int32_t _lastKeepAlive = 0;
        int32_t _lastKeepAliveResponse = 0;
        int32_t _lastTimePacket = 0;
        int64_t _startUpTime = 0;
        int32_t _myAddress = 1;
        std::vector<uint8_t> _packetBuffer;
        uint8_t _packetIndex = 0;
        bool _searchFinished = false;
        std::vector<int32_t> _searchResult;

        //AES stuff
        bool _aesInitialized = false;
        bool _aesExchangeComplete = false;
        std::vector<uint8_t> _rfKey;
        std::vector<uint8_t> _oldRFKey;
        std::vector<uint8_t> _key;
		std::vector<uint8_t> _remoteIV;
		std::vector<uint8_t> _myIV;
        gcry_cipher_hd_t _encryptHandle = nullptr;
        gcry_cipher_hd_t _decryptHandle = nullptr;

        std::vector<char> encrypt(const std::vector<char>& data);
        std::vector<uint8_t> decrypt(std::vector<uint8_t>& data);
        bool aesKeyExchange(std::vector<uint8_t>& data);
        bool aesInit();
        void aesCleanup();
        //End AES stuff

        void reconnect();
        void processData(std::vector<uint8_t>& data);
        void processPacket(std::vector<uint8_t>& packet);
        void parsePacket(std::vector<uint8_t>& packet);
        void buildPacket(std::vector<char>& packet, const std::vector<char>& payload);
        void escapePacket(const std::vector<char>& unescapedPacket, std::vector<char>& escapedPacket);
        void getResponse(const std::vector<char>& packet, std::vector<uint8_t>& response, uint8_t messageCounter, uint8_t responseType);
        void send(std::string hexString, bool raw = false);
        void send(const std::vector<char>& data, bool raw);
        void sendKeepAlivePacket();
        void listen();
        void getFileDescriptor(bool& timedout);
        std::shared_ptr<BaseLib::FileDescriptor> getConnection(std::string& hostname, const std::string& port, std::string& ipAddress);
    private:
};

}
#endif
