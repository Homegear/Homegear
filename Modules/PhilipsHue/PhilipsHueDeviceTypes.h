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

#ifndef PHILIPSHUEDEVICETYPES_H_
#define PHILIPSHUEDEVICETYPES_H_

#include <stdint.h>

namespace PhilipsHue
{
	enum class DeviceType : uint32_t
	{
		none = 					0xFFFFFFFF,
		HMRCV50 = 				0x0000,
		CENTRAL = 				0xFFFFFFFD,
		LCT001 =				0x0001,
		LLC001 =				0x0101,
		LLC006 =				0x0106,
		LLC007 =				0x0107,
		LLC011 =				0x0111,
		LST001 =				0x0201
	};
}
#endif
