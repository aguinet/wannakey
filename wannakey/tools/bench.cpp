#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <random>
#include <chrono>

#include <wkey/bigint.h>
#include <wkey/tools.h>
#include <wkey/search_primes.h>

using namespace wkey;

static constexpr size_t PrimeBytes = 128;

template <size_t SizeBytes>
static BigIntTy randomPrime()
{
  typedef boost::random::independent_bits_engine<std::mt19937, SizeBytes*8, BigIntTy> generator_type;
  static generator_type gen(std::random_device{}());
  BigIntTy Ret = gen();
  Ret |= 1;
  // Check every odd number until one is good!
  while (!isPrime(Ret)) {
    Ret += 2;
  }
  return Ret;
}

template <class Func>
static void bench(const char* Name, const size_t Size, double RatioRand, Func F)
{
  static std::mt19937 RandGen(1000);

  // Generate three distinct random primes: two for N, and a third for false
  // positives
  BigIntTy const P = randomPrime<PrimeBytes>();
  BigIntTy Q,O;
  do {
    Q = randomPrime<PrimeBytes>();
  }
  while (Q == P);
  do {
    O = randomPrime<PrimeBytes>();
  }
  while ((O == P) || (O == Q));
  BigIntTy const N = P*Q;

  // Write O in memory according to PercentPrime
  auto Odata = getDataFromInteger(O);

  std::unique_ptr<uint8_t[]> Buf_(new uint8_t[Size]);
  uint8_t* Buf = Buf_.get();
  memset(Buf, 0, Size);
  const size_t SizePrimes = (Size/PrimeBytes)*PrimeBytes;
  std::uniform_real_distribution<double> RandRatio;
  // std::uniform_int_distribution<uint8_t> isn't defined by the standard for
  // some reason... 
  std::uniform_int_distribution<uint16_t> RandBytes;
  for (size_t i = 0; i < SizePrimes; i += PrimeBytes) {
    if (RandRatio(RandGen) < RatioRand) {
      std::generate(&Buf[i], &Buf[i+PrimeBytes], [&]() { return RandBytes(RandGen); });
    }
  }

  // Write P in the end
  auto Pdata = getDataFromInteger(P);
  memcpy(&Buf[Size-Pdata.size()], &Pdata[0], Pdata.size());

  // Bench F using this buffer
  std::array<double, 10> Times;
  for (size_t i = 0; i < Times.size(); ++i) {
    const auto Start = std::chrono::system_clock::now();
    F(Buf, Size, N);
    const auto End = std::chrono::system_clock::now();
    const std::chrono::duration<double> Diff = End-Start;
    Times[i] = Diff.count();
  }
  std::sort(std::begin(Times), std::end(Times));
  const double Res = Times[0];
  std::cout << Name << ", ratio " << RatioRand << ": " << Res*1000.0 << "ms" << std::endl;
}

void do_bench(size_t const Size, double const RatioRand)
{
  bench("search_primes", Size, RatioRand, [=](uint8_t const* Data, size_t const Len, BigIntTy const& N) {
      searchPrimes(Data, Len, N, PrimeBytes);
  });
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " size" << std::endl;
    return 1;
  }

  const size_t Size = atoll(argv[1]);
  do_bench(Size, .02);
  do_bench(Size, .05);
  do_bench(Size, .2);
  do_bench(Size, .4);
  do_bench(Size, .6);
  do_bench(Size, .8);

  return 0;
}
