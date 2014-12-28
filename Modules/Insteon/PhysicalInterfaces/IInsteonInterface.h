/* Copyright 2013-2015 Sathya Laufer
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

#ifndef IINSTEONINTERFACE_H_
#define IINSTEONINTERFACE_H_

#include "../../Base/BaseLib.h"

namespace Insteon {

class IInsteonInterface : public BaseLib::Systems::IPhysicalInterface
{
public:
	IInsteonInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
	virtual ~IInsteonInterface();

	virtual int32_t address() { return _myAddress; }

	virtual void addPeer(int32_t address) {}
	virtual void addPeers(std::vector<int32_t>& addresses) {}
	virtual void removePeer(int32_t address) {}

	virtual void enablePairingMode() = 0;
	virtual void disablePairingMode() = 0;
protected:
	class PeerInfo
	{
	public:
		PeerInfo() {}
		virtual ~PeerInfo() {}

		int32_t address = 0;
		uint8_t flagsResponder = 0;
		int32_t databaseAddressResponder = 0;
		uint8_t data1Responder = 0;
		uint8_t data2Responder = 0;
		uint8_t data3Responder = 0;
		uint8_t flagsController = 0;
		int32_t databaseAddressController = 0;
		uint8_t data1Controller = 0;
		uint8_t data2Controller = 0;
		uint8_t data3Controller = 0;
	};

	BaseLib::Output _out;
	int32_t _myAddress = 0xFFFFFF;
};

}

#endif
