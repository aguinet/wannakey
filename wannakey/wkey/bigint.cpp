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

#include <random>
#include <wkey/bigint.h>

namespace wkey {

// Thanks to wine for this!
void writeIntegerToFile(FILE* f, BigIntTy const& N, uint32_t padSize)
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

BigIntTy getInteger(uint8_t const* const Data, size_t const Len, bool MsvFirst /* = false */)
{
  BigIntTy n;
  boost::multiprecision::import_bits(n,
      Data, Data + Len, 8, MsvFirst);
  return n;
}

std::vector<uint8_t> getDataFromInteger(BigIntTy const& N, bool MsvFirst /* = false */)
{
  std::vector<uint8_t> Ret;
  boost::multiprecision::export_bits(N, std::back_inserter(Ret), 8, MsvFirst);
  return Ret;
}

bool isPrime(BigIntTy const& n)
{
  static std::mt19937 RandEng(std::random_device{}());
  return boost::multiprecision::miller_rabin_test(n, 25, RandEng);
}

} // wkey
