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

#include <set>

#include <wkey/bigint.h>
#include <wkey/search_primes.h>
#include <wkey/tools.h>

#define PALIGN 4
wkey::BigIntTy wkey::searchPrimes(uint8_t const* Data, size_t const Len, SetPrimes& Primes, BigIntTy const& N, const size_t PrimeSize)
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
          dumpHex("Prime", Block, PrimeSize);
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
