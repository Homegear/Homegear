#ifndef HM_SD_H
#define HM_SD_H

using namespace std;

#include "HomeMaticDevice.h"
#include "Cul.h"

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

class HM_SD_BlockResponse
{
    public:
        int32_t sendAfter;
        std::string packetPartToCapture;
};

class HM_SD : public HomeMaticDevice
{
    public:
        HM_SD();
        virtual ~HM_SD();
        void packetReceived(BidCoSPacket*);
        void addFilter(FilterType, int32_t);
        void removeFilter(FilterType, int32_t);
        void addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter);
        void addBlockResponse(std::string packetPartToCapture, int32_t blockAfter);
    protected:
    private:
        std::list<HM_SD_Filter> _filters;
        std::list<HM_SD_OverwriteResponse> _responsesToOverwrite;
        std::list<HM_SD_BlockResponse> _responsesToBlock;
};

#endif // HM_SD_H
