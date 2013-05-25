#ifndef PHYSICALPARAMETER_H_
#define PHYSICALPARAMETER_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include "rapidxml.hpp"

using namespace rapidxml;

namespace XMLRPC {

class PhysicalParameter
{
public:
	struct Type
	{
		enum Enum { none, typeBoolean, typeInteger, typeOption };
	};
	struct Interface
	{
		enum Enum { none, command, internal };
	};
	Type::Enum type() { return _type; }
	Interface::Enum interface = Interface::none;
	uint32_t list = 0;
	double index = 0;
	double size = 0;
	std::string valueID = "";
	bool noInit = false;
	std::string getRequest = "";
	std::string setRequest = "";
	std::vector<std::string> eventFrames;

	PhysicalParameter() {}
	PhysicalParameter(xml_node<>* node);
	virtual ~PhysicalParameter() {}
protected:
	Type::Enum _type = Type::Enum::none;
};
} /* namespace XMLRPC */
#endif /* PHYSICALPARAMETER_H_ */
