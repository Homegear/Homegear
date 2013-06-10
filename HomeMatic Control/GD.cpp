#include "GD.h"

Cul GD::cul;
std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
HomeMaticDevices GD::devices;
RPC::Server GD::rpcServer;
RPC::Devices GD::rpcDevices;
int32_t GD::debugLevel = 7;
int32_t GD::rpcLogLevel = 1;
bool GD::bigEndian = false;
