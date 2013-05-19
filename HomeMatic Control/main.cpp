using namespace std;

#include <algorithm>
#include <ncurses.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

#include "Cul.h"
#include "HM-RC-Sec3-B.h"
#include "HM-SD.h"
#include "HM-CC-VD.h"
#include "HM-CC-TC.h"
#include "HomeMaticDevice.h"
#include "Database.h"
#include "GD.h"

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
	try	{ intInput = std::stoi(input); } catch(...) {}
    return intInput;
}

int32_t getHexInput()
{
	std::string input;
	cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoi(input, 0, 16); } catch(...) {}
    return intInput;
}

int main()
{
    try
    {
    	signal(SIGSEGV, exceptionHandler);

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
    	GD::startUpPath = std::string(path);
    	GD::db = new Database(GD::startUpPath + "/db.sql");
        GD::db->executeCommand("CREATE TABLE IF NOT EXISTS devices (address INTEGER, serializedObject TEXT, lastDutyCycle INTEGER)");
        //GD::db->executeCommand("INSERT INTO bla VALUES('Hallo')");
        //std::vector<std::vector<DataColumn>> rows = GD::db->executeCommand("SELECT * FROM bla");
        //cout << rows[0][0].textValue << endl;

        GD::cul = new Cul("/dev/ttyACM0");
        //HM_RC_Sec3_B* rc = new HM_RC_Sec3_B(cul, "RC_Sathya1", 0x1DDD0F, 0, 0, 0);
        HM_SD* sd = new HM_SD();
        HM_CC_VD* vd1 = new HM_CC_VD("VD_Sathya1", 0x3A0001);
        HM_CC_VD* vd2 = new HM_CC_VD("VVD0000002", 0x3A0002);
        HM_CC_VD* vd3 = new HM_CC_VD("VVD0000003", 0x3A0003);
        HM_CC_TC* tc = new HM_CC_TC("VTC0000001", 0x390001);

        //sd->addFilter(FilterType::SenderAddress, 0x1E53E7);
        //sd->addFilter(FilterType::DestinationAddress, 0x1E53E7);

        sd->addFilter(FilterType::SenderAddress, 0x390001);
        sd->addFilter(FilterType::DestinationAddress, 0x390001);

        sd->addFilter(FilterType::SenderAddress, 0x1DA44D); //Stellantrieb Preetz 1. OG Zimmer NW
        sd->addFilter(FilterType::DestinationAddress, 0x1DA44D); //Stellantrieb Preetz 1. OG Zimmer NW

        //sd->addFilter(FilterType::SenderAddress, 0x1D8DDD); //Wandthermostat Kiel Zimmer NW
        //sd->addFilter(FilterType::DestinationAddress, 0x1D8DDD); //Wandthermostat Kiel Zimmer NW

        //sd->addFilter(FilterType::SenderAddress, 0x1D8F45); //Wandthermostat Preetz 1. OG Zimmer NW
        //sd->addFilter(FilterType::DestinationAddress, 0x1D8F45); //Wandthermostat Preetz 1. OG Zimmer NW

        /*sd->addFilter(FilterType::DestinationAddress, 0x3A0001);
        sd->addFilter(FilterType::DestinationAddress, 0x3A0002);
        sd->addFilter(FilterType::DestinationAddress, 0x3A0003);
        sd->addFilter(FilterType::DestinationAddress, 0x1DDD0F);
        sd->addFilter(FilterType::SenderAddress, 0x1F454D);
        sd->addFilter(FilterType::DestinationAddress, 0x1F454D);*/
        //sd->addOverwriteResponse("A0111C69431F454D0201000000", "A0021F454D1C69430400000000000002", 80);
        //sd->addBlockResponse("A0031C69431F454D", 50);
        GD::cul->startListening();

        std::string input = "";
        while(input != "q")
        {
            std::cin >> input;
            if(input == "h" || input == "?" || input == "help")
            {
                //Help
            }
            if(input == "a")
            {
            	cout << "Please enter a 3 byte address for the device in hexadecimal format (e. g. 3A0001): ";
            	cin >> input;
            	int32_t address = getHexInput();
            	if(address < 1 || address > 0xFFFFFF) cout << "Address not valid." << endl;
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
						HM_CC_TC tc(serialNumber, address);
						switch(deviceType)
						{
						case 0x39:
							GD::devices->push_back((HomeMaticDevice)tc);
							break;
						case 0x3A:
							//GD::devices->push_back(HM_CC_VD(serialNumber, address));
							break;
						default:
							cout << "Unknown device type." << endl;
						}
            		}
            	}
            }
            if(input == "s")
            {
            	cout << "Device address: ";
            	int32_t address = getHexInput();
            	currentDevice = nullptr;
            	for(std::vector<HomeMaticDevice>::iterator i = GD::devices->begin(); i != GD::devices->end(); ++i)
				{
					if(i->getAddress() == address)
					{
						currentDevice = &(*i);
						break;
					}
				}
            	if(currentDevice == nullptr) cout << "Device not found." << endl;
            	else cout << "Device selected." << endl;
            }
            if(input == "p1")
            {
                if(vd1->pairDevice(10000))
                {
                    cout << "Pairing successful" << '\n';
                }
                else
                {
                    cout << "Pairing not successful" << '\n';
                }
            }
            if(input == "p2")
            {
                if(tc->pairDevice(10000))
                {
                    cout << "Pairing successful" << '\n';
                }
                else
                {
                    cout << "Pairing not successful" << '\n';
                }
            }
            if(input == "p3")
            {
                if(vd3->pairDevice(10000))
                {
                    cout << "Pairing successful" << '\n';
                }
                else
                {
                    cout << "Pairing not successful" << '\n';
                }
            }
        }

        return 0;
    }
    catch(Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
}
