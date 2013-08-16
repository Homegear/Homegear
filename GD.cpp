#include "GD.h"

std::shared_ptr<RF::RFDevice> GD::rfDevice;
std::string GD::configPath = "";
std::string GD::pidfilePath = "";
std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
HomeMaticDevices GD::devices;
RPC::Server GD::rpcServer;
RPC::Client GD::rpcClient;
CLI::Server GD::cliServer;
CLI::Client GD::cliClient;
RPC::Devices GD::rpcDevices;
int32_t GD::debugLevel = 7;
int32_t GD::rpcLogLevel = 1;
bool GD::bigEndian = false;
Settings GD::settings;
