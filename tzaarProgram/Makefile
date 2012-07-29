# Makefile for building Tzaar library and a simple program tzaarmain 
#
# Author: Pavel Veselý

# ---- Settings ----
# directories and files
MAINFILE = tzaar
# due to simplicity, all is compiled together
CFILES := $(wildcard *.c)
HFILES := $(wildcard *.h)
# gcc and its flags
GCC = gcc
GCCFLAGS = -std=c99
WARNINGFLAGS =  -Wall -Winline -Wextra
OPTFLAGS = -O3 -funroll-loops --param inline-unit-growth=1000 --param large-function-growth=1000 

all: tzaar

tzaar: $(LIBTZAAR) $(CFILES) $(HFILES)
	$(GCC) $(GCCFLAGS) $(OPTFLAGS) $(WARNINGFLAGS) $(CFILES) -o $(MAINFILE)
clean:
	rm -f $(MAINFILE)
	
.PHONY: clean
