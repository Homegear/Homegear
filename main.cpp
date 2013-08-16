#define VERSION "0.0.1"

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

#include "Devices/HM-SD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HomeMaticCentral.h"
#include "HomeMaticDevice.h"
#include "Database.h"
#include "GD.h"
#include "HelperFunctions.h"

void exceptionHandler(int32_t signalNumber) {
  void *stackTrace[30];
  size_t length = backtrace(stackTrace, 30);

  std::cerr << "Error: Signal " << signalNumber << ". Backtrace:" << std::endl;
  backtrace_symbols_fd(stackTrace, length, STDERR_FILENO);
  signal(signalNumber, SIG_DFL);
  kill(getpid(), signalNumber); //Generate core dump
}

void killHandler(int32_t signalNumber)
{
	try
	{
		std::cout << "Stopping Homegear..." << std::endl;
		GD::cliServer.stop();
		GD::rpcServer.stop();
		GD::rpcClient.reset();
		GD::rfDevice->stopListening();
		GD::devices.save();
		exit(0);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
	std::cout << "Usage: Homegear [OPTIONS]" << std::endl << std::endl;
	std::cout << "Option\t\tMeaning" << std::endl;
	std::cout << "--help\t\tShow this help" << std::endl;
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
			std::cerr << "Could not change working directory to " << GD::settings.logfilePath() << "." << std::endl;
			exit(1);
		}

		close(STDIN_FILENO);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

int main(int argc, char* argv[])
{
    try
    {
    	if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() < 1000000000000)
			throw(Exception("Time is in the past. Please run ntp or set date and time manually before starting this program."));

    	//Set rlimit for core dumps
    	struct rlimit coreLimits;
    	coreLimits.rlim_cur = coreLimits.rlim_max;
    	setrlimit(RLIMIT_CORE, &coreLimits);

    	//Analyze core dump with:
    	//gdb Homegear core
    	//where
    	//thread apply all bt

    	//Enable printing of backtraces
    	signal(SIGSEGV, exceptionHandler);
    	signal(SIGTERM, killHandler);

    	bool startAsDaemon = false;
    	for(int32_t i = 1; i < argc; i++)
    	{
    		std::string arg(argv[i]);
    		if(arg == "--help")
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
    			startAsDaemon = true;
    		}
    		else if(arg == "-r")
    		{
    			GD::cliClient.start();
    			exit(0);
    		}
    		else if(arg == "-v")
    		{
    			std::cout << "Homegear version " << VERSION << std::endl;
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
        if(!mainWindow) std::cerr << "Bla" << std::endl;

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
    	getcwd(path, 1024);
    	GD::workingDirectory = std::string(path);
		ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
		if (length == -1) throw Exception("Could not get executable path.");
		path[length] = '\0';
		GD::executablePath = std::string(path);
		GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
		if(GD::configPath.empty()) GD::configPath = "/etc/Homegear/";
		GD::settings.load(GD::configPath + "main.conf");

    	if(startAsDaemon) startDaemon();

    	//Create PID file
    	try
    	{
			if(!GD::pidfilePath.empty())
			{
				int32_t pidfile = open(GD::pidfilePath.c_str(), O_CREAT | O_RDWR, 0666);
				int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
				if(rc && errno == EWOULDBLOCK)
				{
					std::cerr << "Error: Homegear is already running - Can't lock PID file." << std::endl;
				}
				std::string pid(std::to_string(getpid()));
				write(pidfile, pid.c_str(), pid.size());
				close(pidfile);
			}
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
		}
		catch(const Exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
		}
		catch(...)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
		}

		if(startAsDaemon)
		{
			std::freopen((GD::settings.logfilePath() + "homegear.log").c_str(), "a", stdout);
			std::freopen((GD::settings.logfilePath() + "homegear.err").c_str(), "a", stderr);
		}
    	GD::db.init(GD::settings.databasePath());

    	GD::rfDevice = RF::RFDevice::create(GD::settings.rfDeviceType());
        GD::rfDevice->init(GD::settings.rfDevice());
        if(!GD::rfDevice) return -1;
        if(GD::debugLevel >= 4) std::cout << "Start listening for BidCoS packets..." << std::endl;
        GD::rfDevice->startListening();
        if(!GD::rfDevice->isOpen()) return -1;
        if(GD::debugLevel >= 4) std::cout << "Loading XML RPC devices..." << std::endl;
        GD::rpcDevices.load();
        if(GD::debugLevel >= 4) std::cout << "Loading devices..." << std::endl;
        GD::devices.load(); //Don't load before database is open!
        if(GD::debugLevel >= 4) std::cout << "Starting CLI server..." << std::endl;
        GD::cliServer.start();
        if(GD::debugLevel >= 4) std::cout << "Starting XML RPC server..." << std::endl;
        GD::rpcServer.start();

        rl_bind_key('\t', rl_abort); //no autocompletion

		char* inputBuffer;
        std::string input;
        uint32_t bytes;
        if(startAsDaemon)
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

				std::cout << GD::devices.handleCLICommand(input);
			}
        }

        std::cout << "Shutting down..." << std::endl;

        //Stop rpc server and client before saving
        std::cout << "Stopping CLI server..." << std::endl;
        GD::cliServer.stop();
        std::cout << "Stopping RPC server..." << std::endl;
        GD::rpcServer.stop();
        std::cout << "Stopping RPC client..." << std::endl;
        GD::rpcClient.reset();
        std::cout << "Closing RF device..." << std::endl;
        GD::rfDevice->stopListening();
        std::cout << "Saving devices..." << std::endl;
        GD::devices.save();
        std::cout << "Shutdown complete." << std::endl;
        if(startAsDaemon)
        {
        	fclose(stdout);
        	fclose(stderr);
        }
        return 0;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    return 1;
}
