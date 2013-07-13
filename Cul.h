#ifndef CUL_H
#define CUL_H

#include "HomeMaticDevice.h"
#include "Exception.h"
#include "BidCoSPacket.h"

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

class HomeMaticDevice;

class Cul
{
    public:
        Cul();
        Cul(std::string culDevice);
        void init(std::string culDevice);
        virtual ~Cul();
        void startListening();
        void stopListening();
        void addHomeMaticDevice(HomeMaticDevice*);
        void removeHomeMaticDevice(HomeMaticDevice*);
        void sendPacket(std::shared_ptr<BidCoSPacket> packet);
        bool isOpen() { return _fileDescriptor > -1; }
    protected:
    private:
        std::list<HomeMaticDevice*> _homeMaticDevices;
        int32_t _fileDescriptor = -1;
        std::string _culDevice;
        std::thread _listenThread;
        std::thread _callbackThread;
        bool _stopCallbackThread;
        std::string _lockfile;
        std::mutex _sendMutex;

        void openDevice();
        void closeDevice();
        void setupDevice();
        void writeToDevice(std::string, bool);
        std::string readFromDevice();
        void callCallback(std::shared_ptr<BidCoSPacket> packet);
        void listen();
};

#endif // CUL_H
