/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "Version.h"
#include "Libraries/GD/GD.h"
#include "Libraries/UPnP/UPnP.h"
#include "Libraries/MQTT/MQTT.h"
#include "Modules/Base/BaseLib.h"
#include "Modules/Base/HelperFunctions/HelperFunctions.h"
#include "Libraries/RPC/ServerInfo.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/sysctl.h> //For BSD systems

#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

#include <gcrypt.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;

bool _startAsDaemon = false;
bool _startUpComplete = false;
bool _disposing = false;
std::shared_ptr<std::function<void(int32_t, std::string)>> _errorCallback;

void exitHomegear(int exitCode)
{
	GD::physicalInterfaces.dispose();
    GD::familyController.dispose();
    exit(exitCode);
}

void startRPCServers()
{
	for(int32_t i = 0; i < GD::serverInfo.count(); i++)
	{
		std::shared_ptr<RPC::ServerInfo::Info> settings = GD::serverInfo.get(i);
		std::string info = "Starting XML RPC server " + settings->name + " listening on " + settings->interface + ":" + std::to_string(settings->port);
		if(settings->ssl) info += ", SSL enabled";
		else GD::bl->rpcPort = settings->port;
		if(settings->authType != RPC::ServerInfo::Info::AuthType::none) info += ", authentification enabled";
		info += "...";
		GD::out.printInfo(info);
		GD::rpcServers[i].start(settings);
		if(GD::bl->settings.enableUPnP() && !settings->ssl && settings->authType == RPC::ServerInfo::Info::AuthType::none && settings->webServer) GD::uPnP->registerServer(settings->port);
	}
	if(GD::rpcServers.size() == 0)
	{
		GD::out.printCritical("Critical: No RPC servers are running. Terminating Homegear.");
		exitHomegear(1);
	}

}

void stopRPCServers(bool dispose)
{
	GD::uPnP->resetServers();
	GD::out.printInfo( "(Shutdown) => Stopping RPC servers");
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		i->second.stop();
		if(dispose) i->second.dispose();
	}
	GD::bl->rpcPort = 0;
	//Don't clear map!!! Server is still accessed i. e. by the event handler!
}

