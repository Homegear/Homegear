#ifndef PHYSICALPARAMETER_H_
#define PHYSICALPARAMETER_H_

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

#include "rapidxml.hpp"

using namespace rapidxml;

namespace RPC {

class PhysicalParameterEvent
{
public:
	std::string frame;
	bool dominoEvent = false;
	int32_t dominoEventValue;
	std::string dominoEventDelayID;

	PhysicalParameterEvent() {}
	virtual ~PhysicalParameterEvent() {}
};

class PhysicalParameter
{
public:
	struct Type
	{
		enum Enum { none, typeBoolean, typeInteger, typeOption, typeString };
	};
	struct Interface
	{
		enum Enum { none, command, centralCommand, internal, config, configString, store };
	};
	Type::Enum type = Type::Enum::none;
	Interface::Enum interface = Interface::none;
	uint32_t list = 9999;
	double index = 0;
	uint32_t startIndex = 0;
	uint32_t endIndex = 0;
	bool sizeDefined = false;
	double size = 1.0;
	int32_t mask = -1;
	std::string valueID;
	bool noInit = false;
	std::string getRequest;
	std::string setRequest;
	std::string counter;
	std::vector<std::shared_ptr<PhysicalParameterEvent>> eventFrames;
	std::vector<std::string> resetAfterSend;
	bool isVolatile = false;
	std::string id;

	PhysicalParameter() {}
	PhysicalParameter(xml_node<>* node);
	virtual ~PhysicalParameter() {}
};
} /* namespace XMLRPC */
#endif /* PHYSICALPARAMETER_H_ */
