/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef PHPEVENTS_H_
#define PHPEVENTS_H_

#ifndef NO_SCRIPTENGINE

#include <homegear-base/BaseLib.h>

namespace Homegear
{

class PhpEvents
{

public:
    class EventData
    {
    public:
        std::string source;
        std::string type;
        uint64_t id = 0;
        int32_t channel = -1;
        int32_t hint = -1;
        std::string variable;
        BaseLib::PVariable value;
    };

    static std::mutex eventsMapMutex;
    static std::map<int32_t, std::shared_ptr<PhpEvents>> eventsMap;

    PhpEvents(std::string& token, std::function<void(std::string output, bool error)>& outputCallback, std::function<BaseLib::PVariable(std::string methodName, BaseLib::PVariable parameters, bool wait)>& rpcCallback);

    virtual ~PhpEvents();

    void stop();

    bool enqueue(std::shared_ptr<EventData>& entry);

    std::shared_ptr<EventData> poll(int32_t timeout = -1);

    void addPeer(uint64_t peerId, int32_t channel, std::string& variable);

    void removePeer(uint64_t peerId, int32_t channel, std::string& variable);

    bool peerSubscribed(uint64_t peerId, int32_t channel, std::string& variable);

    void setLogLevel(int32_t logLevel) { _logLevel = logLevel; }

    int32_t getLogLevel() { return _logLevel; }

    void setPeerId(uint64_t peerId) { _peerId = peerId; }

    uint64_t getPeerId() { return _peerId; }

    void setNodeId(std::string nodeId) { _nodeId = nodeId; }

    std::string getNodeId() { return _nodeId; }

    std::function<void(std::string output, bool error)>& getOutputCallback() { return _outputCallback; };

    std::function<BaseLib::PVariable(std::string methodName, BaseLib::PVariable parameters, bool wait)>& getRpcCallback() { return _rpcCallback; };

    std::string& getToken() { return _token; }

private:
    std::function<void(std::string output, bool error)> _outputCallback;
    std::function<BaseLib::PVariable(std::string methodName, BaseLib::PVariable parameters, bool wait)> _rpcCallback;
    std::string _token;

    // {{{ Data exchange - we are abusing the events object here for data exchange between main thread and sub threads.
    uint64_t _peerId = 0;
    std::string _nodeId;
    int32_t _logLevel = -1;
    // }}}

    std::atomic_bool _stopProcessing;
    static const int32_t _bufferSize = 1000;
    std::mutex _queueMutex;
    int32_t _bufferHead = 0;
    int32_t _bufferTail = 0;
    std::atomic_int _bufferCount;
    std::mutex _bufferMutex;
    std::shared_ptr<EventData> _buffer[_bufferSize];
    std::condition_variable _processingConditionVariable;
    std::mutex _peersMutex;
    std::map<uint64_t, std::map<int32_t, std::set<std::string>>> _peers;
};

}

#endif
#endif