void terminate(int32_t signalNumber)
{
	try
	{
		if(signalNumber == SIGTERM)
		{
			while(!_startUpComplete)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			GD::familyController.homegearShuttingDown();
			GD::bl->shuttingDown = true;
			_disposing = true;
			GD::out.printMessage("(Shutdown) => Stopping Homegear (Signal: " + std::to_string(signalNumber) + ")");
			if(_startAsDaemon)
			{
				GD::out.printInfo("(Shutdown) => Stopping CLI server");
				GD::cliServer.stop();
			}
			if(GD::bl->settings.enableUPnP())
			{
				GD::out.printInfo("Stopping UPnP server...");
				GD::uPnP->stop();
			}
#ifdef EVENTHANDLER
			GD::eventHandler.dispose();
#endif
			stopRPCServers(true);
			GD::rpcServers.clear();
			GD::out.printInfo( "(Shutdown) => Stopping Event handler");

			if(GD::mqtt->enabled())
			{
				GD::out.printInfo( "(Shutdown) => Stopping MQTT client");;
				GD::mqtt->stop();
			}
			GD::out.printInfo( "(Shutdown) => Stopping RPC client");;
			GD::rpcClient.dispose();
			GD::out.printInfo( "(Shutdown) => Closing physical devices");
			GD::physicalInterfaces.stopListening();
			GD::physicalInterfaces.dispose();
#ifdef SCRIPTENGINE
			GD::scriptEngine.dispose();
#endif
			GD::familyController.save(false);
			GD::db.dispose(); //Finish database operations before closing modules, otherwise SEGFAULT
			GD::familyController.dispose();
			GD::out.printMessage("(Shutdown) => Shutdown complete.");
			if(_startAsDaemon)
			{
				fclose(stdout);
				fclose(stderr);
			}
			gnutls_global_deinit();
			gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
			gcry_control(GCRYCTL_TERM_SECMEM);
			gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
			exit(0);
		}
		else if(signalNumber == SIGHUP)
		{
			GD::out.printInfo("Info: SIGHUP received... Reloading...");
			if(!_startUpComplete)
			{
				GD::out.printError("Error: Cannot reload. Startup is not completed.");
				return;
			}
			_startUpComplete = false;
			if(GD::bl->settings.changed())
			{
				if(GD::bl->settings.enableUPnP())
				{
					GD::out.printInfo("Stopping UPnP server");
					GD::uPnP->stop();
				}
				stopRPCServers(false);
				if(GD::mqtt->enabled())
				{
					GD::out.printInfo( "(Shutdown) => Stopping MQTT client");;
					GD::mqtt->stop();
				}
				GD::physicalInterfaces.stopListening();
				//Binding fails sometimes with "address is already in use" without waiting.
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				GD::out.printMessage("Reloading settings...");
				GD::bl->settings.load(GD::configPath + "main.conf");
				GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
				GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
				GD::mqtt->loadSettings();
				if(GD::mqtt->enabled())
				{
					GD::out.printInfo("Starting MQTT client");;
					GD::mqtt->start();
				}
				GD::physicalInterfaces.startListening();
				startRPCServers();
				if(GD::bl->settings.enableUPnP())
				{
					GD::out.printInfo("Starting UPnP server");
					GD::uPnP->start();
				}
			}
			//Reopen log files, important for logrotate
			if(_startAsDaemon)
			{
				if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
				{
					GD::out.printError("Error: Could not redirect output to new log file.");
				}
				if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
				{
					GD::out.printError("Error: Could not redirect errors to new log file.");
				}
			}
			GD::db.hotBackup();
			if(!GD::db.isOpen())
			{
				GD::out.printCritical("Critical: Can't reopen database. Exiting...");
				exit(1);
			}
			_startUpComplete = true;
			GD::out.printInfo("Info: Reload complete.");
		}
		else
		{
			if(!_disposing) GD::out.printCritical("Critical: Signal " + std::to_string(signalNumber) + " received. Stopping Homegear...");
			signal(signalNumber, SIG_DFL); //Reset signal handler for the current signal to default
			kill(getpid(), signalNumber); //Generate core dump
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void errorCallback(int32_t level, std::string message)
{
	GD::rpcClient.broadcastError(level, message);
}

int32_t getIntInput()
{
	std::string input;
	std::cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input); } catch(...) {}
    return intInput;
}

int32_t getHexInput()
{
	std::string input;
	std::cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input, 0, 16); } catch(...) {}
    return intInput;
}

void getExecutablePath()
{
	char path[1024];
	if(!getcwd(path, sizeof(path)))
	{
		std::cerr << "Could not get working directory." << std::endl;
		exit(1);
	}
	GD::workingDirectory = std::string(path);
#ifdef KERN_PROC //BSD system
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	size_t cb = sizeof(path);
	int result = sysctl(mib, 4, path, &cb, NULL, 0);
	if(result == -1)
	{
		std::cerr << "Could not get executable path." << std::endl;
		exit(1);
	}
	path[sizeof(path) - 1] = '\0';
	GD::executablePath = std::string(path);
	GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#else
	int length = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (length < 0)
	{
		std::cerr << "Could not get executable path." << std::endl;
		exit(1);
	}
	if((unsigned)length > sizeof(path))
	{
		std::cerr << "The path the homegear binary is in has more than 1024 characters." << std::endl;
		exit(1);
	}
	path[length] = '\0';
	GD::executablePath = std::string(path);
	GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#endif
}

void printHelp()
{
	std::cout << "Usage: homegear [OPTIONS]" << std::endl << std::endl;
	std::cout << "Option\t\t\tMeaning" << std::endl;
	std::cout << "-h\t\t\tShow this help" << std::endl;
	std::cout << "-c <path>\t\tSpecify path to config file" << std::endl;
	std::cout << "-d\t\t\tRun as daemon" << std::endl;
	std::cout << "-p <pid path>\t\tSpecify path to process id file" << std::endl;
	std::cout << "-s <user> <group>\tSet GPIO settings and necessary permissions for all defined physical devices" << std::endl;
	std::cout << "-r\t\t\tConnect to Homegear on this machine" << std::endl;
	std::cout << "-e <command>\t\tExecute CLI command" << std::endl;
	std::cout << "-v\t\t\tPrint program version" << std::endl;
}

