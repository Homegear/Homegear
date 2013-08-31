#ifndef HM_SD_H
#define HM_SD_H

#include "../HomeMaticDevice.h"
#include "../Database.h"

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
        HM_SD(uint32_t deviceID, std::string serialNumber, int32_t address);
        virtual ~HM_SD();
        bool packetReceived(std::shared_ptr<BidCoSPacket> packet);
        void addFilter(FilterType, int32_t);
        void removeFilter(FilterType, int32_t);
        void addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter);
        void removeOverwriteResponse(std::string packetPartToCapture);
        std::string handleCLICommand(std::string command);
        void loadVariables();
        void saveVariables();
        void saveFilters();
        void saveResponsesToOverwrite();
        void serializeFilters(std::vector<uint8_t>& encodedData);
        void unserializeFilters(std::shared_ptr<std::vector<char>> serializedData);
        void serializeResponsesToOverwrite(std::vector<uint8_t>& encodedData);
        void unserializeResponsesToOverwrite(std::shared_ptr<std::vector<char>> serializedData);
        void unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
    private:
        //In table variables
        bool _enabled = true;
        std::list<HM_SD_Filter> _filters;
        std::list<HM_SD_OverwriteResponse> _responsesToOverwrite;
        //End

        void init();
};

#endif // HM_SD_H
