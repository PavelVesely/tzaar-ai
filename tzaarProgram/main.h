/*
 * In the header file there are arrays for parsing command line arguments using
 * getopt library.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "tzaarlib.h"
#include "tzaarSaveLoad.h"
#include <getopt.h>

static __attribute__ ((unused))
struct option long_options[] = {
	{"ai", 1, 0, 'a'},
	{"bestmove", 1, 0, 'b'},
	{"help", 1, 0, 'h'},
	{"execute", 1, 0, 'e'},
	{"timelimit", 1, 0, 't'},
	{0, 0, 0, 0}
};

static __attribute__ ((unused))
const char *options = "a:t:e:b:h";

i32 ProcessPosition(i32 ai, i32 time, const char *fileWithPosition, const char *fileBestMoves, const char *fileEorExecutedPos);

#endif				// MAIN_H_INCLUDED
