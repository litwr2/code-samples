/*
	Copyright (C) 2012-2013 Yaroslav Zotov.
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BIGNUMBER_H
#define BIGNUMBER_H

#include<iostream>
#include<string>
#include<cstring>
#include<cstdlib>
#include<vector>
#include<sstream>
#include <climits>
using namespace std;

typedef unsigned int digit;
typedef unsigned long long uLong;

const digit Base2 = 1000000000U, BASE = 2147483648U, BaseMod = BASE - 1, BaseLog = 31, KaratsubaMin = 80, Base2Log = 9, Base3Log = Base2Log + 1;
const uLong Base3 = (uLong)Base2*10;
const int sizeOfDigit = sizeof(digit), sizeOfuLong = sizeof(uLong);

class bigNumber {
   vector<digit> num;
   long size;
   bool sign;
   void fromDecimal(uLong*&, long&, digit&);
   void deleteLeadingZeroes();
public:
   long getSize() const {return size;};
   bigNumber();
   bigNumber(string);
   bigNumber(const char*);
   bigNumber(long);
   bigNumber(long long);
   bigNumber(int);
   bigNumber(digit);
   bigNumber(uLong);
   bigNumber(unsigned long);
   bigNumber& operator=(const bigNumber&);
   friend bigNumber abs(const bigNumber&);
   bigNumber operator-() const;
   friend bigNumber operator+(const bigNumber&, const bigNumber&);
   bigNumber operator+=(const bigNumber&);
   bigNumber operator++();
   bigNumber operator++(int);
   friend bigNumber operator-(const bigNumber&, const bigNumber&);
   bigNumber operator-=(const bigNumber&);
   bigNumber operator--();
   bigNumber operator--(int);
   friend bigNumber operator*(const bigNumber&, const bigNumber&);
   bigNumber operator*=(const bigNumber&);
   friend bigNumber operator/(const bigNumber&, const bigNumber&);
   bigNumber operator/=(const bigNumber&);
   friend bigNumber operator%(const bigNumber&, const bigNumber&);
   bigNumber operator%=(const bigNumber&); 
   friend bool operator==(const bigNumber&, const bigNumber&);
   friend bool operator!=(const bigNumber&, const bigNumber&);
   friend bool operator> (const bigNumber&, const bigNumber&);
   friend bool operator< (const bigNumber&, const bigNumber&);
   friend bool operator>=(const bigNumber&, const bigNumber&);
   friend bool operator<=(const bigNumber&, const bigNumber&); 
   friend bool absCompare(const bigNumber&, const bigNumber&);
   friend istream& operator>>(istream&, bigNumber&);
   friend ostream& operator<<(ostream&, const bigNumber&);
   friend bigNumber operator>>(const bigNumber&, const bigNumber&);
   friend bigNumber operator<<(const bigNumber&, const bigNumber&);     
   friend bigNumber pow(bigNumber, bigNumber);
   friend void bigDivide(const bigNumber&, const bigNumber&, bigNumber&, bigNumber&);
   friend void tinyDivide(const bigNumber&, bigNumber&, digit, digit&);
   friend void tinyMul(const bigNumber&, digit, bigNumber&);
   friend void KaratsubaMul(const bigNumber&, const bigNumber&, bigNumber&);
   friend bigNumber tinyMul1(const bigNumber&, digit);
   friend void bigAdd(const bigNumber&, const bigNumber&, bigNumber&);
   friend void bigSub(const bigNumber&, const bigNumber&, bigNumber&);
   friend bigNumber gcd(const bigNumber&, const bigNumber&);
   friend bigNumber lcm(const bigNumber&, const bigNumber&);
   friend bigNumber fib(const bigNumber&);
   unsigned int toUint() const;
   string toString(int, int);
};

bigNumber log10(bigNumber);

#endif

