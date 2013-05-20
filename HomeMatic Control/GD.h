#ifndef GD_H_
#define GD_H_

#include <vector>
#include <string>

#include "Database.h"
#include "Cul.h"
#include "HomeMaticDevices.h"

class Cul;
class Database;
class HomeMaticDevices;

class GD {
public:
	static std::string startUpPath;
	static HomeMaticDevices devices;
	static Database db;
	static Cul cul;
	static int32_t debugLevel;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
