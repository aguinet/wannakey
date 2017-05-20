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

#ifndef WKEY_SEARCH_PRIMES_H
#define WKEY_SEARCH_PRIMES_H

#include <cstdint>
#include <set>

#include <wkey/bigint.h>

namespace wkey {

typedef std::set<BigIntTy> SetPrimes;
BigIntTy searchPrimes(uint8_t const* Data, size_t const Len, SetPrimes& Primes, BigIntTy const& N, const size_t PrimeSize);

} // wkey

#endif
