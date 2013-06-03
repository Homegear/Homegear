#include "HM-RC-Sec3-B.h"

HM_RC_Sec3_B::HM_RC_Sec3_B(std::string serialNumber, int32_t address, unsigned char buttonIntCounter, unsigned char buttonExtCounter, unsigned char buttonUnscharfCounter) : HomeMaticDevice(serialNumber, address), _buttonIntCounter(buttonIntCounter), _buttonExtCounter(buttonExtCounter), _buttonUnscharfCounter(buttonUnscharfCounter)
{

}

HM_RC_Sec3_B::~HM_RC_Sec3_B()
{
}

void HM_RC_Sec3_B::packetReceived(BidCoSPacket*)
{
}

void HM_RC_Sec3_B::pressButtonInt()
{
    std::vector<uint8_t> payload;
    payload.push_back(1);
    payload.push_back(_buttonIntCounter);
    shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[0], 0b10000100, 0x40, _address, 0, payload));
    sendPacket(packet);
    _buttonIntCounter++;
}

void HM_RC_Sec3_B::pressButtonUnscharf()
{
    std::vector<uint8_t> payload;
    payload.push_back(3);
    payload.push_back(_buttonUnscharfCounter);
    shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[0], controlByteCounter, 0x40, _address, 0x1C6943, payload));
    sendPacket(packet);
    _buttonUnscharfCounter++;
    _messageCounter[0]++;
}
