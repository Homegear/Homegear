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

#ifndef HM_CC_VD_H
#define HM_CC_VD_H

#include "../HomeMaticDevice.h"

namespace BidCoS
{
class HM_CC_VD : public HomeMaticDevice
{
    public:
        HM_CC_VD();
        HM_CC_VD(uint32_t deviceType, std::string, int32_t);
        virtual ~HM_CC_VD();
        void setValveDriveBlocked(bool);
        void setValveDriveLoose(bool);
        void setAdjustingRangeTooSmall(bool);
        std::string handleCLICommand(std::string command);

        void handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
    protected:
        virtual void setUpBidCoSMessages();
        virtual void init();
        void loadVariables();
        void saveVariables();
    private:
        //In table variables
        int32_t _valveState = 0;
        bool _valveDriveBlocked = false;
        bool _valveDriveLoose = false;
        bool _adjustingRangeTooSmall = false;
        //End

        int32_t* _errorPosition = nullptr;
        int32_t* _offsetPosition = nullptr;

        std::shared_ptr<BidCoSPeer> createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>(), bool save = true);
        void reset();

        void handleDutyCyclePacket(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void sendDutyCycleResponse(int32_t destinationAddress, unsigned char oldValveState, unsigned char adjustmentCommand);
        void sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress);

};
}
#endif // HM_CC_VD_H
