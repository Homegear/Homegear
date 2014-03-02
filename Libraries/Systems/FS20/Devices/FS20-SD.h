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

#ifndef FS20_SD_H
#define FS20_SD_H

#include "../FS20Device.h"
#include "../../../HelperFunctions/HelperFunctions.h"

#include <list>

namespace FS20
{
enum class FilterType {SenderAddress, DestinationAddress};

class FS20_SD_Filter
{
    public:
        FilterType filterType;
        int32_t filterValue;
};

class FS20_SD : public FS20Device
{
    public:
        FS20_SD();
        FS20_SD(uint32_t deviceType, std::string serialNumber, int32_t address);
        virtual ~FS20_SD();
        bool packetReceived(std::shared_ptr<Packet> packet);
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
        std::list<FS20_SD_Filter> _filters;
        //End

        virtual void init();
};
}
#endif // FS20_SD_H
