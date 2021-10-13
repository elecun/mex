
# Makefile for MEX Application
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
OUTDIR		= ./../../dist/
BUILDDIR		= ./../../dist/
INCLUDE_DIR = -I./
LD_LIBRARY_PATH += -L/usr/local/lib -L./lib/x86_64


# OS
ifeq ($(OS),Linux) #for Linux
	LDFLAGS = -Wl,--export-dynamic -Wl,-rpath=$(LD_LIBRARY_PATH)
	LDLIBS = -pthread -lboost_thread -lmosquitto -lboost_chrono -lmodbus
	GTEST_LDLIBS = -lgtest
endif

$(shell mkdir -p $(OUTDIR))
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
mex_ap0:	$(BUILDDIR)main.o
		$(CC) $(LDFLAGS) $(LD_LIBRARY_PATH) -o $(BUILDDIR)$@ $^ $(LDLIBS)

$(BUILDDIR)main.o:	$(SOURCE_FILES)main.cc
					$(CC) $(CXXFLAGS) $(INCLUDE_DIR) -c $^ -o $@

all : mex_ap0

deploy : FORCE
	cp $(BINDIR)
clean : FORCE 
		$(RM) $(BUILDDIR)*.o
FORCE : 