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

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <wincrypt.h>
// Breaks boost::multiprecision
#undef min
#undef max

#include <iostream>
#include <cstdint>

#include <wkey/process.h>
#include <wkey/wcry.h>
#include <wkey/bigint.h>
#include <wkey/tools.h>
#include <wkey/search_primes.h>

using namespace wkey;

static bool genRSAKey(BigIntTy const& N, BigIntTy const& P, uint32_t PrimeSize, const char* OutFile)
{
  FILE* f = fopen(OutFile, "wb");
  if (!f) {
    std::cerr << "Unable to open '" << OutFile << "'" << std::endl;
    return false;
  }

  BLOBHEADER header;
  header.bType = 7;
  header.bVersion = 2;
  header.reserved = 0;
  header.aiKeyAlg = 0x0000a400;
  fwrite(&header, 1, sizeof(BLOBHEADER), f);

  auto const e = 0x10001;

  RSAPUBKEY pubKey;
  pubKey.magic = 0x32415352;
  pubKey.bitlen = (PrimeSize * 2) * 8;
  pubKey.pubexp = e;
  fwrite(&pubKey, 1, sizeof(RSAPUBKEY), f);

  // Thanks to the wine source code for this format!
  BigIntTy const Q = N / P;
  BigIntTy const Phi = boost::multiprecision::lcm(P - 1, Q - 1);
  BigIntTy const d = mulInv(e, Phi);
  BigIntTy const dP = d % (P - 1);
  BigIntTy const dQ = d % (Q - 1);
  BigIntTy const iQ = mulInv(Q, P);
  writeIntegerToFile(f, N, PrimeSize * 2);
  writeIntegerToFile(f, P, PrimeSize);
  writeIntegerToFile(f, Q, PrimeSize);
  writeIntegerToFile(f, dP, PrimeSize);
  writeIntegerToFile(f, dQ, PrimeSize);
  writeIntegerToFile(f, iQ, PrimeSize);
  writeIntegerToFile(f, d, PrimeSize * 2);

  fclose(f);
  return true;
}

int main(int argc, char** argv)
{
  std::cout << "Gather list of processes..." << std::endl;
  auto ProcList = getProcessList();

  uint32_t pid;
  if (argc < 2) {
    pid = getWcryPID(ProcList);
    if (pid == -1) {
      std::cerr << "Unable to find the PID of the wcry encryption process! You might need to pass it by hand as an argument to this software." << std::endl;
      std::cerr << "Usage: " << argv[0] << " PID" << std::endl;
      return 1;
    }
  }
  else
  if (argc >= 2) {
    pid = atoi(argv[1]);
  }

  std::string wcry_path = getProcessPath(ProcList, pid).c_str();
  if (wcry_path.size() == 0) {
    std::cerr << "unable to get working directory of the encrypting process!" << std::endl;
    return 1;
  }
  const char* sep = strrchr(wcry_path.c_str(), '\\');
  if (!sep) {
    std::cerr << "invalid path" << std::endl;
    return 1;
  }
  wcry_path.resize((size_t)std::distance(wcry_path.c_str(), sep));

  std::string const public_key = wcry_path +"\\00000000.pky";

  std::cout << "Using PID " << pid << " and working directory " << wcry_path << "..." << std::endl;

  std::cout << "Reading public key from " << public_key << "..." << std::endl;
  std::error_code EC;
  std::vector<uint8_t> keyData = readFile(public_key.c_str(), EC);
  if (EC) {
    std::cerr << "Error reading file: " << EC.message() << std::endl;
    return 1;
  }

  // Check that this is an RSA2 key of 2048 bits
  size_t idx = 0;
  dumpHex("blob_header", &keyData[idx], 8);
  idx += 8;
  dumpHex("pub_key", &keyData[idx], 12);
  if (*((uint32_t*)&keyData[idx]) == 0x52534131) {
    printf("Invalid RSA key!\n");
    return 1;
  }
  idx += 12;
  uint32_t keyLen = *(((uint32_t*)&keyData[0]) + 3) / 8;
  uint32_t subkeyLen = (keyLen + 1) / 2;
  printf("Keylen: %d\n", keyLen);

  // Get N
  dumpHex("N", &keyData[idx], keyLen);
  const auto N = getInteger(&keyData[idx], keyLen);

  // Search for primes of subkeyLen bits, and check if they factor N!
  int ret = 1;
  auto Err = walkProcessPrivateRWMemory(pid, [&] (uint8_t const* Buf, const size_t Size, uint8_t const*) {
    const auto P = searchPrimes(Buf, Size, N, subkeyLen);
    if (P == 0) {
      return true;
    }

    // Generate the private key
    std::string const pkeyPath = wcry_path + "\\" + "00000000.dky";
    std::cout << "Generating the private key in '" << pkeyPath << "'..." << std::endl;
    genRSAKey(N, P, subkeyLen, pkeyPath.c_str());
    std::cout << "Done! You can now decrypt your files with the \"official\" decryptor interface by clicking on the \"Decrypt\" button!" << std::endl;
    ret = 0;
    return false;
  });

  if (Err) {
    std::cerr << "Error while reading process " << pid << " memory: " << Err.message() << std::endl;
    return 1;
  }

  if (ret == 1) {
    std::cerr << "Error: no prime that divides N was found!\n" << std::endl;
  }
  return ret;
}

