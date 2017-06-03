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

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#include <iostream>

#include <wkey/process.h>
#include <wkey/tools.h>

namespace wkey {

// Adapted from http://stackoverflow.com/questions/20874381/get-a-process-id-in-c-by-name
MapFilesPID getProcessList()
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  uint32_t result = NULL;

  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE == hProcessSnap) {
    std::cerr << "Unable to create process list snapshot!" << std::endl;
    return{};
  }

  pe32.dwSize = sizeof(MODULEENTRY32);

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if (!Process32First(hProcessSnap, &pe32))
  {
    CloseHandle(hProcessSnap);          // clean the snapshot object
    std::cerr << "Failed to gather information on system processes!" << std::endl;
    return{};
  }

  MapFilesPID Ret;

  do
  {
    const uint32_t pid = pe32.th32ProcessID;
    char Path[MAX_PATH + 1];
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProc == NULL) {
      auto Err = getLastEC();
      if (Err && Err.value() != ERROR_ACCESS_DENIED) {
        std::cerr << "Warning: unable to open process " << pid << ": " << Err.message() << std::endl;
      }
      continue;
    }
    uint32_t size = MAX_PATH;
    if (!GetModuleFileNameEx(hProc, NULL, Path, MAX_PATH)) {
      std::cerr << "Warning: unable to retrieve the full path of the process for PID " << pid << ": " << getLastErrorMsg() << std::endl;
      continue;
    }
    CloseHandle(hProc);
    FileID fid = getFileID(Path);
    if (fid == FileIDInvalid) {
      continue;
    }

    Ret.insert(std::make_pair(fid, ProcInfo{ std::string{Path}, pe32.th32ProcessID }));
  } while (Process32Next(hProcessSnap, &pe32));

  CloseHandle(hProcessSnap);

  return Ret;
}

uint32_t getPIDByPath(MapFilesPID const& PIDs, const char* Path)
{
  FileID fid = getFileID(Path);
  if (fid == FileIDInvalid) {
    return -1;
  }
  auto It = PIDs.find(fid);
  if (It == PIDs.end()) {
    std::cerr << "Unable to find a running process that is mapped to '" << Path << "'!" << std::endl;
    return -1;
  }
  return It->second.Pid;
}

std::string getProcessPath(MapFilesPID const& Procs, uint32_t Pid)
{
  for (auto const& P : Procs) {
    if (P.second.Pid == Pid) {
      return P.second.FullPath;
    }
  }
  return std::string{};
}

std::error_code walkProcessMemory(uint32_t PID, ValidCb const& CbValid, MemoryCb const& Cb, ReadFailureCB const* CbRF)
{
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
  if (hProc == NULL) {
    return getLastEC();
  }
  auto const Ret = walkProcessMemory(hProc, CbValid, Cb, CbRF);
  CloseHandle(hProc);
  return Ret;
}

std::error_code walkProcessPrivateRWMemory(uint32_t PID, MemoryCb const& Cb, ReadFailureCB const* CbRF)
{
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
  if (hProc == NULL) {
    return getLastEC();
  }
  auto const Ret = walkProcessPrivateRWMemory(hProc, Cb, CbRF);
  CloseHandle(hProc);
  return Ret;
}

std::error_code walkProcessMemory(HANDLE hProc, ValidCb const& CbValid, MemoryCb const& Cb, ReadFailureCB const* CbRF)
{
  MEMORY_BASIC_INFORMATION MemInfo;

  // Search for primes of subkeyLen bits, and check if they factor N!

  std::vector<uint8_t> Buf;
  SYSTEM_INFO SI;
  GetSystemInfo(&SI);
  uint8_t* CurAddr = (uint8_t*) SI.lpMinimumApplicationAddress;
  const size_t PageSize = SI.dwPageSize;
  while ((uintptr_t)CurAddr < (uintptr_t)SI.lpMaximumApplicationAddress) {
    if (!VirtualQueryEx(hProc, CurAddr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
      CurAddr += PageSize;
      continue;
    }
    CurAddr += MemInfo.RegionSize;

    if (!CbValid(MemInfo)) {
      continue;
    }


    const size_t Size = MemInfo.RegionSize;
    SIZE_T ReadSize = 0;
    Buf.resize(Size);
    if (!ReadProcessMemory(hProc, MemInfo.BaseAddress, &Buf[0], Size, &ReadSize)) {
      auto const Err = getLastEC();
      if (!CbRF) {
        return Err;
      }
      if ((*CbRF)((uint8_t const*)MemInfo.BaseAddress, Size, Err)) {
        continue;
      }
      else {
        return Err;
      }
    }
    if (ReadSize != Size) {
      std::cerr << "Warning: ReadProcessMemory returned only " << ReadSize << " bytes when asked for " << Size << std::endl;
    }

    if (!Cb(&Buf[0], ReadSize, (uint8_t const*) MemInfo.BaseAddress)) {
      break;
    }
  }

  return std::error_code{};
}

std::error_code walkProcessPrivateRWMemory(HANDLE hProc, MemoryCb const& Cb, ReadFailureCB const* CbRF)
{
  return walkProcessMemory(hProc,
    [] (MEMORY_BASIC_INFORMATION const& MemInfo) {
      return ((MemInfo.Type == MEM_PRIVATE) && (MemInfo.State != MEM_RESERVE) && (MemInfo.Protect == PAGE_READWRITE));
    },
    Cb,
    CbRF);
}


} // wcry