void startDaemon()
{
	try
	{
		pid_t pid, sid;
		pid = fork();
		if(pid < 0)
		{
			exitHomegear(1);
		}
		if(pid > 0)
		{
			exitHomegear(0);
		}
		//Set process permission
		umask(S_IWGRP | S_IWOTH);
		//Set child processe's id
		sid = setsid();
		if(sid < 0)
		{
			exitHomegear(1);
		}
		//Set root directory as working directory (always available)
		if((chdir(GD::bl->settings.logfilePath().c_str())) < 0)
		{
			GD::out.printError("Could not change working directory to " + GD::bl->settings.logfilePath() + ".");
			exitHomegear(1);
		}

		close(STDIN_FILENO);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int main(int argc, char* argv[])
{
    try
    {
    	getExecutablePath();
    	_errorCallback.reset(new std::function<void(int32_t, std::string)>(errorCallback));
    	GD::bl.reset(new BaseLib::Obj(GD::executablePath, _errorCallback.get()));
    	GD::out.init(GD::bl.get());

    	for(int32_t i = 1; i < argc; i++)
    	{
    		std::string arg(argv[i]);
    		if(arg == "-h" || arg == "--help")
    		{
    			printHelp();
    			exit(0);
    		}
    		else if(arg == "-c")
    		{
    			if(i + 1 < argc)
    			{
    				GD::configPath = std::string(argv[i + 1]);
    				if(!GD::configPath.empty() && GD::configPath[GD::configPath.size() - 1] != '/') GD::configPath.push_back('/');
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-p")
    		{
    			if(i + 1 < argc)
    			{
    				GD::pidfilePath = std::string(argv[i + 1]);
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-s")
    		{
    			if(i + 2 < argc)
    			{
    				if(getuid() != 0)
    				{
    					std::cout <<  "Please run Homegear as root to set the device permissions." << std::endl;
    					exit(1);
    				}
    				GD::bl->settings.load(GD::configPath + "main.conf");
    				GD::bl->debugLevel = 3; //Only output warnings.
    				GD::familyController.loadModules();
    				GD::physicalInterfaces.load(GD::bl->settings.physicalInterfaceSettingsPath());
    				int32_t userID = GD::bl->hf.userID(std::string(argv[i + 1]));
    				int32_t groupID = GD::bl->hf.groupID(std::string(argv[i + 2]));
    				GD::out.printDebug("Debug: User ID set to " + std::to_string(userID) + " group ID set to " + std::to_string(groupID));
    				if(userID == -1 || groupID == -1)
    				{
    					GD::out.printCritical("Could not setup physical devices. Username or group name is not valid.");
    					GD::physicalInterfaces.dispose();
    					GD::familyController.dispose();
    					exit(1);
    				}
    				GD::physicalInterfaces.setup(userID, groupID);
    				GD::physicalInterfaces.dispose();
    				GD::familyController.dispose();
    				exit(0);
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-d")
    		{
    			_startAsDaemon = true;
    		}
    		else if(arg == "-r")
    		{
    			GD::cliClient.start();
    			exit(0);
    		}
    		else if(arg == "-e")
    		{
    			std::stringstream command;
    			if(i + 1 < argc)
    			{
    				command << std::string(argv[i + 1]);
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}

    			for(int32_t j = i + 2; j < argc; j++)
    			{
    				std::string element(argv[j]);
    				if(element.find(' ') != std::string::npos) command << " \"" << element << "\"";
    				else command << " " << argv[j];
    			}

    			GD::cliClient.start(command.str());
    			exit(0);
    		}
    		else if(arg == "-v")
    		{
    			std::cout <<  "Homegear version " << VERSION << std::endl;
    			std::cout << "Copyright (C) 2013-2015 Sathya Laufer" << std::endl;
    			exit(0);
    		}
    		else
    		{
    			printHelp();
    			exit(1);
    		}
    	}

		if(GD::configPath.empty()) GD::configPath = "/etc/homegear/";
		GD::out.printInfo("Loading settings from " + GD::configPath + "main.conf");
		GD::bl->settings.load(GD::configPath + "main.conf");

		GD::out.printInfo("Loading RPC server settings from " + GD::bl->settings.serverSettingsPath());
		GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
		GD::out.printInfo("Loading RPC client settings from " + GD::bl->settings.clientSettingsPath());
		GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
		GD::mqtt->loadSettings();

    	if(_startAsDaemon) startDaemon();

    	//Set rlimit for core dumps
    	struct rlimit limits;
    	getrlimit(RLIMIT_CORE, &limits);
    	limits.rlim_cur = limits.rlim_max;
    	setrlimit(RLIMIT_CORE, &limits);
#ifdef RLIMIT_RTPRIO //Not existant on BSD systems
    	getrlimit(RLIMIT_RTPRIO, &limits);
    	limits.rlim_cur = limits.rlim_max;
    	setrlimit(RLIMIT_RTPRIO, &limits);
#endif

    	//Enable printing of backtraces
    	signal(SIGHUP, terminate);
    	signal(SIGABRT, terminate);
    	signal(SIGSEGV, terminate);
    	signal(SIGTERM, terminate);

    	//Create PID file
    	try
    	{
			if(!GD::pidfilePath.empty())
			{
				int32_t pidfile = open(GD::pidfilePath.c_str(), O_CREAT | O_RDWR, 0666);
				if(pidfile < 0)
				{
					GD::out.printError("Error: Cannot create pid file \"" + GD::pidfilePath + "\".");
				}
				else
				{
					int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
					if(rc && errno == EWOULDBLOCK)
					{
						GD::out.printError("Error: Homegear is already running - Can't lock PID file.");
					}
					std::string pid(std::to_string(getpid()));
					int32_t bytesWritten = write(pidfile, pid.c_str(), pid.size());
					if(bytesWritten <= 0) GD::out.printError("Error writing to PID file: " + std::string(strerror(errno)));
					close(pidfile);
				}
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(BaseLib::Exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}

		if(_startAsDaemon)
		{
			if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
			{
				GD::out.printError("Error: Could not redirect output to log file.");
			}
			if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
			{
				GD::out.printError("Error: Could not redirect errors to log file.");
			}
		}

		for(uint32_t i = 0; i < 10; ++i)
		{
			if(BaseLib::HelperFunctions::getTime() < 1000000000000)
			{
				GD::out.printWarning("Warning: Time is in the past. Waiting for ntp to set the time...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			}
			else break;
		}
		if(BaseLib::HelperFunctions::getTime() < 1000000000000)
		{
			GD::out.printCritical("Critical: Time is still in the past. Check that ntp is setup correctly and your internet connection is working. Exiting...");
			exit(1);
		}

		//Init gcrypt and GnuTLS
		gcry_error_t gcryResult;
		if((gcryResult = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR)
		{
			GD::out.printCritical("Critical: Could not enable thread support for gcrypt.");
			exit(2);
		}

		if (!gcry_check_version(GCRYPT_VERSION))
		{
			GD::out.printCritical("Critical: Wrong gcrypt version.");
			exit(2);
		}
		gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
		if((gcryResult = gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0)) != GPG_ERR_NO_ERROR)
		{
			GD::out.printCritical("Critical: Could not allocate secure memory.");
			exit(2);
		}
		gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
		gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

		int32_t gnutlsResult = 0;
		if((gnutlsResult = gnutls_global_init()) != GNUTLS_E_SUCCESS)
		{
			GD::out.printCritical("Critical: Could not initialize GnuTLS: " + std::string(gnutls_strerror(gnutlsResult)));
			exit(2);
		}
		//End init gcrypt

		GD::familyController.loadModules();
		if(GD::deviceFamilies.empty()) exitHomegear(1);

		GD::db.init();
    	GD::db.open(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databaseWALJournal(), GD::bl->settings.databasePath() + ".bak");
    	if(!GD::db.isOpen()) exitHomegear(1);

    	GD::physicalInterfaces.load(GD::bl->settings.physicalInterfaceSettingsPath());

        GD::out.printInfo("Initializing database...");
        GD::db.convertDatabase();
        GD::db.initializeDatabase();

        GD::out.printInfo("Initializing family controller...");
        GD::familyController.init();
        if(GD::deviceFamilies.empty()) exitHomegear(1);
        GD::out.printInfo("Loading devices...");
        GD::familyController.load(); //Don't load before database is open!

        GD::out.printInfo("Start listening for packets...");
        GD::physicalInterfaces.startListening();
        if(!GD::physicalInterfaces.isOpen())
        {
        	GD::out.printCritical("Critical: At least one of the physical devices could not be opened... Exiting...");
        	GD::physicalInterfaces.stopListening();
        	exitHomegear(1);
        }

        GD::out.printInfo("Initializing RPC client...");
        GD::rpcClient.init();

        if(GD::mqtt->enabled())
		{
			GD::out.printInfo("Starting MQTT client...");;
			GD::mqtt->start();
		}

        startRPCServers();

		GD::out.printInfo("Starting CLI server...");
		GD::cliServer.start();

#ifdef EVENTHANDLER
        GD::out.printInfo("Initializing event handler...");
        GD::eventHandler.init();
        GD::out.printInfo("Loading events...");
        GD::eventHandler.load();
#endif
        _startUpComplete = true;
        GD::out.printMessage("Startup complete. Waiting for physical interfaces to connect.");

        //Wait for all interfaces to connect before setting booting to false
        {
			std::vector<std::shared_ptr<BaseLib::Systems::IPhysicalInterface>> interfaces;
			for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>> familyInterfaces = GD::physicalInterfaces.get(i->first);
				for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = familyInterfaces.begin(); j != familyInterfaces.end(); ++j)
				{
					interfaces.push_back(j->second);
				}
			}

			for(int32_t i = 0; i < 300; i++)
			{
				bool continueLoop = false;
				for(std::vector<std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
				{
					if(!(*j)->isOpen())
					{
						continueLoop = true;
						break;
					}
				}
				if(!continueLoop)
				{
					GD::out.printMessage("All physical interfaces are connected now.");
					break;
				}
				if(i == 299) GD::out.printError("Error: At least one physical interface is not connected.");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			interfaces.clear();
        }

        if(GD::bl->settings.enableUPnP())
		{
        	GD::out.printInfo("Starting UPnP server...");
        	GD::uPnP->start();
		}

        rl_bind_key('\t', rl_abort); //no autocompletion

		char* inputBuffer = nullptr;
        if(_startAsDaemon)
        {
        	//Wait a little more before setting "booting" to false. If "isOpen" of the physical interface is implemented correctly, this is not necessary. But just in case.
			std::this_thread::sleep_for(std::chrono::milliseconds(30000));
        	GD::bl->booting = false;
        	while(true) std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        else
        {
        	GD::bl->booting = false;
			while((inputBuffer = readline("")) != NULL)
			{
				if(inputBuffer[0] == '\n' || inputBuffer[0] == 0) continue;
				if(strncmp(inputBuffer, "quit", 4) == 0 || strncmp(inputBuffer, "exit", 4) == 0 || strncmp(inputBuffer, "moin", 4) == 0) break;
				/*else if(strncmp(inputBuffer, "test", 4) == 0)
				{
					std::string json("{\"on\":true, \"off\":    false, \"bla\":null, \"blupp\":  [5.643,false , null ,true ],\"blupp2\":[ 5.643,false,null,true], \"sat\":255.63456, \"bri\":-255,\"hue\":10000, \"bli\":{\"a\": 2,\"b\":false}    ,     \"e\"  :   -34785326,\"f\":-0.547887237, \"g\":{},\"h\":[], \"i\" : {    }  , \"j\" : [    ] , \"k\": {} , \"l\": [] }");
					BaseLib::RPC::JsonDecoder bla(GD::bl.get());
					std::shared_ptr<BaseLib::RPC::Variable> var = bla.decode(json);
					var->print();
					BaseLib::RPC::JsonEncoder blupp(GD::bl.get());
					std::string hmm;
					blupp.encode(var, hmm);
					std::cout << hmm << std::endl;
					continue;
				}*/

				add_history(inputBuffer); //Sets inputBuffer to 0

				//std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				//std::string packetHex("1ACAA0101D94A8FD00010301110B2A221828005800002422482A8A");
				//packet->import(packetHex, false);
				//if(input == "test") GD::devices.getHomeMaticCentral()->packetReceived(packet);
				//else if(input == "test2")
				//{
					//std::vector<uint8_t> payload({2, 1, 1, 0, 0});
					//std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(0x2F, 0xA0, 0x11, 0x212000, 0x1F454D, payload));
					//GD::physicalInterfaces.get(DeviceFamily::HomeMaticBidCoS)->sendPacket(packet);
				//}
				std::string input(inputBuffer);
				std::cout << GD::familyController.handleCLICommand(input);
			}
        }

        terminate(SIGTERM);

        return 0;
    }
    catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	terminate(SIGTERM);

    return 1;
}
