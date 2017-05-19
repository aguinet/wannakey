// search_primes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void dump_hex(const char* Name, BYTE const* Data, size_t const Len)
{
	printf("%s:", Name);
	for (size_t i = 0; i < Len; ++i) {
		if ((i % 16 == 0)) {
			printf("\n");
		}
		printf("%02X ", Data[i]);

	}
	printf("\n====\n");
}
double normalizedEntropy(uint8_t const* Data, const size_t Len)
{
	// Initialized at 0 thanks to the uint32_t constructor.
	std::array<uint32_t, 256> Hist;

	std::fill(Hist.begin(), Hist.end(), 0);

	for (size_t i = 0; i < Len; ++i) {
		++Hist[Data[i]];
	}

	double Ret = 0.0;
	for (uint32_t Count : Hist) {
		if (Count) {
			double const P = (double)Count / (double)Len;
			Ret += P * std::log(P);
		}
	}
	if (Ret == 0.0) {
		// Or we would have -0.0 with the line below!
		return 0.0;
	}

	return -Ret / std::log(256.);
}

typedef boost::multiprecision::cpp_int BigIntTy;
typedef std::set<BigIntTy> SetPrimes;

static BigIntTy getInteger(uint8_t const* const Data, size_t const Len)
{
	BigIntTy n;
	boost::multiprecision::import_bits(n,
		Data, Data + Len, 8, false);
	return n;
}

static std::vector<uint8_t> getDataFromInteger(BigIntTy const& N)
{
	std::vector<uint8_t> Ret;
	boost::multiprecision::export_bits(N, std::back_inserter(Ret), 8, false);
	return Ret;
}

static bool isPrime(BigIntTy const& n)
{
	return boost::multiprecision::miller_rabin_test(n, 40);
}

static bool isPrime(uint8_t const* const Data, size_t const Len)
{
	const auto n = getInteger(Data, Len);
	return isPrime(n);
}


#define PALIGN 4
BigIntTy searchPrimes(uint8_t const* Data, size_t const Len, SetPrimes& Primes, BigIntTy const& N, const size_t PrimeSize)
{
	if (Len < PrimeSize) {
		return 0;
	}
	size_t const LenStop = ((Len - PrimeSize) / PALIGN) * PALIGN;
	uint8_t const* Block;
	for (size_t i = 0; i < LenStop; i += PALIGN) {
		Block = &Data[i];
		double E = normalizedEntropy(Block, PrimeSize);
		//printf("%0.4f\n", E);
		if (E >= 0.8) {
			// Checks whether we have a prime
			const auto P = getInteger(Block, PrimeSize);
			if (isPrime(P)) {
				if (Primes.insert(P).second) {
					printf("We found a new prime: %p!\n", Block);
					dump_hex("Prime", Block, PrimeSize);
					if (N % P == 0) {
						printf("PRIME FOUND: this prime divides N!\n");
						return P;
					}
				}
			}
		}
	}
	return 0;
}

static std::error_code getLastEC()
{
	return std::error_code{ (int)GetLastError(), std::system_category() };
}

static std::error_code getLastErrno()
{
	return std::error_code{ errno, std::system_category() };
}

static std::string getLastErrorMsg()
{
	return getLastEC().message();
}

std::vector<uint8_t> readFile(const char* path, std::error_code& EC)
{
	std::vector<uint8_t> Ret;
	FILE* f = fopen(path, "rb");
	if (!f) {
		EC = getLastErrno();
		return Ret;
	}
	fseek(f, 0, SEEK_END);
	const long Size = ftell(f);
	fseek(f, 0, SEEK_SET);
	Ret.resize(Size);
	if (fread(&Ret[0], 1, Size, f) != Size) {
		EC = getLastErrno();
		return Ret;
	}
	EC = std::error_code{};
	return Ret;
}

// Thanks to wine for this!

static void writeIntegerToFile(FILE* f, BigIntTy const& N, DWORD padSize)
{
	auto NData = getDataFromInteger(N);
	// Padding with zeros
	NData.resize(padSize);
	if (fwrite(&NData[0], 1, NData.size(), f) != NData.size()) {
		std::cerr << "Error while writing!" << std::endl;
	}
}

// Adapted from https://rosettacode.org/wiki/Modular_inverse#C.2B.2B
BigIntTy mulInv(BigIntTy a, BigIntTy b)
{
	BigIntTy b0 = b, t, q;
	BigIntTy x0 = 0, x1 = 1;
	if (b == 1) return 1;
	while (a > 1) {
		q = a / b;
		t = b, b = a % b, a = t;
		t = x0, x0 = x1 - q * x0, x1 = t;
	}
	if (x1 < 0) x1 += b0;
	return x1;
}

