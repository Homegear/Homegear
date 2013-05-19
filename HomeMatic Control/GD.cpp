/*
 * GD.cpp
 *
 *  Created on: May 19, 2013
 *      Author: sathya
 */

#include "GD.h"

std::string GD::startUpPath = "";
Database* GD::db = nullptr;
Cul* GD::cul = nullptr;
std::vector<HomeMaticDevice>* GD::devices = new std::vector<HomeMaticDevice>();
