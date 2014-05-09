#ifndef GDBASE_H_
#define GDBASE_H_

#include "Exception.h"
#include "Output/Output.h"
#include "HelperFunctions/HelperFunctions.h"
#include "FileDescriptorManager/FileDescriptorManager.h"
#include "Database/Database.h"
#include "Settings/Settings.h"

class BaseLib
{
public:
	static int32_t debugLevel;
	static FileDescriptorManager fileDescriptorManager;
	static Database db;
	static std::string executablePath;
	static Settings settings;


	static void init(std::string exePath);

	virtual ~BaseLib() {}
private:
	//Non public constructor
	BaseLib();
};

#endif