static bool genRSAKey(BigIntTy const& N, BigIntTy const& P, DWORD PrimeSize, const char* OutFile)
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
	pubKey.bitlen = (PrimeSize * 2)*8;
	pubKey.pubexp = e;
	fwrite(&pubKey, 1, sizeof(RSAPUBKEY), f);

	// Thanks to the wine source code for this format!
	BigIntTy const Q = N / P;
	BigIntTy const Phi = boost::multiprecision::lcm(P - 1, Q - 1);
	BigIntTy const d = mulInv(e, Phi);
	BigIntTy const dP = d % (P - 1);
	BigIntTy const dQ = d % (Q - 1);
	BigIntTy const iQ = mulInv(Q, P);
	writeIntegerToFile(f, N, PrimeSize *2);
	writeIntegerToFile(f, P, PrimeSize);
	writeIntegerToFile(f, Q, PrimeSize);
	writeIntegerToFile(f, dP, PrimeSize);
	writeIntegerToFile(f, dQ, PrimeSize);
	writeIntegerToFile(f, iQ, PrimeSize);
	writeIntegerToFile(f, d, PrimeSize *2);
	
	fclose(f);
	return true;
}
int main(int argc, char** argv)
{
	if (argc <= 2) {
		std::cerr << "Usage: " << argv[0] << " PID rsa_public_key" << std::endl;
		return 1;
	}

	std::cout << mulInv(10, 1007) << std::endl;

	DWORD pid = atoi(argv[1]);
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProc == NULL) {
		std::cerr << "Unable to open process " << pid << ": " << getLastErrorMsg() << std::endl;
		return 1;
	}

	const char* public_key = argv[2];
	std::error_code EC;
	std::vector<uint8_t> keyData = readFile(public_key, EC);
	if (EC) {
		std::cerr << "Error reading file: " << EC.message() << std::endl;
		return 1;
	}

	// Check that this is an RSA2 key of 2048 bits
	size_t idx = 0;
	dump_hex("blob_header", &keyData[idx], 8);
	idx += 8;
	dump_hex("pub_key", &keyData[idx], 12);
	if (*((DWORD*)&keyData[idx]) == 0x52534131) {
		printf("Invalid RSA key!\n");
		return 1;
	}
	idx += 12;
	DWORD keyLen = *(((DWORD*)&keyData[0]) + 3) / 8;
	DWORD subkeyLen = (keyLen + 1) / 2;
	printf("Keylen: %d\n", keyLen);

	// Get N
	dump_hex("N", &keyData[idx], keyLen);
	const auto N = getInteger(&keyData[idx], keyLen);

	MEMORY_BASIC_INFORMATION MemInfo;
	
	// Search for primes of subkeyLen bits, and check if they factor N!
	uint8_t* CurAddr = 0;
	const size_t PageSize = 4096;
	std::vector<uint8_t> Buf;
	SetPrimes Primes;
	int ret = 1;
	while ((uintptr_t)CurAddr < 0x80000000) {
		if (!VirtualQueryEx(hProc, CurAddr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
			CurAddr += PageSize;
			continue;
		}
		CurAddr += MemInfo.RegionSize;

		if (MemInfo.Type != MEM_PRIVATE || MemInfo.State == MEM_RESERVE || MemInfo.Protect != PAGE_READWRITE) {
			continue;
		}

		printf("Pages: %p - %p\n", MemInfo.BaseAddress, (uint8_t*)MemInfo.BaseAddress + MemInfo.RegionSize);

		// Gather memory from remote process
		const size_t Size = MemInfo.RegionSize;
		SIZE_T ReadSize;
		Buf.resize(Size);
		if (!ReadProcessMemory(hProc, MemInfo.BaseAddress, &Buf[0], Size, &ReadSize)) {
			std::cerr << "Error reading process memory: " << getLastErrorMsg() << std::endl;
			return 1;
		}
		if (ReadSize != Size) {
			std::cerr << "Warninng: ReadProcessMemory returned only " << ReadSize << " bytes when asked for " << Size << std::endl;
		}
		const auto P = searchPrimes(&Buf[0], ReadSize, Primes, N, subkeyLen);
		if (P != 0) {
			// Generate the private key
			std::cout << "Generating the private key in 'priv.key'..." << std::endl;
			genRSAKey(N, P, subkeyLen, "priv.key");		
			std::cout << "Done! You can now decrypt your files!" << std::endl;
			ret = 0;
			break;
		}
	}

	if (ret == 1) {
		std::cerr << "Error: no prime that divides N was found!\n" << std::endl;
	}
    return ret;
}

