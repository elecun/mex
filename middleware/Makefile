
# Makefile for MEX Middlware
# Author : Byunghun Hwang <bh.hwang@iae.re.kr>
# Usage : make ARCH=x86_64

# Makefile

OS := $(shell uname)

#Set Architecutre
ARCH := x86_64

#Compilers
CC := g++-8
GCC := gcc-8
LD_LIBRARY_PATH += -L./lib/x86_64
BUILDDIR		= ./dist/
INCLUDE_DIR = -I./
LD_LIBRARY_PATH += -L/usr/local/lib -L./lib/x86_64


# OS
ifeq ($(OS),Linux) #for Linux
	LDFLAGS = -Wl,--export-dynamic -Wl,-rpath=$(LD_LIBRARY_PATH)
	LDLIBS = -pthread -lboost_thread -lmosquitto -lboost_chrono -lboost_system
	GTEST_LDLIBS = -lgtest
endif

$(shell mkdir -p $(BUILDDIR))

#if release(-O3), debug(-O0)
CXXFLAGS = -O3 -fPIC -Wall -std=c++17 -D__cplusplus=201703L

#custom definitions
CXXFLAGS += -D__MAJOR__=0 -D__MINOR__=0 -D__REV__=1
RM	= rm -rf

#directories

INCLUDE_FILES = ./include/
SOURCE_FILES = ./
INSTALL_DIR = /usr/local/bin/

# Make
mex_load:	$(BUILDDIR)mex_load.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_load.o:	$(SOURCE_FILES)mex_load/mex_load.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_tpm:	$(BUILDDIR)mex_tpm.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_tpm.o:	$(SOURCE_FILES)mex_tpm/mex_tpm.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_rpm:	$(BUILDDIR)mex_rpm.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_rpm.o:	$(SOURCE_FILES)mex_rpm/mex_rpm.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_temperature:	$(BUILDDIR)mex_temperature.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_temperature.o:	$(SOURCE_FILES)mex_temperature/mex_temperature.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_relay:	$(BUILDDIR)mex_relay.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_relay.o:	$(SOURCE_FILES)mex_relay/mex_relay.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_plc:	$(BUILDDIR)mex_plc.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_plc.o:	$(SOURCE_FILES)mex_plc/mex_plc.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

mex_scheduler:	$(BUILDDIR)mex_scheduler.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)
$(BUILDDIR)mex_scheduler.o:	$(SOURCE_FILES)mex_scheduler/mex_scheduler.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

all : mex_load mex_tpm mex_relay mex_plc mex_scheduler mex_rpm mex_temperature

deploy : FORCE
	cp $(BINDIR)
clean : FORCE 
		$(RM) $(BUILDDIR)*.o
FORCE : 