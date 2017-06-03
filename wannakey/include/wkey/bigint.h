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

#ifndef WKEY_BIGINT_H
#define WKEY_BIGINT_H

#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/miller_rabin.hpp>


// Use boost::multiprecision for big int handling. It has the advantage to be
// header-only and saves us the pain of supporting third-party libraries within
// windows...

namespace wkey {

typedef boost::multiprecision::cpp_int BigIntTy;

void writeIntegerToFile(FILE* f, BigIntTy const& N, uint32_t padSize);
BigIntTy mulInv(BigIntTy a, BigIntTy b);
BigIntTy getInteger(uint8_t const* const Data, size_t const Len, bool MsvFirst  = false);
std::vector<uint8_t> getDataFromInteger(BigIntTy const& N, bool MsvFirst = false);

bool isPrime(BigIntTy const& n);

static bool isPrime(uint8_t const* const Data, size_t const Len)
{
  const auto n = getInteger(Data, Len);
  return isPrime(n);
}

} // wkey

#endif
