
# Makefile for MEX Middlware
# Author : Byunghun Hwang <bh.hwang@iae.re.kr>
# Usage : make ARCH=x86_64

# Makefile

OS := $(shell uname)

#Set Architecutre
ARCH := x86_64

#Compilers
CC := g++
GCC := gcc
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


mex_ap0:	$(BUILDDIR)mex_ap0.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)

mex_ap1:	$(BUILDDIR)mex_ap1.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)

mex_s0:	$(BUILDDIR)mex_s0.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)

mex_usb0:	$(BUILDDIR)mex_usb0.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)

$(BUILDDIR)mex_ap0.o:	$(SOURCE_FILES)mex_ap0/mex_ap0.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@
$(BUILDDIR)mex_ap1.o:	$(SOURCE_FILES)mex_ap1/mex_ap1.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@
$(BUILDDIR)mex_s0.o:	$(SOURCE_FILES)mex_s0/mex_s0.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@
$(BUILDDIR)mex_usb0.o:	$(SOURCE_FILES)mex_usb/mex_usb0.cc
						$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

all : mex_loadcell

deploy : FORCE
	cp $(BINDIR)
clean : FORCE 
		$(RM) $(BUILDDIR)*.o
FORCE : 