/*
 * GD.cpp
 *
 *  Created on: May 19, 2013
 *      Author: sathya
 */

#include "GD.h"

std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
HomeMaticDevices GD::devices;
XMLRPC::Server GD::xmlrpcServer;
XMLRPC::Devices GD::xmlrpcDevices;
int32_t GD::debugLevel = 5;
