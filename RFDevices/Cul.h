#ifndef CUL_H
#define CUL_H

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

#include "RFDevice.h"

namespace RF
{

class Cul  : public RFDevice
{
    public:
        Cul();
        void init(std::string rfDevice);
        virtual ~Cul();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BidCoSPacket> packet, bool CCA = false);
        bool isOpen() { return _fileDescriptor > -1; }
    protected:
        int32_t _fileDescriptor = -1;

        void openDevice();
        void closeDevice();
        void setupDevice();
        void writeToDevice(std::string, bool);
        std::string readFromDevice();
        void listen();
    private:
};

}
#endif // CUL_H
