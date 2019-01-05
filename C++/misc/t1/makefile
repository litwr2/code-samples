BOOST_INCLUDE_PATH = /mnt/store/current/src/

CXXFLAGS = -O3 -s -I$(BOOST_INCLUDE_PATH)boost_1_67_0 -I$(BOOST_INCLUDE_PATH)boost_1_67_0/boost
#CXXFLAGS = -g -I$(BOOST_INCLUDE_PATH)boost_1_67_0 -I$(BOOST_INCLUDE_PATH)/boost_1_67_0/boost

LDFLAGS = -L/usr/local/lib
LDLIBS = -lboost_system -lwebsockets -lstdc++ -lssl -lcrypto -pthread

loader: loader.o sha1z.o

clean:
	rm *.o loader
