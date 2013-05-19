#ifndef HM_RC_SEC3_B_H
#define HM_RC_SEC3_B_H

using namespace std;

#include "HomeMaticDevice.h"
#include "Cul.h"

class HM_RC_Sec3_B : public HomeMaticDevice
{
    public:
        /** Default constructor */
        HM_RC_Sec3_B(std::string, int32_t, unsigned char, unsigned char, unsigned char);
        /** Default destructor */
        virtual ~HM_RC_Sec3_B();
        void packetReceived(BidCoSPacket*);
        void pressButtonInt();
        void pressButtonExt();
        void pressButtonUnscharf();
        int32_t controlByteCounter = 0b10000000;
    protected:
    private:
        unsigned char _buttonIntCounter;
        unsigned char _buttonExtCounter;
        unsigned char _buttonUnscharfCounter;
};

#endif // HM_RC_SEC3_B_H
