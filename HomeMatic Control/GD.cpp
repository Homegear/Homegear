#include "GD.h"

std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
HomeMaticDevices GD::devices;
RPC::Server GD::rpcServer;
RPC::Devices GD::rpcDevices;
int32_t GD::debugLevel = 7;
