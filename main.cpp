/* Copyright 2013 Sathya Laufer
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

#include <readline/readline.h>
#include <readline/history.h>
//#include <ncurses.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

#include "Version.h"
#include "Devices/HM-SD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HomeMaticCentral.h"
#include "HomeMaticDevice.h"
#include "Database.h"
#include "GD.h"
#include "HelperFunctions.h"
#include "RPC/ServerSettings.h"

bool _startAsDaemon = false;

void startRPCServers()
{
	for(uint32_t i = 0; i < GD::serverSettings.count(); i++)
	{
		std::shared_ptr<RPC::ServerSettings::Settings> settings = GD::serverSettings.get(i);
		std::string info = "Starting XML RPC server " + settings->name + " listening on " + settings->interface + ":" + std::to_string(settings->port);
		if(settings->ssl) info += ", SSL enabled";
		if(settings->authType != RPC::ServerSettings::Settings::AuthType::none) info += ", authentification enabled";
		info += "...";
		HelperFunctions::printInfo(info);
		GD::rpcServers[i].start(settings);
	}
	if(GD::rpcServers.size() == 0)
	{
		HelperFunctions::printCritical("Critical: No RPC servers are running. Terminating Homegear.");
		exit(1);
	}
}

void stopRPCServers()
{
	HelperFunctions::printInfo( "(Shutdown) => Stopping RPC servers");
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		i->second.stop();
	}
	GD::rpcServers.clear();
}

void terminate(int32_t signalNumber)
{
	try
	{
		if(signalNumber == SIGTERM)
		{
			HelperFunctions::printMessage("(Shutdown) => Stopping Homegear (Signal: " + std::to_string(signalNumber) + ")");
			if(_startAsDaemon)
			{
				HelperFunctions::printInfo("(Shutdown) => Stopping CLI server");
				GD::cliServer.stop();
			}
			stopRPCServers();
			HelperFunctions::printInfo( "(Shutdown) => Stopping RPC client");
			GD::rpcClient.reset();
			HelperFunctions::printInfo( "(Shutdown) => Closing RF device");
			GD::rfDevice->stopListening();
			GD::devices.save(false);
			HelperFunctions::printMessage("(Shutdown) => Shutdown complete.");
			if(_startAsDaemon)
			{
				fclose(stdout);
				fclose(stderr);
			}
			exit(0);
		}
		else if(signalNumber == SIGHUP)
		{
			stopRPCServers();
			HelperFunctions::printMessage("Reloading settings...");
			GD::settings.load(GD::configPath + "main.conf");
			GD::clientSettings.load(GD::settings.clientSettingsPath());
			GD::serverSettings.load(GD::settings.serverSettingsPath());
			startRPCServers();
			//Reopen log files, important for logrotate
			if(_startAsDaemon)
			{
				if(!std::freopen((GD::settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
				{
					HelperFunctions::printError("Error: Could not redirect output to new log file.");
				}
				if(!std::freopen((GD::settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
				{
					HelperFunctions::printError("Error: Could not redirect errors to new log file.");
				}
			}
		}
		else
		{
			HelperFunctions::printCritical("Critical: Signal " + std::to_string(signalNumber) + " received. Stopping Homegear...");
			HelperFunctions::printCritical("Critical: Trying to save data to " + GD::settings.databasePath() + ".crash");
			GD::db.init(GD::settings.databasePath(), GD::settings.databasePath() + ".crash");
			if(GD::db.isOpen()) GD::devices.save(false, true);
			signal(signalNumber, SIG_DFL); //Reset signal handler for the current signal to default
			kill(getpid(), signalNumber); //Generate core dump
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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

void printHelp()
{
	std::cout << "Usage: homegear [OPTIONS]" << std::endl << std::endl;
	std::cout << "Option\t\tMeaning" << std::endl;
	std::cout << "-h\t\tShow this help" << std::endl;
	std::cout << "-c <path>\tSpecify path to config file" << std::endl;
	std::cout << "-d\t\tRun as daemon" << std::endl;
	std::cout << "-p <pid path>\tSpecify path to process id file" << std::endl;
	std::cout << "-r\t\tConnect to Homegear on this machine" << std::endl;
	std::cout << "-v\t\tPrint program version" << std::endl;
}

void startDaemon()
{
	try
	{
		pid_t pid, sid;
		pid = fork();
		if(pid < 0) exit(1);
		if(pid > 0) exit(0);
		//Set process permission
		umask(S_IWGRP | S_IWOTH);
		//Set child processe's id
		sid = setsid();
		if(sid < 0) exit(1);
		//Set root directory as working directory (always available)
		if((chdir(GD::settings.logfilePath().c_str())) < 0)
		{
			HelperFunctions::printError("Could not change working directory to " + GD::settings.logfilePath() + ".");
			exit(1);
		}

		close(STDIN_FILENO);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int main(int argc, char* argv[])
{
    try
    {
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
    		else if(arg == "-d")
    		{
    			_startAsDaemon = true;
    		}
    		else if(arg == "-r")
    		{
    			GD::cliClient.start();
    			exit(0);
    		}
    		else if(arg == "-v")
    		{
    			std::cout <<  "Homegear version " << VERSION << std::endl;
    			std::cout << "Copyright (C) 2013 Sathya Laufer" << std::endl;
    			exit(0);
    		}
    		else
    		{
    			printHelp();
    			exit(1);
    		}
    	}

        /*int row,col;
        WINDOW* mainWindow = initscr();
        if(!mainWindow) HelperFunctions::printError("Bla" << std::endl;

        getmaxyx(stdscr, row, col);
        WINDOW* left = newwin(row, col / 2, 0, 0);
        WINDOW* right = newwin(row, col / 2, 0, col);

        mvwprintw(left, row/2, 0, "%s", "Hallo");
        refresh();
        mvwprintw(right, row/2 - 2, 0, "%s", "Hallo2");
        refresh();
        //std::string input2 = "";
        //std::cin >> input2;
        //mvwprintw(right, row/2 - 4, 0, "%s", input2.c_str());
        getch();
        endwin();
        //delscreen for all screens!!!
        return 0;*/

    	GD::bigEndian = HelperFunctions::isBigEndian();

    	char path[1024];
    	if(!getcwd(path, 1024)) throw Exception("Could not get working directory.");
    	GD::workingDirectory = std::string(path);
		ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
		if (length == -1) throw Exception("Could not get executable path.");
		path[length] = '\0';
		GD::executablePath = std::string(path);
		GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
		if(GD::configPath.empty()) GD::configPath = "/etc/homegear/";
		HelperFunctions::printInfo("Loading settings from " + GD::configPath + "main.conf");
		GD::settings.load(GD::configPath + "main.conf");
		HelperFunctions::printInfo("Loading RPC server settings from " + GD::settings.serverSettingsPath());
		GD::serverSettings.load(GD::settings.serverSettingsPath());
		HelperFunctions::printInfo("Loading RPC client settings from " + GD::settings.clientSettingsPath());
		GD::clientSettings.load(GD::settings.clientSettingsPath());

    	if(_startAsDaemon) startDaemon();

    	//Set rlimit for core dumps
    	struct rlimit limits;
    	getrlimit(RLIMIT_CORE, &limits);
    	limits.rlim_cur = limits.rlim_max;
    	setrlimit(RLIMIT_CORE, &limits);
    	getrlimit(RLIMIT_RTPRIO, &limits);
    	limits.rlim_cur = limits.rlim_max;
    	setrlimit(RLIMIT_RTPRIO, &limits);

    	//Analyze core dump with:
    	//gdb homegear core
    	//where
    	//thread apply all bt

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
				int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
				if(rc && errno == EWOULDBLOCK)
				{
					HelperFunctions::printError("Error: Homegear is already running - Can't lock PID file.");
				}
				std::string pid(std::to_string(getpid()));
				int32_t bytesWritten = write(pidfile, pid.c_str(), pid.size());
				if(bytesWritten <= 0) HelperFunctions::printError("Error writing to PID file: " + std::string(strerror(errno)));
				close(pidfile);
			}
		}
		catch(const std::exception& ex)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(Exception& ex)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}

		if(_startAsDaemon)
		{
			if(!std::freopen((GD::settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
			{
				HelperFunctions::printError("Error: Could not redirect output to log file.");
			}
			if(!std::freopen((GD::settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
			{
				HelperFunctions::printError("Error: Could not redirect errors to log file.");
			}
		}

		for(uint32_t i = 0; i < 10; ++i)
		{
			if(HelperFunctions::getTime() < 1000000000000)
			{
				HelperFunctions::printWarning("Warning: Time is in the past. Waiting for ntp to set the time...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			}
			else break;
		}
		if(HelperFunctions::getTime() < 1000000000000)
		{
			HelperFunctions::printCritical("Critical: Time is still in the past. Check that ntp is setup correctly and your internet connection is working. Exiting...");
			exit(1);
		}

    	GD::db.init(GD::settings.databasePath(), GD::settings.databasePath() + ".bak");
    	if(!GD::db.isOpen()) exit(1);

    	GD::rfDevice = RF::RFDevice::create(GD::settings.rfDeviceType());
    	if(!GD::rfDevice)
    	{
    		HelperFunctions::printError("Could not create rf device");
    		return 1;
    	}
        GD::rfDevice->init(GD::settings.rfDevice());
        if(!GD::rfDevice) return 1;
        HelperFunctions::printInfo("Loading XML RPC devices...");
        GD::rpcDevices.load();
        GD::devices.convertDatabase();
        HelperFunctions::printInfo("Start listening for BidCoS packets...");
        GD::rfDevice->startListening();
        if(!GD::rfDevice->isOpen()) return 1;
        HelperFunctions::printInfo("Loading devices...");
        GD::devices.load(); //Don't load before database is open!
        if(_startAsDaemon)
        {
        	HelperFunctions::printInfo("Starting CLI server...");
        	GD::cliServer.start();
        }

        startRPCServers();

        HelperFunctions::printInfo("Loading events...");
        GD::eventHandler.load();
        HelperFunctions::printInfo("Startup complete.");

        rl_bind_key('\t', rl_abort); //no autocompletion

		char* inputBuffer;
        std::string input;
        uint32_t bytes;
        if(_startAsDaemon)
        	while(true) std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        else
        {
			while((inputBuffer = readline("")) != NULL)
			{
				input = std::string(inputBuffer);
				bytes = strlen(inputBuffer);
				if(inputBuffer[0] == '\n' || inputBuffer[0] == 0) continue;
				if(strcmp(inputBuffer, "quit") == 0 || strcmp(inputBuffer, "exit") == 0) break;

				add_history(inputBuffer); //Sets inputBuffer to 0

				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				std::string packetHex("1ACAA0101D94A8FD00010301110B2A221828005800002422482A8A");
				packet->import(packetHex, false);
				if(input == "test") GD::devices.getCentral()->packetReceived(packet);
				else if(input == "test2")
				{
					std::vector<uint8_t> payload({2, 1, 1, 0, 0});
					std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(0x2F, 0xA0, 0x11, 0x212000, 0x1F454D, payload));
					GD::rfDevice->sendPacket(packet);
				}
				else std::cout << GD::devices.handleCLICommand(input);
			}
        }

        terminate(SIGTERM);
        return 0;
    }
    catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	terminate(SIGTERM);
    return 1;
}
