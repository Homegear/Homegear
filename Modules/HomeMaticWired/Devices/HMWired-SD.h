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

#ifndef HMWIRED_SD_H
#define HMWIRED_SD_H

#include "../HMWiredDevice.h"

#include <list>

namespace HMWired
{
enum class FilterType {SenderAddress, DestinationAddress};

class HMWired_SD_Filter
{
    public:
        FilterType filterType;
        int32_t filterValue;
};

class HMWired_SD_OverwriteResponse
{
    public:
        int32_t sendAfter;
        std::string packetPartToCapture;
        std::string response;
};

class HMWired_SD : public HMWiredDevice
{
    public:
        HMWired_SD(IDeviceEventSink* eventHandler);
        HMWired_SD(uint32_t deviceType, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
        virtual ~HMWired_SD();
        virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet);
        void addFilter(FilterType, int32_t);
        void removeFilter(FilterType, int32_t);
        std::string handleCLICommand(std::string command);
        void loadVariables();
        void saveVariables();
        void saveFilters();
        void serializeFilters(std::vector<uint8_t>& encodedData);
        void unserializeFilters(std::shared_ptr<std::vector<char>> serializedData);
    protected:
    private:
        //In table variables
        bool _enabled = true;
        std::list<HMWired_SD_Filter> _filters;
        //End

        virtual void init();
};
}
#endif // HMWIRED_SD_H
