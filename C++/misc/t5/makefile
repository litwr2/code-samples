CXXFLAGS = -g
LDFLAGS = -lstdc++

test-fme: test-fme.o parse.o fme.o

test: test-fme
	./results-compare.sh

clean:
	rm -f *.o test-fme
