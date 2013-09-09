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
	int32_t rpcSSLPort() { return _rpcSSLPort; }
	std::string certPath() { return _certPath; }
	std::string keyPath() { return _keyPath;  }
	bool verifyCertificate() { return _verifyCertificate; }
	int32_t debugLevel() { return _debugLevel; }
	std::string databasePath() { return _databasePath; }
	bool databaseSynchronous() { return _databaseSynchronous; }
	bool databaseMemoryJournal() { return _databaseMemoryJournal; }
	std::string mySQLServer() { return _mySQLServer; }
	std::string mySQLDatabase() { return _mySQLDatabase; }
	std::string mySQLUser() { return _mySQLUser; }
	std::string mySQLPassword() { return _mySQLPassword; }
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
	int32_t _rpcSSLPort = 2002;
	std::string _certPath;
	std::string _keyPath;
	bool _verifyCertificate = true;
	int32_t _debugLevel = 3;
	std::string _databasePath;
	bool _databaseSynchronous = false;
	bool _databaseMemoryJournal = true;
	std::string _mySQLServer;
	std::string _mySQLDatabase;
	std::string _mySQLUser;
	std::string _mySQLPassword;
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
