#include "GD.h"

std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
HomeMaticDevices GD::devices;
RPC::Server GD::xmlrpcServer;
RPC::Devices GD::xmlrpcDevices;
int32_t GD::debugLevel = 5;
