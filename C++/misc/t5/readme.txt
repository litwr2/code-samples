To build and run this program and a supplied test you need the following programs (they are part of almost any Linux distribution): g++, make, bash, tree.  Program `tree` required for the test only.  It is possible to use almost any Unix shell instead of bash.

To build the program, run
    make

To execute the program, run
    test-fme

To test it, run
    make test

The test runs script-file `results-compare.sh' which uses commands from file `test-fme.cmd'.  The test compares two directory listings created using two methods.  The first method uses `fme-test' and the second one uses standard Linux shell commands (mkdir, cp, touch, mv, rm).  You can edit or just replace the file `test-fme.cmd' to perform other tests.


