#include <iostream>
#include <exception>
#include <cmath>
using namespace std;

int ediv(int n, int d) {
    if (d == 0) throw 1;
    return n/d;
}

double ln(double x) {
    if (x <= 0) throw 2;
    return log(x);
}

void deepjmp() {
    static int n;
    if (++n < 10)
        deepjmp();
    else
       throw overflow_error("stack recursion depth is 10");
}

int main() {
    int i = -5, j = 0;
    while (1) {
        try {
            cout << ediv(i, j) << endl;
            cout << ln(i) << endl;
            if (j == 1)
                throw 7.4;
            if (i == 1) deepjmp();
                throw 5;
        }
        catch (int k) {
            switch (k) {
            case 1:
                cerr << "bad division\n";
                j = 1;
                continue;
            case 2:
                cerr << "bad logarithm\n";
                i = 1;
                continue;
            default:
                cerr << "exiting\n";
            }
        }
        catch (exception& e) {
            cerr << "exception is caught '" << e.what() << "'\n";
            i = 2;
            continue;
        }
        catch (...) {
           cerr << "other error\n";
           j = 2;
           continue;
        }
        break; //end of loop
    }
    return 0;
}
