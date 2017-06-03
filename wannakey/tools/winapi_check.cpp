#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <array>

#include <wkey/process.h>
#include <wkey/tools.h>

static bool doGenKey = false;
static bool doCryptDestroy = true;
static bool doCryptReleaseCtx = true;
static bool verbose = false;

static constexpr size_t KeyBits = 2048;
static constexpr size_t SubKeyBits = (KeyBits +1)/2;

static constexpr size_t KeyBytes = KeyBits/8;
static constexpr size_t SubKeyBytes = SubKeyBits/8;

#define USER_RSA_KEY_LEN 2048

using namespace wkey;

// Based on https://msdn.microsoft.com/fr-fr/library/windows/desktop/ms724832(v=vs.85).aspx
static const char* GetWinVersionStr(uint32_t Major, uint32_t Minor)
{
  switch (Major) {
    case 10:
      return "Win10 or Server 2016";
    case 6:
      if (Minor == 3) {
        return "Win 8.1 or Server 2012 R3";
      }
      if (Minor == 2) {
        return "Win8 or Server 2012";
      }
      if (Minor == 1) {
        return "Win7 or Server 2008R2";
      }
      if (Minor == 0) {
        return "Vista or Server 2008";
      }
      break;
    case 5:
      if (Minor == 2) {
        return "XP 64-bit or Server 2003 or Server 2003R2";
      }
      if (Minor == 1) {
        return "XP 32-bit";
      }
      if (Minor == 0) {
        return "2000";
      }
      break;
    default:
      break;
  };
  return "Unkown";
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL IsWow64()
{
  BOOL bIsWow64 = FALSE;

  //IsWow64Process is not available on all supported versions of Windows.
  //Use GetModuleHandle to get a handle to the DLL that contains the function
  //and GetProcAddress to get a pointer to the function if available.

  fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
      GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

  if (NULL != fnIsWow64Process)
  {
    if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
    {
      return FALSE;
    }
  }
  return bIsWow64;
}

static const char* getArchi()
{
  SYSTEM_INFO SI;
  GetSystemInfo(&SI);
  switch (SI.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return "Intel 64-bit";
    case PROCESSOR_ARCHITECTURE_ARM:
      return "ARM";
    case PROCESSOR_ARCHITECTURE_IA64:
      return "IA64";
    case PROCESSOR_ARCHITECTURE_INTEL:
      if (IsWow64()) {
        return "Intel 64-bit";
      }
      return "Intel 32-bit";
    default:
      break;
  };
  return "Unknown arch";
}

static int searchPrimes(HANDLE hProc, uint8_t const* const P, uint8_t const* const Q)
{
  int Ret = 1;
  ReadFailureCB OnErr = [&](uint8_t const* Buf, const size_t Size, std::error_code const& EC) {
    if (verbose) {
      std::cerr << "Warning: unable to read memory at " << std::hex << (uintptr_t)Buf << " for " << std::dec << Size << " bytes: " << EC.message();
    }
    return true;
  };

  std::array<uint8_t, SubKeyBytes> Pinv;
  std::array<uint8_t, SubKeyBytes> Qinv;
  memcpy(&Pinv[0], P, SubKeyBytes);
  memcpy(&Qinv[0], Q, SubKeyBytes);
  std::reverse(std::begin(Pinv), std::end(Pinv));
  std::reverse(std::begin(Qinv), std::end(Qinv));

  auto Err = walkProcessPrivateRWMemory(hProc,
    [&](uint8_t const* Buf, const size_t Size, uint8_t const* OrgMem) {
      uint8_t const* SRet = memmem(Buf, Size, &P[0], SubKeyBytes);
      size_t Idx = std::distance(Buf, SRet);
      uint8_t const* OrgPtr = OrgMem + Idx;
      if (SRet != nullptr && OrgPtr != P) {
        printf("Found P at %p\n", OrgPtr);
        Ret = 0;
      }
      SRet = memmem(Buf, Size, &Q[0], SubKeyBytes);
      Idx = std::distance(Buf, SRet);
      OrgPtr = OrgMem + Idx;
      if (SRet != nullptr && OrgPtr != Q) {
        printf("Found Q at %p\n", OrgPtr);
        Ret = 0;
      }
      SRet = memmem(Buf, Size, &Pinv[0], SubKeyBytes);
      Idx = std::distance(Buf, SRet);
      OrgPtr = OrgMem + Idx;
      if (SRet != nullptr && OrgPtr != &Pinv[0]) {
        printf("Found Pinv at %p\n", OrgPtr);
        Ret = 0;
      }
      SRet = memmem(Buf, Size, &Qinv[0], SubKeyBytes);
      Idx = std::distance(Buf, SRet);
      OrgPtr = OrgMem + Idx;
      if (SRet != nullptr && OrgPtr != &Qinv[0]) {
        printf("Found Qinv at %p\n", OrgPtr);
        Ret = 0;
      }
      return true;
    },
    &OnErr
  );

  if (Err) {
    std::cerr << "Error while walking process memory: " << Err.message() << std::endl;
    return 2;
  }
  return Ret;
}


static int generateKeyAndCheck()
{
  HCRYPTPROV prov;
  HCRYPTKEY keyUser;

  if (!CryptAcquireContext(&prov,
        NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT)) {
    if (!CryptAcquireContext(&prov, 0, 0, 24, 0xF0000000)) {
      std::cerr << "error CryptAcquireContext: " << getLastErrorMsg() << std::endl;
      return 2;
    }
  }

  if (!CryptGenKey(prov, AT_KEYEXCHANGE,
        (USER_RSA_KEY_LEN << 16) | CRYPT_EXPORTABLE, &keyUser)) {
    std::cerr << "error CryptGenKey: " << getLastErrorMsg() << std::endl;
    return 2;
  }


  BYTE keyData[4096];
  if (verbose) {
    printf("prov: %08X\n", prov);
    printf("keyUser: %08X\n", keyUser);
    printf("keyData: %08X\n", keyData);
  }
  memset(keyData, 0xFE, sizeof(keyData));

  DWORD len = 4096;
  if (!CryptExportKey(keyUser, 0, PRIVATEKEYBLOB, 0, &keyData[0], &len)) {
    std::cerr << "error CryptExportKey: " << getLastErrorMsg() << std::endl;
    return 2;
  }

  // We try and involve the less functions that use heap memory here! Do that
  // before calling the "destroy" functions.
  uint8_t P[SubKeyBytes];
  uint8_t Q[SubKeyBytes];
  size_t idx = 8 + 12;
  if (verbose) {
    dumpHex("N", &keyData[idx], KeyBytes);
  }
  idx += KeyBytes;
  memcpy(&P[0], &keyData[idx], sizeof(P));
  if (verbose) {
	  dumpHex("P", &keyData[idx], SubKeyBytes);
	  std::cout << "Entropy P: " << normalizedEntropy(&P[0], sizeof(P)) << std::endl;
  }
  idx += SubKeyBytes;
  memcpy(&Q[0], &keyData[idx], sizeof(Q));
  if (verbose) {
	  dumpHex("Q", &keyData[idx], SubKeyBytes);
	  std::cout << "Entropy Q: " << normalizedEntropy(&P[0], sizeof(Q)) << std::endl;
  }
  if (verbose) {
    printf("P: %08X\n", P);
    printf("Q: %08X\n", Q);
  }

  printf("Key generated, zeroying data...\n");
  SecureZeroMemory(keyData, 4096);

  if (doCryptDestroy) {
    printf("Will destroy key...\n");
  }
  if (doCryptReleaseCtx) {
    printf("Will release ctx...\n");
  }
  // From now one we should call the less functions that would ask memory from heap!
  if (doCryptDestroy) {
    CryptDestroyKey(keyUser);
  }

  if (doCryptReleaseCtx) {
    CryptReleaseContext(prov, 0);
  }

  return searchPrimes(GetCurrentProcess(), P, Q);
}

static void usage(const char* Path)
{
  std::cerr << "Usage: " << Path << " [options]" << std::endl;
  std::cerr << "where options are:" << std::endl;
  std::cerr << "  -h, --help: show this help" << std::endl;
  std::cerr << "  --without-destroy-key: do not call CryptDecryptKey. Implies --without-release-ctx." << std::endl;
  std::cerr << "  --without-release-ctx: do not call CryptReleaseContext." << std::endl;
  std::cerr << "  --verbose:             print various debug informations." << std::endl;

}


int main(int argc, char** argv)
{
  for (int i = 1; i < argc; ++i) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      usage(argv[0]);
      return 1;
    }
    else
    if (strcmp(argv[i], "--without-destroy-key") == 0) {
      doCryptDestroy = false;
      doCryptReleaseCtx = false;
    }
    else
    if (strcmp(argv[i], "--without-release-ctx") == 0) {
      doCryptReleaseCtx = false;
    }
    else
    if (strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    }
    else {
      std::cerr << "Error: unknown argument '" << argv[i] << "'" << std::endl << std::endl;
      usage(argv[0]);
      return 1;
    }
  }

  int ret = generateKeyAndCheck();
  if (ret == 2) {
    std::cerr << "An error occured while searching for primes in memory!" << std::endl;
    return 2;
  }
  std::cout << std::endl << "Results:" << std::endl;
  if (ret == 0) {
    std::cout << "Wannakey technique has chances to work on this OS: Windows Cryptograhic API leaks secrets!" << std::endl;
    std::cout << "You can try wannakey now by launching the wannakey.exe binary." << std::endl;
  }
  else {
    std::cout << "The Windows Cryptograhic API does not seem to leak secrets on this Windows version :/" << std::endl;
    std::cout << "Wannakey has close to zero chance to recover the private key. Sorry :/" << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Please report the formatted result below to https://github.com/aguinet/wannakey/issues/5: " << std::endl;

  // Get the Windows version in english. As simple as MS like it to be...
  DWORD Zero;
  DWORD FVISize = GetFileVersionInfoSize("kernel32.dll", &Zero);
  if (FVISize == 0) {
    std::cerr << "Error: unable to get Windows version: " << getLastErrorMsg() << std::endl;
    return 2;
  }
  std::unique_ptr<uint8_t[]> FVIBuf(new uint8_t[FVISize]);
  if (!GetFileVersionInfo("kernel32.dll", NULL, FVISize, FVIBuf.get())) {
    std::cerr << "Error: unable to get Windows version: " << getLastErrorMsg() << std::endl;
    return 2;
  }
  VS_FIXEDFILEINFO* VI;
  UINT VISize;
  VerQueryValue(FVIBuf.get(), "\\", (void**) &VI, &VISize);
  if (VISize < sizeof(VS_FIXEDFILEINFO)) {
    std::cerr << "Error: unable to get Windows version!" << std::endl;
    return 2;
  }
  uint32_t Major = HIWORD(VI->dwProductVersionMS);
  uint32_t Minor = LOWORD(VI->dwProductVersionMS);
  std::cout << "Windows " << Major << "." << Minor << "." << HIWORD(VI->dwProductVersionLS) << " (" << GetWinVersionStr(Major,Minor) << ") / " << getArchi() << " / " << ((ret == 0) ? "Leaks" : "No leak") << std::endl;

  return ret;
}
