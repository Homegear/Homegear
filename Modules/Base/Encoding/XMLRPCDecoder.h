/* Copyright 2013-2014 Sathya Laufer
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

#ifndef XMLRPCDECODER_H_
#define XMLRPCDECODER_H_

#include "../RPC/RPCVariable.h"
#include "rapidxml.hpp"

#include <memory>
#include <vector>

using namespace rapidxml;

namespace BaseLib
{
namespace RPC
{

class XMLRPCDecoder {
public:
	XMLRPCDecoder();
	virtual ~XMLRPCDecoder() {}

	virtual std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName);
	virtual std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<std::vector<char>> packet);
	virtual std::shared_ptr<RPCVariable> decodeResponse(std::string& packet);
private:
	std::shared_ptr<RPCVariable> decodeParameter(xml_node<>* valueNode);
	std::shared_ptr<RPCVariable> decodeArray(xml_node<>* dataNode);
	std::shared_ptr<RPCVariable> decodeStruct(xml_node<>* structNode);
	std::shared_ptr<RPCVariable> decodeResponse(xml_document<>* doc);
};

} /* namespace RPC */
}
#endif /* XMLRPCDECODER_H_ */
