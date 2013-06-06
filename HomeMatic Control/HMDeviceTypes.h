#ifndef HMDEVICETYPES_H_
#define HMDEVICETYPES_H_

#include <iostream>

enum class HMDeviceTypes : uint32_t { HMUNKNOWN = 0xFFFFFFFF, HMSD = 0xFFFFFFFE, HMCENTRAL = 0xFFFFFFFD, HMRCV50 = 0x0000, HMCCTC = 0x0039, HMCCVD = 0x003A };

#endif
