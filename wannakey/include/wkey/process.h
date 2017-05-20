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

#ifndef WKEY_PROCESS_H
#define WKEY_PROCESS_H

#include <string>
#include <cstdint>
#include <map>

#include <wkey/filesystem.h>

namespace wkey {

struct ProcInfo {
  std::string FullPath;
  uint32_t Pid;
};

typedef std::map<FileID, ProcInfo> MapFilesPID;

MapFilesPID getProcessList();
uint32_t getPIDByPath(MapFilesPID const& PIDs, const char* Path);
std::string getProcessPath(MapFilesPID const& Procs, uint32_t Pid);

} // wkey

#endif
