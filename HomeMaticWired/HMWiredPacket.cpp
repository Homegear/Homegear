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

#include "HMWiredPacket.h"
#include "../Exception.h"
#include "../HelperFunctions.h"

namespace HMWired
{

HMWiredPacket::HMWiredPacket()
{
	init();
}

HMWiredPacket::HMWiredPacket(std::vector<uint8_t>& packet, int64_t timeReceived)
{
	init();
	_timeReceived = timeReceived;
	import(packet);
}

HMWiredPacket::~HMWiredPacket()
{
}

void HMWiredPacket::init()
{
	initCRCTable();
}

void HMWiredPacket::initCRCTable()
{
	    _crcTable[0] = 0x0;
    _crcTable[1] = 0xC1;
    _crcTable[2] = 0x81;
    _crcTable[3] = 0x40;
    _crcTable[4] = 0x1;
    _crcTable[5] = 0xC0;
    _crcTable[6] = 0x80;
    _crcTable[7] = 0x41;
    _crcTable[8] = 0x1;
    _crcTable[9] = 0xC0;
    _crcTable[10] = 0x80;
    _crcTable[11] = 0x41;
    _crcTable[12] = 0x0;
    _crcTable[13] = 0xC1;
    _crcTable[14] = 0x81;
    _crcTable[15] = 0x40;
    _crcTable[16] = 0x1;
    _crcTable[17] = 0xC0;
    _crcTable[18] = 0x80;
    _crcTable[19] = 0x41;
    _crcTable[20] = 0x0;
    _crcTable[21] = 0xC1;
    _crcTable[22] = 0x81;
    _crcTable[23] = 0x40;
    _crcTable[24] = 0x0;
    _crcTable[25] = 0xC1;
    _crcTable[26] = 0x81;
    _crcTable[27] = 0x40;
    _crcTable[28] = 0x1;
    _crcTable[29] = 0xC0;
    _crcTable[30] = 0x80;
    _crcTable[31] = 0x41;
    _crcTable[32] = 0x1;
    _crcTable[33] = 0xC0;
    _crcTable[34] = 0x80;
    _crcTable[35] = 0x41;
    _crcTable[36] = 0x0;
    _crcTable[37] = 0xC1;
    _crcTable[38] = 0x81;
    _crcTable[39] = 0x40;
    _crcTable[40] = 0x0;
    _crcTable[41] = 0xC1;
    _crcTable[42] = 0x81;
    _crcTable[43] = 0x40;
    _crcTable[44] = 0x1;
    _crcTable[45] = 0xC0;
    _crcTable[46] = 0x80;
    _crcTable[47] = 0x41;
    _crcTable[48] = 0x0;
    _crcTable[49] = 0xC1;
    _crcTable[50] = 0x81;
    _crcTable[51] = 0x40;
    _crcTable[52] = 0x1;
    _crcTable[53] = 0xC0;
    _crcTable[54] = 0x80;
    _crcTable[55] = 0x41;
    _crcTable[56] = 0x1;
    _crcTable[57] = 0xC0;
    _crcTable[58] = 0x80;
    _crcTable[59] = 0x41;
    _crcTable[60] = 0x0;
    _crcTable[61] = 0xC1;
    _crcTable[62] = 0x81;
    _crcTable[63] = 0x40;
    _crcTable[64] = 0x1;
    _crcTable[65] = 0xC0;
    _crcTable[66] = 0x80;
    _crcTable[67] = 0x41;
    _crcTable[68] = 0x0;
    _crcTable[69] = 0xC1;
    _crcTable[70] = 0x81;
    _crcTable[71] = 0x40;
    _crcTable[72] = 0x0;
    _crcTable[73] = 0xC1;
    _crcTable[74] = 0x81;
    _crcTable[75] = 0x40;
    _crcTable[76] = 0x1;
    _crcTable[77] = 0xC0;
    _crcTable[78] = 0x80;
    _crcTable[79] = 0x41;
    _crcTable[80] = 0x0;
    _crcTable[81] = 0xC1;
    _crcTable[82] = 0x81;
    _crcTable[83] = 0x40;
    _crcTable[84] = 0x1;
    _crcTable[85] = 0xC0;
    _crcTable[86] = 0x80;
    _crcTable[87] = 0x41;
    _crcTable[88] = 0x1;
    _crcTable[89] = 0xC0;
    _crcTable[90] = 0x80;
    _crcTable[91] = 0x41;
    _crcTable[92] = 0x0;
    _crcTable[93] = 0xC1;
    _crcTable[94] = 0x81;
    _crcTable[95] = 0x40;
    _crcTable[96] = 0x0;
    _crcTable[97] = 0xC1;
    _crcTable[98] = 0x81;
    _crcTable[99] = 0x40;
    _crcTable[100] = 0x1;
    _crcTable[101] = 0xC0;
    _crcTable[102] = 0x80;
    _crcTable[103] = 0x41;
    _crcTable[104] = 0x1;
    _crcTable[105] = 0xC0;
    _crcTable[106] = 0x80;
    _crcTable[107] = 0x41;
    _crcTable[108] = 0x0;
    _crcTable[109] = 0xC1;
    _crcTable[110] = 0x81;
    _crcTable[111] = 0x40;
    _crcTable[112] = 0x1;
    _crcTable[113] = 0xC0;
    _crcTable[114] = 0x80;
    _crcTable[115] = 0x41;
    _crcTable[116] = 0x0;
    _crcTable[117] = 0xC1;
    _crcTable[118] = 0x81;
    _crcTable[119] = 0x40;
    _crcTable[120] = 0x0;
    _crcTable[121] = 0xC1;
    _crcTable[122] = 0x81;
    _crcTable[123] = 0x40;
    _crcTable[124] = 0x1;
    _crcTable[125] = 0xC0;
    _crcTable[126] = 0x80;
    _crcTable[127] = 0x41;
    _crcTable[128] = 0x1;
    _crcTable[129] = 0xC0;
    _crcTable[130] = 0x80;
    _crcTable[131] = 0x41;
    _crcTable[132] = 0x0;
    _crcTable[133] = 0xC1;
    _crcTable[134] = 0x81;
    _crcTable[135] = 0x40;
    _crcTable[136] = 0x0;
    _crcTable[137] = 0xC1;
    _crcTable[138] = 0x81;
    _crcTable[139] = 0x40;
    _crcTable[140] = 0x1;
    _crcTable[141] = 0xC0;
    _crcTable[142] = 0x80;
    _crcTable[143] = 0x41;
    _crcTable[144] = 0x0;
    _crcTable[145] = 0xC1;
    _crcTable[146] = 0x81;
    _crcTable[147] = 0x40;
    _crcTable[148] = 0x1;
    _crcTable[149] = 0xC0;
    _crcTable[150] = 0x80;
    _crcTable[151] = 0x41;
    _crcTable[152] = 0x1;
    _crcTable[153] = 0xC0;
    _crcTable[154] = 0x80;
    _crcTable[155] = 0x41;
    _crcTable[156] = 0x0;
    _crcTable[157] = 0xC1;
    _crcTable[158] = 0x81;
    _crcTable[159] = 0x40;
    _crcTable[160] = 0x0;
    _crcTable[161] = 0xC1;
    _crcTable[162] = 0x81;
    _crcTable[163] = 0x40;
    _crcTable[164] = 0x1;
    _crcTable[165] = 0xC0;
    _crcTable[166] = 0x80;
    _crcTable[167] = 0x41;
    _crcTable[168] = 0x1;
    _crcTable[169] = 0xC0;
    _crcTable[170] = 0x80;
    _crcTable[171] = 0x41;
    _crcTable[172] = 0x0;
    _crcTable[173] = 0xC1;
    _crcTable[174] = 0x81;
    _crcTable[175] = 0x40;
    _crcTable[176] = 0x1;
    _crcTable[177] = 0xC0;
    _crcTable[178] = 0x80;
    _crcTable[179] = 0x41;
    _crcTable[180] = 0x0;
    _crcTable[181] = 0xC1;
    _crcTable[182] = 0x81;
    _crcTable[183] = 0x40;
    _crcTable[184] = 0x0;
    _crcTable[185] = 0xC1;
    _crcTable[186] = 0x81;
    _crcTable[187] = 0x40;
    _crcTable[188] = 0x1;
    _crcTable[189] = 0xC0;
    _crcTable[190] = 0x80;
    _crcTable[191] = 0x41;
    _crcTable[192] = 0x0;
    _crcTable[193] = 0xC1;
    _crcTable[194] = 0x81;
    _crcTable[195] = 0x40;
    _crcTable[196] = 0x1;
    _crcTable[197] = 0xC0;
    _crcTable[198] = 0x80;
    _crcTable[199] = 0x41;
    _crcTable[200] = 0x1;
    _crcTable[201] = 0xC0;
    _crcTable[202] = 0x80;
    _crcTable[203] = 0x41;
    _crcTable[204] = 0x0;
    _crcTable[205] = 0xC1;
    _crcTable[206] = 0x81;
    _crcTable[207] = 0x40;
    _crcTable[208] = 0x1;
    _crcTable[209] = 0xC0;
    _crcTable[210] = 0x80;
    _crcTable[211] = 0x41;
    _crcTable[212] = 0x0;
    _crcTable[213] = 0xC1;
    _crcTable[214] = 0x81;
    _crcTable[215] = 0x40;
    _crcTable[216] = 0x0;
    _crcTable[217] = 0xC1;
    _crcTable[218] = 0x81;
    _crcTable[219] = 0x40;
    _crcTable[220] = 0x1;
    _crcTable[221] = 0xC0;
    _crcTable[222] = 0x80;
    _crcTable[223] = 0x41;
    _crcTable[224] = 0x1;
    _crcTable[225] = 0xC0;
    _crcTable[226] = 0x80;
    _crcTable[227] = 0x41;
    _crcTable[228] = 0x0;
    _crcTable[229] = 0xC1;
    _crcTable[230] = 0x81;
    _crcTable[231] = 0x40;
    _crcTable[232] = 0x0;
    _crcTable[233] = 0xC1;
    _crcTable[234] = 0x81;
    _crcTable[235] = 0x40;
    _crcTable[236] = 0x1;
    _crcTable[237] = 0xC0;
    _crcTable[238] = 0x80;
    _crcTable[239] = 0x41;
    _crcTable[240] = 0x0;
    _crcTable[241] = 0xC1;
    _crcTable[242] = 0x81;
    _crcTable[243] = 0x40;
    _crcTable[244] = 0x1;
    _crcTable[245] = 0xC0;
    _crcTable[246] = 0x80;
    _crcTable[247] = 0x41;
    _crcTable[248] = 0x1;
    _crcTable[249] = 0xC0;
    _crcTable[250] = 0x80;
    _crcTable[251] = 0x41;
    _crcTable[252] = 0x0;
    _crcTable[253] = 0xC1;
    _crcTable[254] = 0x81;
    _crcTable[255] = 0x40;
    _crcTable[256] = 0x0;
    _crcTable[257] = 0xC0;
    _crcTable[258] = 0xC1;
    _crcTable[259] = 0x1;
    _crcTable[260] = 0xC3;
    _crcTable[261] = 0x3;
    _crcTable[262] = 0x2;
    _crcTable[263] = 0xC2;
    _crcTable[264] = 0xC6;
    _crcTable[265] = 0x6;
    _crcTable[266] = 0x7;
    _crcTable[267] = 0xC7;
    _crcTable[268] = 0x5;
    _crcTable[269] = 0xC5;
    _crcTable[270] = 0xC4;
    _crcTable[271] = 0x4;
    _crcTable[272] = 0xCC;
    _crcTable[273] = 0xC;
    _crcTable[274] = 0xD;
    _crcTable[275] = 0xCD;
    _crcTable[276] = 0xF;
    _crcTable[277] = 0xCF;
    _crcTable[278] = 0xCE;
    _crcTable[279] = 0xE;
    _crcTable[280] = 0xA;
    _crcTable[281] = 0xCA;
    _crcTable[282] = 0xCB;
    _crcTable[283] = 0xB;
    _crcTable[284] = 0xC9;
    _crcTable[285] = 0x9;
    _crcTable[286] = 0x8;
    _crcTable[287] = 0xC8;
    _crcTable[288] = 0xD8;
    _crcTable[289] = 0x18;
    _crcTable[290] = 0x19;
    _crcTable[291] = 0xD9;
    _crcTable[292] = 0x1B;
    _crcTable[293] = 0xDB;
    _crcTable[294] = 0xDA;
    _crcTable[295] = 0x1A;
    _crcTable[296] = 0x1E;
    _crcTable[297] = 0xDE;
    _crcTable[298] = 0xDF;
    _crcTable[299] = 0x1F;
    _crcTable[300] = 0xDD;
    _crcTable[301] = 0x1D;
    _crcTable[302] = 0x1C;
    _crcTable[303] = 0xDC;
    _crcTable[304] = 0x14;
    _crcTable[305] = 0xD4;
    _crcTable[306] = 0xD5;
    _crcTable[307] = 0x15;
    _crcTable[308] = 0xD7;
    _crcTable[309] = 0x17;
    _crcTable[310] = 0x16;
    _crcTable[311] = 0xD6;
    _crcTable[312] = 0xD2;
    _crcTable[313] = 0x12;
    _crcTable[314] = 0x13;
    _crcTable[315] = 0xD3;
    _crcTable[316] = 0x11;
    _crcTable[317] = 0xD1;
    _crcTable[318] = 0xD0;
    _crcTable[319] = 0x10;
    _crcTable[320] = 0xF0;
    _crcTable[321] = 0x30;
    _crcTable[322] = 0x31;
    _crcTable[323] = 0xF1;
    _crcTable[324] = 0x33;
    _crcTable[325] = 0xF3;
    _crcTable[326] = 0xF2;
    _crcTable[327] = 0x32;
    _crcTable[328] = 0x36;
    _crcTable[329] = 0xF6;
    _crcTable[330] = 0xF7;
    _crcTable[331] = 0x37;
    _crcTable[332] = 0xF5;
    _crcTable[333] = 0x35;
    _crcTable[334] = 0x34;
    _crcTable[335] = 0xF4;
    _crcTable[336] = 0x3C;
    _crcTable[337] = 0xFC;
    _crcTable[338] = 0xFD;
    _crcTable[339] = 0x3D;
    _crcTable[340] = 0xFF;
    _crcTable[341] = 0x3F;
    _crcTable[342] = 0x3E;
    _crcTable[343] = 0xFE;
    _crcTable[344] = 0xFA;
    _crcTable[345] = 0x3A;
    _crcTable[346] = 0x3B;
    _crcTable[347] = 0xFB;
    _crcTable[348] = 0x39;
    _crcTable[349] = 0xF9;
    _crcTable[350] = 0xF8;
    _crcTable[351] = 0x38;
    _crcTable[352] = 0x28;
    _crcTable[353] = 0xE8;
    _crcTable[354] = 0xE9;
    _crcTable[355] = 0x29;
    _crcTable[356] = 0xEB;
    _crcTable[357] = 0x2B;
    _crcTable[358] = 0x2A;
    _crcTable[359] = 0xEA;
    _crcTable[360] = 0xEE;
    _crcTable[361] = 0x2E;
    _crcTable[362] = 0x2F;
    _crcTable[363] = 0xEF;
    _crcTable[364] = 0x2D;
    _crcTable[365] = 0xED;
    _crcTable[366] = 0xEC;
    _crcTable[367] = 0x2C;
    _crcTable[368] = 0xE4;
    _crcTable[369] = 0x24;
    _crcTable[370] = 0x25;
    _crcTable[371] = 0xE5;
    _crcTable[372] = 0x27;
    _crcTable[373] = 0xE7;
    _crcTable[374] = 0xE6;
    _crcTable[375] = 0x26;
    _crcTable[376] = 0x22;
    _crcTable[377] = 0xE2;
    _crcTable[378] = 0xE3;
    _crcTable[379] = 0x23;
    _crcTable[380] = 0xE1;
    _crcTable[381] = 0x21;
    _crcTable[382] = 0x20;
    _crcTable[383] = 0xE0;
    _crcTable[384] = 0xA0;
    _crcTable[385] = 0x60;
    _crcTable[386] = 0x61;
    _crcTable[387] = 0xA1;
    _crcTable[388] = 0x63;
    _crcTable[389] = 0xA3;
    _crcTable[390] = 0xA2;
    _crcTable[391] = 0x62;
    _crcTable[392] = 0x66;
    _crcTable[393] = 0xA6;
    _crcTable[394] = 0xA7;
    _crcTable[395] = 0x67;
    _crcTable[396] = 0xA5;
    _crcTable[397] = 0x65;
    _crcTable[398] = 0x64;
    _crcTable[399] = 0xA4;
    _crcTable[400] = 0x6C;
    _crcTable[401] = 0xAC;
    _crcTable[402] = 0xAD;
    _crcTable[403] = 0x6D;
    _crcTable[404] = 0xAF;
    _crcTable[405] = 0x6F;
    _crcTable[406] = 0x6E;
    _crcTable[407] = 0xAE;
    _crcTable[408] = 0xAA;
    _crcTable[409] = 0x6A;
    _crcTable[410] = 0x6B;
    _crcTable[411] = 0xAB;
    _crcTable[412] = 0x69;
    _crcTable[413] = 0xA9;
    _crcTable[414] = 0xA8;
    _crcTable[415] = 0x68;
    _crcTable[416] = 0x78;
    _crcTable[417] = 0xB8;
    _crcTable[418] = 0xB9;
    _crcTable[419] = 0x79;
    _crcTable[420] = 0xBB;
    _crcTable[421] = 0x7B;
    _crcTable[422] = 0x7A;
    _crcTable[423] = 0xBA;
    _crcTable[424] = 0xBE;
    _crcTable[425] = 0x7E;
    _crcTable[426] = 0x7F;
    _crcTable[427] = 0xBF;
    _crcTable[428] = 0x7D;
    _crcTable[429] = 0xBD;
    _crcTable[430] = 0xBC;
    _crcTable[431] = 0x7C;
    _crcTable[432] = 0xB4;
    _crcTable[433] = 0x74;
    _crcTable[434] = 0x75;
    _crcTable[435] = 0xB5;
    _crcTable[436] = 0x77;
    _crcTable[437] = 0xB7;
    _crcTable[438] = 0xB6;
    _crcTable[439] = 0x76;
    _crcTable[440] = 0x72;
    _crcTable[441] = 0xB2;
    _crcTable[442] = 0xB3;
    _crcTable[443] = 0x73;
    _crcTable[444] = 0xB1;
    _crcTable[445] = 0x71;
    _crcTable[446] = 0x70;
    _crcTable[447] = 0xB0;
    _crcTable[448] = 0x50;
    _crcTable[449] = 0x90;
    _crcTable[450] = 0x91;
    _crcTable[451] = 0x51;
    _crcTable[452] = 0x93;
    _crcTable[453] = 0x53;
    _crcTable[454] = 0x52;
    _crcTable[455] = 0x92;
    _crcTable[456] = 0x96;
    _crcTable[457] = 0x56;
    _crcTable[458] = 0x57;
    _crcTable[459] = 0x97;
    _crcTable[460] = 0x55;
    _crcTable[461] = 0x95;
    _crcTable[462] = 0x94;
    _crcTable[463] = 0x54;
    _crcTable[464] = 0x9C;
    _crcTable[465] = 0x5C;
    _crcTable[466] = 0x5D;
    _crcTable[467] = 0x9D;
    _crcTable[468] = 0x5F;
    _crcTable[469] = 0x9F;
    _crcTable[470] = 0x9E;
    _crcTable[471] = 0x5E;
    _crcTable[472] = 0x5A;
    _crcTable[473] = 0x9A;
    _crcTable[474] = 0x9B;
    _crcTable[475] = 0x5B;
    _crcTable[476] = 0x99;
    _crcTable[477] = 0x59;
    _crcTable[478] = 0x58;
    _crcTable[479] = 0x98;
    _crcTable[480] = 0x88;
    _crcTable[481] = 0x48;
    _crcTable[482] = 0x49;
    _crcTable[483] = 0x89;
    _crcTable[484] = 0x4B;
    _crcTable[485] = 0x8B;
    _crcTable[486] = 0x8A;
    _crcTable[487] = 0x4A;
    _crcTable[488] = 0x4E;
    _crcTable[489] = 0x8E;
    _crcTable[490] = 0x8F;
    _crcTable[491] = 0x4F;
    _crcTable[492] = 0x8D;
    _crcTable[493] = 0x4D;
    _crcTable[494] = 0x4C;
    _crcTable[495] = 0x8C;
    _crcTable[496] = 0x44;
    _crcTable[497] = 0x84;
    _crcTable[498] = 0x85;
    _crcTable[499] = 0x45;
    _crcTable[500] = 0x87;
    _crcTable[501] = 0x47;
    _crcTable[502] = 0x46;
    _crcTable[503] = 0x86;
    _crcTable[504] = 0x82;
    _crcTable[505] = 0x42;
    _crcTable[506] = 0x43;
    _crcTable[507] = 0x83;
    _crcTable[508] = 0x41;
    _crcTable[509] = 0x81;
    _crcTable[510] = 0x80;
    _crcTable[511] = 0x40;
}

void HMWiredPacket::import(std::vector<uint8_t>& packet)
{
	try
	{
		HelperFunctions::printMessage(HelperFunctions::getHexString(packet));
		if(packet.empty()) return;
		if(packet.at(0) == 0xFD)
		{
			if(packet.size() < 11)
			{
				HelperFunctions::printError("HomeMatic Wired packet of type 0xFD is smaller than 11 bytes: " + HelperFunctions::getHexString(packet));
				return;
			}
			if(packet[0] != 0xFD && packet[0] != 0xFE)
			{
				HelperFunctions::printError("HomeMatic Wired packet has invalid start byte: " + HelperFunctions::getHexString(packet));
				return;
			}
			_length = packet[10]; //Frame length
			if(_length > 64 || _length + 11 > packet.size())
			{
				HelperFunctions::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
				return;
			}
			_type = HMWiredPacketType::FD;
			_controlByte = packet[5];
			_senderAddress = (packet[6] << 24) + (packet[7] << 16) + (packet[8] << 8) + packet[9];
			_destinationAddress = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
			if(_length >= 2)
			{
				_payload.clear();
				_payload.insert(_payload.end(), packet.begin() + 11, packet.end() - 2);
				_checksum.clear();
				_checksum.insert(_checksum.end(), packet.end() - 2, packet.end());
			}
		}
		else if(packet.at(0) == 0xFE)
		{
			_type = HMWiredPacketType::FE;
		}
		else if(packet.at(0) == 0xF8)
		{
			_type = HMWiredPacketType::F8;
		}
		else
		{
			HelperFunctions::printError("HomeMatic Wired packet of unknown type received: " + HelperFunctions::getHexString(packet));
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredPacket::import(std::string packetHex)
{
	try
	{

	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint16_t HMWiredPacket::crc16(std::vector<uint8_t>& data)
{
	HelperFunctions::printMessage("Data: " + HelperFunctions::getHexString(data));
	uint16_t crc = 0xFFFF;

    //for(uint32_t k = 0; k <= 0xFFFF; k++)
    //{
    	//crc = 0xFFFF;
    	//uint16_t polinom = k;
		//for(int32_t j = data.size() - 1; j >= 0; j--)
		for(uint32_t j = 0; j < data.size(); j++)
		{
			crc ^= data.at(j);
			for (uint32_t i = 0; i < 8; i++)
			{
				//if(crc & 0x8000) crc = ((crc << 1) & 0xFFFF) ^ 0x1002;
				//else crc = ((crc << 1) & 0xFFFF);
				if(crc & 1) crc = (crc >> 1) ^ 0x1002;
				else crc >>= 1;
				//if(crc & 1) crc = (crc >> 1) ^ polinom;
				//else crc >>= 1;
			}
		}
		//if(crc == (uint16_t)0x7518 || ~crc == (uint16_t)0x7518)
		//{
		//	HelperFunctions::printError("Polinom: " + HelperFunctions::getHexString(polinom, 4));
		//}
    //}
	HelperFunctions::printError("CRC: " + HelperFunctions::getHexString(crc, 4));
    HelperFunctions::printError("Inverse CRC: " + HelperFunctions::getHexString((uint16_t)~crc, 4));

    return crc;
}

} /* namespace HMWired */
