LDFLAGS = -lstdc++ -lm

gtest: gtest.o graph.o

gtest.o: gtest.cpp graph.h list.h

graph.o: graph.cpp graph.h

clean:
	rm *.o *~ gtest
