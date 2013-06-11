#ifndef HM_SD_H
#define HM_SD_H

#include "../HomeMaticDevice.h"
#include "../Cul.h"

enum class FilterType {SenderAddress, DestinationAddress, DeviceType, MessageType};

class HM_SD_Filter
{
    public:
        FilterType filterType;
        int32_t filterValue;
};

class HM_SD_OverwriteResponse
{
    public:
        int32_t sendAfter;
        std::string packetPartToCapture;
        std::string response;
};

class HM_SD : public HomeMaticDevice
{
    public:
        HM_SD();
        HM_SD(std::string serialNumber, int32_t address);
        virtual ~HM_SD();
        void packetReceived(std::shared_ptr<BidCoSPacket> packet);
        void addFilter(FilterType, int32_t);
        void removeFilter(FilterType, int32_t);
        void addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter);
        void removeOverwriteResponse(std::string packetPartToCapture);
        void handleCLICommand(std::string command);
        std::string serialize();
        void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
    private:
        std::list<HM_SD_Filter> _filters;
        std::list<HM_SD_OverwriteResponse> _responsesToOverwrite;

        void init();
};

#endif // HM_SD_H
