#ifndef BIDCOSPACKET_H
#define BIDCOSPACKET_H

using namespace std;

#include "Exception.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

class BidCoSPacket
{
    public:
        //Properties
        uint8_t length();
        uint8_t messageCounter();
        uint8_t controlByte();
        uint8_t messageType();
        int32_t messageSubtype();
        int32_t senderAddress();
        int32_t destinationAddress();
        int32_t channel();
        std::vector<uint8_t>* payload();
        std::string hexString();

        /** Default constructor */
        BidCoSPacket();
        BidCoSPacket(std::string);
        BidCoSPacket(uint8_t, uint8_t, uint8_t, int32_t, int32_t, std::vector<uint8_t>);
        /** Default destructor */
        virtual ~BidCoSPacket();
        void import(std::string);
    protected:
    private:
        uint8_t _length;
        uint8_t _messageCounter;
        uint8_t _controlByte;
        uint8_t _messageType;
        uint32_t _senderAddress;
        uint32_t _destinationAddress;
        std::vector<uint8_t> _payload;

        uint8_t getByte(std::string);
        int32_t getInt(std::string);
};

#endif // BIDCOSPACKET_H
