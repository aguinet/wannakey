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
#include <atomic>

#ifdef HAS_OMP_SUPPORT
#include <omp.h>
#endif

#include <wkey/bigint.h>
#include <wkey/search_primes.h>
#include <wkey/tools.h>

#define PALIGN 4
#define MINIMAL_ENTROPY 0.7

using namespace wkey;

#ifdef HAS_OMP_SUPPORT
static BigIntTy searchPrimes_omp(uint8_t const* Data, size_t const Len, BigIntTy const& N, const size_t PrimeSize)
{
  if (Len < PrimeSize) {
    return 0;
  }

  size_t const LenStop = ((Len - PrimeSize) / PALIGN) * PALIGN;
  // Visual studio has only support for OpenMP 2.0. We thus can't use #pragma
  // omp cancel for...
  bool Found = false;
  BigIntTy Ret = 0;
#pragma omp parallel for shared(Found) shared(Ret)
  for (int64_t i = 0; i <= LenStop; i += PALIGN) {
    if (!Found) {
      uint8_t const* Block = &Data[i];
      const double E = normalizedEntropy(Block, PrimeSize);
      if (E >= MINIMAL_ENTROPY) {
        auto P = getInteger(Block, PrimeSize, false);
        if (N % P == 0) {
#pragma omp critical
          {
            Ret = P;
            Found = true;
          }
        }
        P = getInteger(Block, PrimeSize, true);
        if (N % P == 0) {
#pragma omp critical
          {
            Ret = P;
            Found = true;
          }
        }
      }
    }
  }
  

  return Ret;
}
#endif

static BigIntTy searchPrimes_serial(uint8_t const* Data, size_t const Len, BigIntTy const& N, const size_t PrimeSize)
{
  size_t const LenStop = ((Len - PrimeSize) / PALIGN) * PALIGN;
  for (size_t i = 0; i <= LenStop; i += PALIGN) {
    uint8_t const* Block = &Data[i];
    const double E = normalizedEntropy(Block, PrimeSize);
    if (E >= MINIMAL_ENTROPY) {
      auto P = getInteger(Block, PrimeSize, false);
      if (N % P == 0) {
        return P;
      }
      P = getInteger(Block, PrimeSize, true);
      if (N % P == 0) {
        return P;
      }
    }
  }

  return 0;
}

BigIntTy wkey::searchPrimes(uint8_t const* Data, size_t const Len, BigIntTy const& N, const size_t PrimeSize)
{
  if (Len < PrimeSize) {
    return 0;
  }

#ifdef HAS_OMP_SUPPORT
  if (omp_get_max_threads() > 1) {
    return searchPrimes_omp(Data, Len, N, PrimeSize);
  }
#endif
  return searchPrimes_serial(Data, Len, N, PrimeSize);
}
