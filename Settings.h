#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Exception.h"

#include <iostream>
#include <string>
#include <map>

class Settings {
public:
	Settings();
	virtual ~Settings() {}
	void load(std::string filename);

	std::string rpcInterface() { return _rpcInterface; }
	int32_t rpcPort() { return _rpcPort; }
	int32_t debugLevel() { return _debugLevel; }
	std::string databasePath() { return _databasePath; }
	std::string rfDeviceType() { return _rfDeviceType; }
	std::string rfDevice() { return _rfDevice; }
	std::string logfilePath() { return _logfilePath; }
	bool prioritizeThreads() { return _prioritizeThreads; }
	void setPrioritizeThreads(bool value) { _prioritizeThreads = value; }
	uint32_t workerThreadWindow() { return _workerThreadWindow; }
	uint32_t bidCoSResponseDelay() { return _bidCoSResponseDelay; }
	uint32_t rpcServerThreadPriority() { return _rpcServerThreadPriority; }
	std::map<std::string, bool>& tunnelClients() { return _tunnelClients; }
private:
	std::string _rpcInterface;
	int32_t _rpcPort = 2001;
	int32_t _debugLevel = 3;
	std::string _databasePath;
	std::string _rfDeviceType;
	std::string _rfDevice;
	std::string _logfilePath;
	bool _prioritizeThreads = true;
	uint32_t _workerThreadWindow = 3000;
	uint32_t _bidCoSResponseDelay = 90;
	uint32_t _rpcServerThreadPriority = 0;
	std::map<std::string, bool> _tunnelClients;

	void reset();
};

#endif /* SETTINGS_H_ */
