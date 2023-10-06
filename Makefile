 CC = gcc
 COPTS = -g -Wall -fPIC
 CXX = g++
 CXXOPTS = -g -Wall -fPIC
 LIBS = -lm -lstdc++ -lpthread -ldl

############################### Various ##############################

### -rpath arguments for finding .so objects in pre-specified locations
EXTRA_DIR = .
RPATH = -rpath=$(EXTRA_DIR)

### decide on debugging level(s)
 COPTS += -D  _DEBUG_
#COPTS += -D  _DEBUG2_
#COPTS += -D  _DEBUG3_
 COPTS += -DNO_DEBUG_TERM_
#COPTS += -D  _DEBUG_UTIL_
#COPTS += -D  _DEBUG_INSHA_
 COPTS += -I $(EXTRA_DIR)

 CXXOPTS += -D  _DEBUG_
#CXXOPTS += -D  _DEBUG2_

############################### Target ##############################
all: objs
	$(CC) $(COPTS) -Wl,-rpath=. main.c \
         hdfy_stl.o stl.o \
         hdfy_obj.o inobj.o \
         $(LIBS)

objs:
	$(CC) $(COPTS) -c stl.c
	$(CC) $(COPTS) -c hdfy_stl.c
	$(CXX) $(CXXOPTS) -c inobj.cpp
	$(CC) $(COPTS) -c hdfy_obj.c

clean:
	rm -f  *.o a.out 

