
#include "bignumber.h"

ostream& operator<<(ostream& s, const bigNumber& number) {
   if (number.size == 0) {
      s << "0";
      return s;
   }
   long newLength = number.size + number.size/6 + 1;//newLength = 1.2*number.size + 1;
   digit *newLong = new digit [newLength];
   if (number.sign)
      s << "-";
   bigNumber A(number);
   for(long i = 0; i < newLength; i++)
      tinyDivide(A, A, Base2, newLong[i]);
   while (newLong[newLength - 1] == 0)
       newLength--;
   s << newLong[newLength - 1];
   for(long i = newLength - 2; i >= 0; i--) {
      digit temp = newLong[i], pow = Base2 / 10;
      for (int j = 0; j < Base2Log; j++) {
         s << temp / pow;
         temp %= pow;
         pow /= 10;
      }
   }
   delete [] newLong;
   return s;
}

istream& operator>>(istream& s, bigNumber& number) {
   string tmp;
   s >> tmp;
   number = tmp;
   return s;
}

bigNumber::bigNumber() {
   sign = 0;
   size = 0;
   num.push_back(0);
}

bigNumber::bigNumber(string s1) {
   while(s1[0] == ' ' || s1[0] == '\t')
      s1.erase(0,1);
   sign = (s1[0] == '-');
   if (sign)
      s1.erase(0,1);
   for (int i = 0; i < s1.size(); i ++)
      if (s1[i] < '0' || s1[i] > '9')
         throw 1;  
   while(s1[0] == '0')
      s1.erase(0,1);
   if(s1.size() == 0) {
      sign = 0;
      num.push_back(0);
      size = 0;
      return;
   } 
   long size2 = (size = s1.size() / Base3Log + (s1.size()%Base3Log == 0 ? 0 : 1)) + size/2 + 1;
   uLong* num1 = new uLong[size];
   for (long i = 0; i < size; i++)
      num1[i] = 0;
   long index = size - 1;
   uLong pow = 1;
   for (long i = s1.size() - 1; i >= 0; i--) {
      num1[index] += (s1[i] - '0') * pow;
      pow *= 10;
      if (pow == Base3) {
         index--;
         pow = 1;
      }
   }
   num.resize(size2, 0);
   for(long i = 0; i < size2; i++)
      fromDecimal(num1, size, num[i]);
   delete [] num1;
   size = size2;
   this->deleteLeadingZeroes();
}

bigNumber::bigNumber(const char* s1) {
   while(s1[0] == ' ' || s1[0] == '\t')
      s1++;
   sign = (s1[0] == '-');
   if (sign)
      s1++;
   for (int i = 0; i < strlen(s1); i ++)
      if (s1[i] < '0' || s1[i] > '9')
         throw 1;
   while(s1[0] == '0')
      s1++;
   if(strlen(s1) == 0) {
      sign = 0;
      num.push_back(0);
      size = 0;
      return;
   }
   long size2 = (size = strlen(s1) / Base3Log + (strlen(s1)%Base3Log == 0 ? 0 : 1)) + size/2 + 1;
   uLong* num1 = new uLong[size];
   for (long i = 0; i < size; i++)
      num1[i] = 0;
   long index = size - 1;
   uLong pow = 1;
   for (long i = strlen(s1) - 1; i >= 0; i--) {
      num1[index] += (s1[i] - '0') * pow;
      pow *= 10;
      if (pow == Base3) {
         index--;
         pow = 1;
      }
   }
   num.resize(size2, 0);
   for(long i = 0; i < size2; i++)
      fromDecimal(num1, size, num[i]);
   delete [] num1;
   size = size2;
   this->deleteLeadingZeroes();
}

