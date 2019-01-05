#include<cassert>
#include<iostream>
#include<typeinfo>
#include<string>
using namespace std;

struct A {
   int a;
   virtual ~A(){}
} a, *p;
struct B : A {
   double a;
   ~B(){}
} b;
int main() {
    p = &b;
    cout << typeid(*p).name() << typeid(a).name() << endl;
    cout << typeid(p->a).name() << typeid(a.a).name() << endl;
    assert((string)typeid(*p).name() + typeid(a).name() + typeid(p->a).name() + typeid(a.a).name() == "1B1Aii");
    return 0;
}

