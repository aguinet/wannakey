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

#include <Windows.h>

#include <string>
#include <cstdint>
#include <map>
#include <functional>

#include <wkey/filesystem.h>

namespace wkey {

struct ProcInfo {
  std::string FullPath;
  uint32_t Pid;
};

typedef std::map<FileID, ProcInfo> MapFilesPID;
typedef std::function<bool(MEMORY_BASIC_INFORMATION const&)> ValidCb;
typedef std::function<bool(uint8_t const* Buf, const size_t Data, uint8_t const* OrgPtr)> MemoryCb;
typedef std::function<bool(uint8_t const* Buf, const size_t Data, std::error_code const&)> ReadFailureCB;

MapFilesPID getProcessList();
uint32_t getPIDByPath(MapFilesPID const& PIDs, const char* Path);
std::string getProcessPath(MapFilesPID const& Procs, uint32_t Pid);

std::error_code walkProcessMemory(uint32_t PID, ValidCb const&, MemoryCb const& Cb, ReadFailureCB const* CbRF = nullptr);
std::error_code walkProcessPrivateRWMemory(uint32_t PID, MemoryCb const& Cb, ReadFailureCB const* CbRF = nullptr);

std::error_code walkProcessMemory(HANDLE hProc, ValidCb const&, MemoryCb const& Cb, ReadFailureCB const* CbRF = nullptr);
std::error_code walkProcessPrivateRWMemory(HANDLE hProc, MemoryCb const& Cb, ReadFailureCB const* CbRF = nullptr);

} // wkey

#endif
