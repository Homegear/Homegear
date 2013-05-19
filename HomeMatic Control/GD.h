#ifndef GD_H_
#define GD_H_

#include <vector>
#include <string>

#include "HomeMaticDevice.h"
#include "Cul.h"
#include "Database.h"

class HomeMaticDevice;
class Cul;

class GD {
public:
	static std::string startUpPath;
	static std::vector<HomeMaticDevice> devices;
	static Database* db;
	static Cul* cul;
	virtual ~GD() { delete db; }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