bigNumber::bigNumber(long l) {
   size = 0;
   sign = l < 0;
   if (l == 0) {
      num.push_back(0);
      return;
   }
   if (l == LONG_MIN) {
      uLong temp = -l;
      while (temp != 0) {
         temp >>= BaseLog;
         size++;
      }
      for (int i = size - 1; i >= 0; i--) {
         num.push_back(l & BaseMod);
         l >>= BaseLog;
      }
      return;
   }   
   l = abs(l);
   long temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber::bigNumber(long long l) {
   size = 0;
   sign = l < 0;
   if (l == 0) {
      num.push_back(0);
      return;
   }
   if (l == LLONG_MIN) {
      uLong temp = -l;
      while (temp != 0) {
         temp >>= BaseLog;
         size++;
      }
      for (int i = size - 1; i >= 0; i--) {
         num.push_back(l & BaseMod);
         l >>= BaseLog;
      }
      return;
   }
   l = abs(l);
   long long temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber::bigNumber(int l) {
   size = 0;
   sign = l < 0;
   if (l == 0) {
      num.push_back(0);
      return;
   }
   if (l == INT_MIN) {
      uLong temp = -l;
      while (temp != 0) {
         temp >>= BaseLog;
         size++;
      }
      for (int i = size - 1; i >= 0; i--) {
         num.push_back(l & BaseMod);
         l >>= BaseLog;
      }
      return;
   }
   l = abs(l);
   int temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber::bigNumber(digit l) {
   size = 0;
   sign = l < 0;
   if (l == 0) {
      num.push_back(0);
      return;
   }
   digit temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber::bigNumber(uLong l) {
   size = 0;
   sign = 0; 
   if (l == 0) {
      num.push_back(0);
      return;
   }
   uLong temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber::bigNumber(unsigned long l) {
   size = 0;
   sign = 0; 
   if (l == 0) {
      num.push_back(0);
      return;
   }
   uLong temp = l;
   while (temp != 0) {
       temp >>= BaseLog;
       size++;
   }
   for (int i = size - 1; i >= 0; i--) {
       num.push_back(l & BaseMod);
       l >>= BaseLog;
   }
}

bigNumber& bigNumber::operator=(const bigNumber &n1){
   if (this == &n1)
      return *this;
   sign = n1.sign;
   size = n1.size;
   this->num = n1.num;
   return *this;
}

bigNumber abs(const bigNumber &n1) {
   bigNumber temp(n1);
   temp.sign = false;
   return temp;
}

bigNumber bigNumber::operator-() const {
   bigNumber temp(*this);
   temp.sign = !temp.sign;
   return temp;
}

bool operator==(const bigNumber& n2, const bigNumber& n1) {
   if (n1.sign == n2.sign) { 
      if (n1.size == n2.size) {
         const digit *n20 = &n2.num[0], *n10 = &n1.num[0];
         for (long i = 0; i < n2.size; i++)
            if (n10[i] != n20[i])
               return false;
      } else {
         return false;
      }
      return true;
   }
   return false;
}

bool operator!=(const bigNumber& n1, const  bigNumber& n2) {
   return !(n2 == n1);
}

bool operator> (const bigNumber& n1, const bigNumber& n2) {
   if (n2.sign && !n1.sign)
      return true;
   else if (n1.sign && !n2.sign)
      return false;
   if (n1.size == n2.size) {
      if (n1.sign)
         return abs(n2) > abs(n1);
      const digit *n10 = &n1.num[0], *n20 = &n2.num[0];
      for (long i = n1.size - 1; i >= 0; i--)
         if (n10[i] > n20[i])
            return true;
         else if (n10[i] < n20[i])
            return false;
   }    
   if (n1.sign) 
      return n1.size < n2.size;
   else
      return n1.size > n2.size;
}

bool operator< (const bigNumber& n1, const bigNumber& n2) {
   return n2 > n1;
}

bool operator>= (const bigNumber& n1, const bigNumber& n2) {
   return n2 == n1 || n1 > n2;
}

bool operator<= (const bigNumber& n1, const bigNumber& n2) {
   return n1 == n2 || n1 < n2;
}

bool absCompare(const bigNumber &n1, const bigNumber &n2) { //return 1 if abs(n1) > abs(n2)
    if (n1.size == n2.size) {
       const digit *n10 = &n1.num[0], *n20 = &n2.num[0];
       for (long i = n1.size - 1; i >= 0; i--)
          if (n10[i] > n20[i])
             return true;
          else if (n10[i] < n20[i])
             return false;
    }
    return n1.size > n2.size;
}

bigNumber operator+(const bigNumber &n0, const bigNumber &n1) {
   bigNumber returnNum(0);
   bigAdd(n0, n1, returnNum);
   return returnNum;
}

void bigAdd(const bigNumber &n0, const bigNumber &n1, bigNumber &returnNum) {////returnNum.size must be >= max(n0.size, n1.size)
   if (absCompare(n1, n0)) {
      bigAdd(n1, n0, returnNum);
      return;
   }
   long l0 = n0.size, l1 = n1.size, i = -1, j = -1;//i - l1, j - l0
   const digit *s0 = &n0.num[0], *s1 = &n1.num[0];
   if(returnNum.size < l0) {
      returnNum = n0;
   }
   digit *ret = &returnNum.num[0];
   long long carry = 0;
   if (n1.sign == n0.sign) {
      while (++i < l1) {
          carry += s1[i] + s0[++j];
          ret[j] = carry & BaseMod;
          carry >>= BaseLog;
      }
      while (carry && j < l0 - 1) {
          carry += s0[++j];
          ret[j] = carry & BaseMod;
          carry >>= BaseLog;
      }
      if (carry) {
         returnNum.size++;
         returnNum.num.push_back(carry);
      }
   } else {
      while (++i < l1) {
          carry += (long long)s0[++j] - s1[i];
          ret[j] = carry & BaseMod;
          carry >>= BaseLog;
      }
      while (carry && j < l0 - 1) {
          carry += s0[++j];
          ret[j] = carry & BaseMod;
          carry >>= BaseLog;
      }
      returnNum.deleteLeadingZeroes();
   }
}

void bigSub(const bigNumber &n0, const bigNumber &n1, bigNumber &returnNum) {
   bigAdd(n0, -n1, returnNum);
}

bigNumber bigNumber::operator+= (const bigNumber &n1) {
   bigNumber temp(*this + n1);
   return *this = temp;
}

bigNumber bigNumber::operator++() {
   bigNumber temp(*this + 1);
   return *this = temp;
}

bigNumber bigNumber::operator++(int) {
   bigNumber temp(*this);
   *this += 1;
   return temp;
}

bigNumber bigNumber::operator--() {
   bigNumber temp(*this - 1);
   return *this = temp;
}

bigNumber bigNumber::operator--(int) {
   bigNumber temp(*this);
   *this -= 1;
   return temp;
}

bigNumber operator-(const bigNumber &n1, const bigNumber &n2) {
   bigNumber ret(0);
   bigSub(n1, n2, ret);
   return ret;
}

bigNumber bigNumber::operator-= (const bigNumber &n1) {
   bigNumber temp(*this - n1);
   return *this = temp;
}

bigNumber operator* (const bigNumber &n0, const bigNumber &n1) {////////////////////////////////////
    if (n0.size == 0 || n1.size == 0)
       return 0;
    long n0Size = n0.size, n1Size = n1.size;
    if (n0Size == 1 || n1Size == 1) {
       if (n0Size == 1) {
           digit m = n0.num[0];
           if (n0.sign)
              return -tinyMul1(n1, m);
           return tinyMul1(n1, m);
       }
       digit m = n1.num[0];
       if (n1.sign)
          return -tinyMul1(n0, m);
       return tinyMul1(n0, m);
    }
    bigNumber res(0);
    if(n0Size >= KaratsubaMin || n1Size >= KaratsubaMin) {
       if(n0Size >= n1Size && n1Size >= KaratsubaMin/2) {
           KaratsubaMul(n0, n1, res);
           return res;
       }
       if(n1Size >= n0Size && n0Size >= KaratsubaMin/2) {
          KaratsubaMul(n1, n0, res);
          return res;
       }
    }
    uLong length = n1Size + n0Size, temp, carry;
    long i, j;
    res.num.resize(length);
    res.size = length;
    digit *rslt = &res.num[0];
    const digit *s0 = &n0.num[0], *s1 = &n1.num[0];
    for (i = 0; i < length; i++)
       rslt[i] = 0;
    for (i = 0; i < n0Size; i++) {
       carry = 0;
       for (j = 0; j < n1Size; j++) {
          temp = (uLong)s0[i] * s1[j] + rslt[i+j] + carry;
          carry = temp >> BaseLog;
          rslt[i+j] = temp & BaseMod;//temp%BASE;
       }
       rslt[i+j] = carry;
    }
    res.deleteLeadingZeroes();
    res.sign = n0.sign ^ n1.sign;
    return res;
}

bigNumber bigNumber::operator*= (const bigNumber &n1) {
   bigNumber temp(*this*n1);
   return *this = temp;
}

void bigDivide(const bigNumber &n0, const bigNumber &n1, bigNumber &Q, bigNumber &R) {//n0 < n1
   if (absCompare(n1, n0)) {//if (abs(n0) < abs(n1))
      Q = 0;
      R = n0;
      return;
   }
   bigNumber U(abs(n0)), B1(abs(n1));
   long n = B1.size, l0 = U.size;
   digit scale = BASE / (n1.num[n1.size - 1] + 1), r;
   long long qGuess, borrow, carry;
   if (scale > 1) {
      tinyMul(U, scale, U);//U *= scale;
      tinyMul(B1, scale, B1);//B1 *= scale;
   }
   if(l0 == U.size) {
      U.size++;
      U.num.push_back(0);
   }
   long m = l0 - n + 1;   
   Q.num.resize(m);  
   Q.size = m;
   digit *ret = &Q.num[0];
   for (long i = 0; i < m; i++)
      ret[i] = 0;
   digit *b = &B1.num[B1.size - 1], *u = &U.num[U.size - 1], *q = &Q.num[Q.size - 1];
   long long temp1, temp2;
   long i;
   for(long j = 0; j > -m; j--) {
      qGuess = (((uLong)u[j] << BaseLog) + u[j - 1]) / b[0];
      r = (((uLong)u[j] << BaseLog) + u[j - 1]) % b[0];
      while ( r < BASE) {
         temp2 = b[-1]*qGuess;
         temp1 = ((uLong)r << BaseLog) + u[j - 2];
         if ( temp2 > temp1 || qGuess == BASE ) {
            qGuess--;
            r += b[0];
         } else break;
      }
      digit *shift = u + j;
      carry = borrow = 0;
      for (i = n - 1; i >= 0; i--) {
         temp1 = b[-i] * qGuess + carry;
         carry = temp1 >> BaseLog;
         temp1 &= BaseMod;
         temp2 = shift[-i - 1] - temp1 + borrow;
         shift[-i - 1] = temp2 & BaseMod;
         borrow = temp2 >> BaseLog;
      }
      temp2 = shift[-i - 1] - carry + borrow;
      shift[-i - 1] = temp2 & BaseMod;
      borrow = temp2 >> BaseLog;
      q[j] += qGuess + borrow;
      if (borrow != 0) {
         carry = 0;
         for (i = n - 1; i >= 0; i--) {
            temp2 = shift[-i - 1] + b[-i] + carry;
            shift[-i - 1] = temp2 & BaseMod;
            carry = temp2 >> BaseLog;
         }
         shift[-i - 1] = (shift[-i - 1] + carry) & BaseMod;
      }
   }
   Q.deleteLeadingZeroes();
   R = U;
   if (scale > 1)
      tinyDivide(U, R, scale, r);
   R.deleteLeadingZeroes();
   Q.sign = n1.sign^n0.sign;
   return;
}

void tinyDivide(const bigNumber &A, bigNumber &Q, digit s, digit &R) {
   if (A.num != Q.num) {
      Q.num.clear();
      Q.num.resize(A.size, 0);
      Q.size = A.size;
   }
   const digit *a = &A.num[0];
   digit *q = &Q.num[0];
   R = 0;
   for(long i = A.size - 1; i >= 0; i--) {
      uLong temp = a[i] + ((uLong)R << BaseLog);
      q[i] = temp/s;
      R = temp - q[i]*s;
   }
   Q.deleteLeadingZeroes();
   Q.sign = A.sign ^ (s < 0);
}

bigNumber operator/ (const bigNumber &n0, const bigNumber &n1) {
   if (n1 == 0)
      throw 3;
   bigNumber R(0), Q(0);
   if ( n1.size == 1) {
        digit r;
        tinyDivide(n0, Q, n1.num[0], r);
        Q.sign = n0.sign ^ n1.sign;
        return Q;
   }
   bigDivide(n0, n1, Q, R);
   return Q;
}

bigNumber bigNumber::operator/= (const bigNumber &n1) {
   bigNumber temp(*this/n1);
   return *this = temp;
}

bigNumber operator% (const bigNumber &n0, const bigNumber &n1) {
   if (n1 == 0)
      throw 4;
   bigNumber R(0), Q(0);
   if ( n1.size == 1) {
        digit r;
        tinyDivide(n0, Q, n1.num[0], r);
        return r;
   }
   bigDivide(n0, n1, Q, R);
   return R;
}

bigNumber bigNumber::operator%= (const bigNumber &n1) {
   bigNumber temp(*this%n1);
   return *this = temp;
}

bigNumber pow(bigNumber n1, bigNumber n2) {
  bigNumber b(1);  
  if (n2.sign)
     throw 6;
  while (n2 != 0)
    if (n2%2 == 0) {
       n2 = n2 / 2;
       n1 = n1*n1;
    } else {
       n2--;
       b = b*n1;
    }
  return b;
}

void tinyMul(const bigNumber &A, digit B, bigNumber &C) {
   if (&A.num != &C.num) {
      C.num.clear();
      C.num.resize(A.size, 0);
      C.size = A.size;
   }
   digit *c = &C.num[0];
   const digit *a = &A.num[0];
   uLong temp = 0, carry = 0;
   for(long i = 0; i < A.size; i++) {
      temp = (uLong)a[i]*B + carry;
      carry = temp >> BaseLog;
      c[i] = temp & BaseMod;//temp - carry*BASE;
   }
   if(carry) {
       C.size++;
       C.num.push_back(carry);
   }
   C.sign = A.sign ^ (B < 0);
} 

bigNumber tinyMul1(const bigNumber &A, digit B) {
   bigNumber C(A);
   tinyMul(A, B, C);
   return C;
} 

void KaratsubaMul(const bigNumber &A, const bigNumber &B, bigNumber &C) {//A.size >= B.size 
   bigNumber a1(0), a2(0), b1(0), b2(0);                                 //Karatsuba algorithm
   C.size = A.size + B.size;
   C.num.resize(C.size);
   digit *c = &C.num[0];
   for (long i = 0; i < C.size; i++)
      c[i] = 0;
   a1.size = A.size/2;
   a2.size = (A.size + 1)/2;
   a1.num.resize(a1.size);
   memcpy(&a1.num[0], &A.num[A.size - a1.size], a1.size*sizeOfDigit);
   a2.num.resize(a2.size);
   memcpy(&a2.num[0], &A.num[0], a2.size*sizeOfDigit);
   if (B.size <= a2.size) {
      b2.size = B.size;
      b2.num.resize(b2.size);
      memcpy(&b2.num[0], &B.num[0], b2.size*sizeOfDigit);
   } else {
      b2.size = a2.size;
      b1.size = B.size - b2.size;
      b1.num.resize(b1.size);
      memcpy(&b1.num[0], &B.num[B.size - b1.size], b1.size*sizeOfDigit);
      b2.num.resize(b2.size);
      memcpy(&b2.num[0], &B.num[0], b2.size*sizeOfDigit);
   }
   long a2Size = a2.size, tSize = a1.size + b1.size;;
   a1.deleteLeadingZeroes();
   a2.deleteLeadingZeroes();
   b1.deleteLeadingZeroes();
   b2.deleteLeadingZeroes();
   bigNumber temp2 = a2*b2, temp3 = a1*b1, temp1 = (a1 + a2)*(b1 + b2) - temp2 - temp3;
   memcpy(c, &temp2.num[0], temp2.size*sizeOfDigit);
   memcpy(c + 2*a2Size, &temp3.num[0], temp3.size*sizeOfDigit);
   digit *ptr = c + a2Size, *tmp = &temp1.num[0], *cEnd = c + C.size;
   uLong carry = 0;
   for(long i = 0; i < temp1.size; i++) {
      carry += tmp[i] + *ptr;
      *ptr++ = carry & BaseMod;
      carry >>= BaseLog;
   }
   if (carry) {
      while (ptr < cEnd) {
         carry += *ptr;
         *ptr++ = carry & BaseMod;
         carry >>= BaseLog;
      }
      if (carry) {
         C.size++;
         C.num.push_back(carry);
      }
   }
   C.deleteLeadingZeroes();
   C.sign = A.sign ^ B.sign;
}

void bigNumber::deleteLeadingZeroes() {
   while(this->num[size - 1] == 0 && size >= 1) {///not sure but may be true
      size--;
      num.pop_back();
   }
   if (this->size == 0)
      this->sign = 0;
   return;
}

uLong min(uLong a, uLong b) {
   return (a < b) ? a : b;
}

void bigNumber::fromDecimal(uLong* &A, long &size, digit &R) {   //digit *a = A.num;
   R = 0;
   for(long i = 0; i < size; i++) { 
      uLong temp = R*Base3, temp2 = temp, tmp = 0;
      if (R != 0 && temp / R != Base3)
         tmp = 1;
      temp = temp2 + A[i];
      if (temp < min(A[i], temp2))
         tmp++;
      A[i] = (tmp << 33) + (temp >> BaseLog);
      R = temp & BaseMod;
   }
   long i = 0;
   while (A[i] == 0 && size > 0) {
      i++;
      size--;
   }
   if (i > 0) {
      uLong *tmp = new uLong[size];
      memcpy(tmp, A + i, size*sizeOfuLong);
      delete [] A;
      A = tmp; 
   }
}

bigNumber gcd(const bigNumber &a, const bigNumber &b) {
   bigNumber m(abs(a)), n(abs(b)), shift(0);
   if (m == 0)
      return n;
   if (n == 0)
      return m;
   while (m != n) {
      if(m.num[m.size - 1]%2 == 0 && n.num[n.size - 1]%2 == 0) {
         m /= 2;
         n /= 2;
         shift++;
      }
      if(m.num[m.size - 1]%2 == 0 && n.num[n.size - 1]%2 != 0)
         m /= 2;
      if(m.num[m.size - 1]%2 != 0 && n.num[n.size - 1]%2 == 0)
         n /= 2;
      if(m.num[m.size - 1]%2 != 0 && n.num[n.size - 1]%2 != 0 && m > n)
         m = (m - n)/2;
      if(m.num[m.size - 1]%2 != 0 && n.num[n.size - 1]%2 != 0 && m < n)
         n = (n - m)/2;      
   }
   return m*pow(2, shift);
}

bigNumber lcm(const bigNumber &a, const bigNumber &b) {
   return abs(a*b)/gcd(a, b);
}

bigNumber fib(const bigNumber &n) {
   if (n.sign) throw 7;
   if (n == 1 || n == 2)
      return 1;
   bigNumber n1(0), n2(1), count(n), answer(0);
   while (count-- >= 2) {
      answer = n1;
      n1 = n2;
      answer += n1;
      n2 = answer;
   }
   return answer;
}

unsigned int bigNumber::toUint() const {
   if (size == 0 || size > 2)//ignore this numbers
      return 0;
   if (size == 1)
      return num[0];
   else
      return num[1]*BASE + num[0];
}

bigNumber log10(bigNumber n) {
   bigNumber i = 0;
   while (n >= 10) {
      i++;
      n /= 10;
   }
   return i;
}

string bigNumber::toString(int n, int mode) {
   if (size == 0)
      return "0";
   std::ostringstream oss;
   if (n && abs(*this) > pow(10, n)) {
      bigNumber lg10(log10(abs (*this))), A(*this/pow(10, lg10 - 5)), B(A/100000);
      A = A%100000;
      oss << B << "." << A << "e+" << lg10;
   }
   else if (mode || *this < 10000000)
      oss << *this;
   else {
      bigNumber temp(*this);
      vector<int> v;
      while (temp != 0) {
         v.push_back((temp%1000).toUint());
         temp = temp/1000;
      }
      oss << v.back();
      v.pop_back();
      while (!v.empty()) {
         oss << '\'';
         int digitN = v.back(), pow = 100;
         v.pop_back();
         for (int i = 0; i < 3; i++) {
            oss << digitN/pow;
            digitN %= pow;
            pow /= 10;
         }
      }
   }
   return oss.str();
}

bigNumber operator>>(const bigNumber &n1, const bigNumber &n2) {
	if (n2 == 0)
		return n1;
	if (n2 > 0)
		return n1/pow(2, abs(n2));
	if (n2 < 0)
		return n1*pow(2, abs(n2));				
}

bigNumber operator<<(const bigNumber &n1, const bigNumber &n2) {
	if (n2 == 0)
		return n1;
	if (n2 > 0)
		return n1*pow(2, abs(n2));
	if (n2 < 0)
		return n1/pow(2, abs(n2));				
}

