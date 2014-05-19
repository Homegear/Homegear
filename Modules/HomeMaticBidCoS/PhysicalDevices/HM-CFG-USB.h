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

#ifndef HM_CFG_USB_H
#define HM_CFG_USB_H

#include "../../Base/BaseLib.h"

//#include <libusb-1.0/libusb.h>

namespace BidCoS
{

class HM_CFG_USB  : public BaseLib::Systems::PhysicalDevice
{
    public:
		HM_CFG_USB(std::shared_ptr<BaseLib::Systems::PhysicalDeviceSettings> settings);
        virtual ~HM_CFG_USB();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        virtual void setup(int32_t userID, int32_t groupID);
        void enableUpdateMode();
        void disableUpdateMode();
    protected:
        void openDevice();
        void closeDevice();
        void writeToDevice(std::string, bool);
        std::string readFromDevice();
        void listen();
    private:
};

}
#endif // CUL_H
