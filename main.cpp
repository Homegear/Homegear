#define VERSION "0.0.1"

#include <algorithm>
//#include <ncurses.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/file.h>

#include "Cul.h"
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
		GD::rpcServer.stop();
		GD::rpcClient.reset();
		GD::cul.stopListening();
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
		//Set child processes id
		sid = setsid();
		if(sid < 0) exit(1);
		//Set root directory as working directory (always available)
		if((chdir("/")) < 0) exit(1);

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

        /*int row,col;
        WINDOW* mainWindow = initscr();

        getmaxyx(stdscr, row, col);
        WINDOW* left = newwin(row, col / 2, 0, 0);
        WINDOW* right = newwin(row, col / 2, 0, col);

        mvwprintw(left, row/2, 0, "%s", "Hallo");
        refresh();
        mvwprintw(right, row/2 - 2, 0, "%s", "Hallo2");
        refresh();
        std::string input2 = "";
        cin >> input2;
        mvwprintw(right, row/2 - 4, 0, "%s", input2.c_str());
        getch();
        endwin();
        //delscreen for all screens!!!
        return 0;*/

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

    	std::shared_ptr<HomeMaticDevice> currentDevice;

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
		if(startAsDaemon)
		{
			std::freopen((GD::settings.logfilePath() + "homegear.log").c_str(), "a", stdout);
			std::freopen((GD::settings.logfilePath() + "homegear.err").c_str(), "a", stderr);
		}
    	GD::db.init(GD::settings.databasePath());

        GD::cul.init(GD::settings.culDevice());
        if(GD::debugLevel >= 4) std::cout << "Start listening for BidCoS packets..." << std::endl;
        GD::cul.startListening();
        if(!GD::cul.isOpen()) return -1;
        if(GD::debugLevel >= 4) std::cout << "Loading XML RPC devices..." << std::endl;
        GD::rpcDevices.load();
        if(GD::debugLevel >= 4) std::cout << "Loading devices..." << std::endl;
        GD::devices.load(); //Don't load before database is open!
        if(GD::debugLevel >= 4) std::cout << "Starting XML RPC server..." << std::endl;
        GD::rpcServer.start();

        //sd->addFilter(FilterType::SenderAddress, 0x1E53E7);
        //sd->addFilter(FilterType::DestinationAddress, 0x1E53E7);

        //sd->addFilter(FilterType::SenderAddress, 0x390001);
        //sd->addFilter(FilterType::DestinationAddress, 0x390001);

        //sd->addFilter(FilterType::SenderAddress, 0x1DA44D); //Stellantrieb Preetz 1. OG Zimmer NW
        //sd->addFilter(FilterType::DestinationAddress, 0x1DA44D); //Stellantrieb Preetz 1. OG Zimmer NW

        //sd->addFilter(FilterType::SenderAddress, 0x1D8DDD); //Wandthermostat Kiel Zimmer NW
        //sd->addFilter(FilterType::DestinationAddress, 0x1D8DDD); //Wandthermostat Kiel Zimmer NW

        //sd->addFilter(FilterType::SenderAddress, 0x1D8F45); //Wandthermostat Preetz 1. OG Zimmer NW
        //sd->addFilter(FilterType::DestinationAddress, 0x1D8F45); //Wandthermostat Preetz 1. OG Zimmer NW

        //sd->addFilter(FilterType::DestinationAddress, 0x1DDD0F);
        //sd->addFilter(FilterType::SenderAddress, 0x1F454D);
        //sd->addFilter(FilterType::DestinationAddress, 0x1F454D);

        char inputBuffer[256];
        std::string input = "";
        while(input != "q" && input != "quit")
        {
            std::cin.getline(inputBuffer, 256);
            input = std::string(inputBuffer);
            if(input == "h" || input == "?" || input == "help")
            {
                //Help
            }
            else if(input == "test")
            {
            	std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
            	packet->import("0E3D82021DA44D1D8F45010150163845", false);
            	GD::devices.getCentral()->packetReceived(packet);
            }
            else if(input == "test2")
            {
            }
            else if(input == "create device" || input == "add device")
            {
            	std::cout << "Please enter a 3 byte address for the device in hexadecimal format (e. g. 3A0001): ";
            	int32_t address = getHexInput();

            	if(address < 1 || address > 0xFFFFFF) std::cout << "Address not valid." << std::endl;
            	else if(GD::devices.get(address) != nullptr) std::cout << "Address already in use." << std::endl;
            	else
            	{
            		std::cout << "Please enter a serial number (length 10, e. g. VVD0000001): ";
            		std::cin >> input;
            		if(input.size() != 10) std::cout << "Serial number has wrong length." << std::endl;
            		else
            		{
            			std::string serialNumber = input;
            			std::cout << "Please enter a device type: ";
						int32_t deviceType = getHexInput();
						switch(deviceType)
						{
						case (uint32_t)HMDeviceTypes::HMCCTC:
							GD::devices.add(new HM_CC_TC(serialNumber, address));
							std::cout << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
							break;
						case (uint32_t)HMDeviceTypes::HMCCVD:
							GD::devices.add(new HM_CC_VD(serialNumber, address));
							std::cout << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
							break;
						case (uint32_t)HMDeviceTypes::HMCENTRAL:
							GD::devices.add(new HomeMaticCentral(serialNumber, address));
							std::cout << "Created HMCENTRAL with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
							break;
						case (uint32_t)HMDeviceTypes::HMSD:
							GD::devices.add(new HM_SD(serialNumber, address));
							std::cout << "Created HM_SD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
							break;
						default:
							std::cout << "Unknown device type." << std::endl;
						}
            		}
            	}
            }
            else if(input == "remove device" || input == "delete device")
            {
            	std::cout << "Please enter the address of the device to delete (e. g. 3A0001): ";
            	int32_t address = getHexInput();
            	if(currentDevice != nullptr && currentDevice->address() == address) currentDevice = nullptr;
            	if(GD::devices.remove(address)) std::cout << "Device removed." << std::endl;
            	else std::cout << "Device not found." << std::endl;
            }
            else if(input == "select device")
            {
            	std::cout << "Device address: ";
            	int32_t address = getHexInput();
            	currentDevice = GD::devices.get(address);
            	if(currentDevice == nullptr) std::cout << "Device not found." << std::endl;
            	else std::cout << "Device selected." << std::endl;
            }
            else if(input == "list devices")
            {
            	std::vector<std::shared_ptr<HomeMaticDevice>>* devices = GD::devices.getDevices();
            	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = devices->begin(); i != devices->end(); ++i)
            	{
            		std::cout << "Address: 0x" << std::hex << (*i)->address() << "\tSerial number: " << (*i)->serialNumber() << "\tDevice type: " << (uint32_t)(*i)->deviceType() << std::endl << std::dec;
            	}
            }
            else if(input == "set verbosity")
            {
            	std::cout << "Verbosity (0 - 8): ";
            	int32_t verbosity = getHexInput();
            	if(verbosity < 0 || verbosity > 8) std::cout << "Invalid verbosity." << std::endl;
            	else
            	{
            		GD::debugLevel = verbosity;
            		std::cout << "Verbosity set to " << verbosity << std::endl;
            	}
            }
            else if(input == "time")
            {
            	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << std::endl;
            }
            else if(input == "q" || input == "quit") {} //nothing
            else
            {
            	if(currentDevice != nullptr)
            	{
            		currentDevice->handleCLICommand(input);
            	}
            }
        }

        //Stop rpc server and client before saving
        GD::rpcServer.stop();
        GD::rpcClient.reset();
        GD::cul.stopListening();
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
