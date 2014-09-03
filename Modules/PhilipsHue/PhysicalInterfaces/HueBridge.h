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

#ifndef HUEBRIDGE_H
#define HUEBRIDGE_H

#include "../PhilipsHuePacket.h"
#include "IPhilipsHueInterface.h"

#include <istream>

namespace PhilipsHue
{

class HueBridge  : public IPhilipsHueInterface
{
    public:
        HueBridge(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
        virtual ~HueBridge();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        int64_t lastAction() { return _lastAction; }
        virtual bool isOpen() { return _client->connected(); }
        virtual std::vector<std::shared_ptr<PhilipsHuePacket>> getPeerInfo();
    protected:
        int64_t _lastAction = 0;
        std::string _hostname;
        int32_t _port;
        std::unique_ptr<BaseLib::HTTPClient> _client;
        Json::Reader _jsonReader;
        Json::FastWriter _jsonWriter;

        void listen();
        void createUser();
        bool getJson(std::string& jsonString, Json::Value& json);
};

}
#endif
