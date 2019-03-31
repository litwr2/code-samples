CXXFLAGS = -g
LDFLAGS = -lstdc++

test-fme: test-fme.o parse.o fme.o

test: test-fme
	./tests-compare.sh

clean:
	rm -f *.o test-fme
