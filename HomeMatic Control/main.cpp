using namespace std;

#include <algorithm>
#include <ncurses.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

#include "Cul.h"
#include "Devices/HM-RC-Sec3-B.h"
#include "Devices/HM-SD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "HomeMaticDevice.h"
#include "Database.h"
#include "GD.h"

//Testing
#include "XMLRPC/Device.h"
//Testing end

void exceptionHandler(int32_t signal) {
  void *stackTrace[10];
  size_t length = backtrace(stackTrace, 10);

  cerr << "Error: Signal " << signal << ":" << endl;
  backtrace_symbols_fd(stackTrace, length, 2);
  exit(1);
}

int32_t getIntInput()
{
	std::string input;
	cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input); } catch(...) {}
    return intInput;
}

int32_t getHexInput()
{
	std::string input;
	cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input, 0, 16); } catch(...) {}
    return intInput;
}

int main()
{
    try
    {
    	if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() < 1000000000000)
			throw(new Exception("Time is in the past. Please run ntp or set date and time manually before starting this program."));

    	signal(SIGSEGV, exceptionHandler);

    	XMLRPC::Device device("rf_cc_tc.xml");
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

    	HomeMaticDevice* currentDevice = nullptr;

    	char path[1024];
    	getcwd(path, 1024);
    	GD::workingDirectory = std::string(path);
		ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
		if (length == -1) throw new Exception("Could not get executable path.");
		path[length] = '\0';
		GD::executablePath = std::string(path);
		GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/"));
		cout << GD::executablePath << endl;
    	GD::db.init(GD::executablePath + "/db.sql");

        GD::cul.init("/dev/ttyACM0");
        cout << "Start listening for BidCoS packets..." << endl;
        GD::cul.startListening();
        cout << "Loading devices..." << endl;
        GD::devices.load(); //Don't load before database is open!
        cout << "Starting XML RPC server..." << endl;
        GD::xmlrpcServer.run();

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
            else if(input == "create device" || input == "add device")
            {
            	cout << "Please enter a 3 byte address for the device in hexadecimal format (e. g. 3A0001): ";
            	int32_t address = getHexInput();

            	if(address < 1 || address > 0xFFFFFF) cout << "Address not valid." << endl;
            	else if(GD::devices.get(address) != nullptr) cout << "Address already in use." << endl;
            	else
            	{
            		cout << "Please enter a serial number (length 10, e. g. VVD0000001): ";
            		cin >> input;
            		if(input.size() != 10) cout << "Serial number has wrong length." << endl;
            		else
            		{
            			std::string serialNumber = input;
            			cout << "Please enter a device type: ";
						int32_t deviceType = getHexInput();
						switch(deviceType)
						{
						case 0x39:
							GD::devices.add(new HM_CC_TC(serialNumber, address));
							cout << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << endl;
							break;
						case 0x3A:
							GD::devices.add(new HM_CC_VD(serialNumber, address));
							cout << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << endl;
							break;
						case 0xFFFFFFFE:
							GD::devices.add(new HM_SD(serialNumber, address));
							cout << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << endl;
							break;
						default:
							cout << "Unknown device type." << endl;
						}
            		}
            	}
            }
            else if(input == "remove device" || input == "delete device")
            {
            	cout << "Please enter the address of the device to delete (e. g. 3A0001): ";
            	int32_t address = getHexInput();
            	if(currentDevice != nullptr && currentDevice->address() == address) currentDevice = nullptr;
            	if(GD::devices.remove(address)) cout << "Device removed." << endl;
            	else cout << "Device not found." << endl;
            }
            else if(input == "select device")
            {
            	cout << "Device address: ";
            	int32_t address = getHexInput();
            	currentDevice = GD::devices.get(address);
            	if(currentDevice == nullptr) cout << "Device not found." << endl;
            	else cout << "Device selected." << endl;
            }
            else if(input == "list devices")
            {
            	std::vector<HomeMaticDevice*>* devices = GD::devices.getDevices();
            	for(std::vector<HomeMaticDevice*>::iterator i = devices->begin(); i != devices->end(); ++i)
            	{
            		cout << "Address: 0x" << std::hex << (*i)->address() << "\tSerial number: " << (*i)->serialNumber() << "\tDevice type: " << (*i)->deviceType() << endl << std::dec;
            	}
            }
            else if(input == "set verbosity")
            {
            	cout << "Verbosity (0 - 5): ";
            	int32_t verbosity = getHexInput();
            	if(verbosity < 0 || verbosity > 5) cout << "Invalid verbosity." << endl;
            	else
            	{
            		GD::debugLevel = verbosity;
            		cout << "Verbosity set to " << verbosity << endl;
            	}
            }
            else if(input == "time")
            {
            	cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << endl;
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

        GD::devices.save();
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
