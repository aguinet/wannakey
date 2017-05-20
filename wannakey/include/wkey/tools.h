// Copyright 2017 Adrien Guinet <adrien@guinet.me>
// This file is part of wannakey.
// 
// wannakey is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// wannakey is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with wannakey.  If not, see <http://www.gnu.org/licenses/>.

#ifndef WKEY_TOOLS_H
#define WKEY_TOOLS_H

#include <vector>
#include <system_error>

namespace wkey {

void dumpHex(const char* Name, uint8_t const* Data, size_t const Len);
double normalizedEntropy(uint8_t const* Data, const size_t Len);
std::vector<uint8_t> readFile(const char* path, std::error_code& EC);
bool fileHasString(const char* path, const char* str);

std::error_code getLastEC();
std::string getLastErrorMsg();
std::error_code getLastErrno();

} // wkey

#endif WKEY_TOOLS_H
