/*
 * In the header file there are definitions of structures, types, macros,
 * global arrays and variables used throughout the library.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef TZAARLIB_H_INCLUDED
#define TZAARLIB_H_INCLUDED

// TYPES
//#include <stdio.h>
#include <stdint.h>

//typedef uint_fast64_t thash;
typedef unsigned long long thash;
//typedef int_fast32_t i32;
typedef int i32;
//typedef uint_fast32_t u32;
typedef unsigned u32;

// DEBUG -- full (could slow down program) and fast 
#define DEBUG

//#define SEARCHFORBUG

//helpful macros
#define null NULL
#define true 1
#define false 0
#define bool i32
#define MALLOC(a) ((a *)malloc(sizeof(a)))
#define FOR(i,m,n) for (i32 i=m; i<n; i++)
#define SIGND(a,b) ((a) == (b) ? 0 : ((a) < (b) ? 1 : -1))
#define SIGN(a) ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#define MIN(a,b) ((a) <= (b) ? (a) : (b))

#ifdef DEBUG
	// Prints a message to the output. Flushing the buffer is because of program termination.
	#define DPRINT(format,...) { printf("D: "); printf(format, ## __VA_ARGS__); printf("\n"); fflush(stdout); }
	// Checks the condition and if it does not hold, prints a message.
	#define ASSERT(con,error,...) { if (!(con)) { printf("ASSERTATION FAILED: "); printf(error, ## __VA_ARGS__); printf("\n"); fflush(stdout); }}
	// Command used only in the case of debugging
	#define DBG(cmd) { cmd; }
	// Command used only in the case of debugging, this macro is used for declarations
	#define DBGDECL(cmd) cmd;
#else
	#define DPRINT(format,...) { }
	#define DPRINT2(format,...) { }
	#define ASSERT(con,error,...) { }
	#define DBG(cmd) {}
	#define DBGDECL(cmd) {}
#endif

// logging
#ifdef SEARCHFORBUG
// these macros are used only when we want to find a bug
	#define DPRINT2(format,...) { printf("D: "); printf(format, ## __VA_ARGS__); printf("\n"); fflush(stdout); }
	#define ASSERT2(con,error,...) { if (!(con)) { printf("ASSERTATION FAILED: "); printf(error, ## __VA_ARGS__); printf("\n"); fflush(stdout); }}
	#define DBG2(cmd) { cmd; }
#else
	#define DPRINT2(format,...) { }
	#define ASSERT2(con,error,...) { }
	#define DBG2(cmd) { }
#endif

// -----------------
// CONSTANTS
// ----------------
#define BOARD_ARRAY_SIZE 81
#define INVALID_INPUT -1

// types of fields
#define EMPTY 0
#define BORDER 100
#define TOTT 1
#define TZARRA 2
#define TZAAR 3

//counts
#define CTOTTS 15
#define CTZARRAS 9
#define CTZAARS 6
#define TOTAL_STONES 60		//for both players
#define STONE_TYPES 7
#define MAX_STACK_HEIGHT 15
#define MAX_MOVES 121		//per moveNumber -- it's 60 * 2 (capture + pass)
#define MAX_POSSIBILITIES 200	// per move of a player, it's 30*6 (max. 30 stones per player and 6 directions)

// players
#define WHITE 1
#define BLACK -1

//standard return values
#define OK 0
#define ERROR 1
#define CANNOT_LOAD_POSITION 2
#define BAD_STONE 3
#define CANNOT_SAVE_POSITION 4
#define GET_BEST_MOVE_ERROR 5
#define FILE_WITH_POSITION_NULL 6
#define FILE_FOR_BEST_MOVES_NULL 7

//AI constants
#define WIN 2000000000

#define ALPHABETA_DEPTH 5	// for simple AB without ID
#define AI_TIME_LIMIT 30
#define AI_RANDOM_MARGIN 20
#define AI_RANDOM_MARGIN_BIGGER 5000
#define MIN_AB_DEPTH 5
#define BEGINNER_AB_DEPTH 4
#define INTERMEDIATE_AB_DEPTH 5

//types of AI
#define HUMAN 0			// not supported :)
#define AIALPHABETA 1
#define AIALPHABETA_ID 2
#define AIALPHABETA_ID_PV 3
#define AIALPHABETA_ID_PV_MO 4
#define AIALPHABETA_ID_MO 5
#define AIALPHABETA_RANDOM 6
#define AIALPHABETA_ID_PV_MO_SCOUT 7
#define AIALPHABETA_ID_PV_MO_HISTORY 8
#define AIALPHABETA_ID_PV_MO_SCOUT_HISTORY 9
#define AIALPHABETA_MAX 9
#define AIALPHABETA_BEST 9
#define DFPNS 20
#define DFPNS_EPS_TRICK 21
#define WEAK_PNS 22
#define DFPNS_EVAL_BASED 23
#define DFPNS_WEAK_EPS_EVAL 24
#define DFPNS_DYNAMIC_WIDENING_EPS_EVAL 25
#define DFPNS_BEST 24
#define DFPNS_MAX 25
#define BEGINNERS_AI 40		// level challenging for beginners
#define INTERMEDIATE_AI 41	// for intermediate players
#define AICOMBI_RANDOM_AB_PNS 42 // for experts, the best
#define MAIN_AI 42		

static __attribute__((unused))
i32 BranchingFactorByFreeFields[] =
	{9508, 9204, 9000, 8256, 8000, 7428, 7000, 6839, 6400, 5980, 5700, 5430, 5000, 4779, 4400, 4262, 4000, 3641, 3400, 3272, 3000, 2745, 2500, 2381, 2100, 1963, 1774, 1652, 1400, 1355, 1154, 1080, 950, 854, 762, 669, 640, 478, 438, 353, 300, 233, 150, 144, 100, 86, 60, 44, 34, 21, 15, 9, 3, 1, 1, 1, 1, 1, 1, 1, 1 };
static __attribute__((unused))
i32 AB_ID_timeMultByFreeFieldsEvenDepth[] =
	{ 200,  180,  160,  140,  120,  110,  105,  100,   98,   96,   96,   95,   94,   92,   91,   90,   89,   88,   86,   85,   83,   82,   81,   80,   78,   76,   74,   72,   70,   68,   66,   64,   62,   60,   55,   50,   45,   40,  33,  25,  18,  13,  11,  10,   9,   8,   7,   7,   6,   5,   5,   4,   4,   3,  3,  2,  2,  1,  1, 1, 1, 1};
static __attribute__((unused))
i32 AB_ID_timeMultByFreeFieldsOddDepth[] =
	{ 100,  100,  100,  100,   80,   60,   50,   40,   35,   30,   29,   28,   27,   26,   26,   26,   25,   24,   23,   22,   21,   20,   19,   18,   17,   16,   15,   14,   13,   12,   11,   10,   10,    9,    9,    8,    8,    7,   7,   6,   6,   5,   5,   4,   4,   3,   3,   3,   3,   2,   2,   2,   2,   2,  2,  2,  1,  1,  1, 1, 1, 1};

static __attribute__ ((unused))
i32 InitialStoneCounts[] = { CTZAARS, CTZARRAS, CTOTTS, 0, CTOTTS, CTZARRAS, CTZAARS };

typedef struct move {
	i32 from, to;		//pass move <=> from == -1 and to == -1;
	struct move *next;	//for generating, not history
	i32 oldStackHeight, oldStone;
	i32 value;		// value
} Move;

typedef struct fullMove {	//for pns
	Move *m1, *m2;
} FullMove;

// Position reprezentation
extern i32 board[BOARD_ARRAY_SIZE];
extern i32 stackHeights[BOARD_ARRAY_SIZE];
extern i32 player, moveNumber;

// History
extern i32 turnNumber;
extern Move *history[MAX_MOVES];

// Position properties useful for evaluation function and moves sorting
extern i32 counts[STONE_TYPES];
extern i32 value, materialValue, stoneSum;
extern thash hash;
extern i32 highestStack[STONE_TYPES];
extern i32 countsByHeight[STONE_TYPES][MAX_STACK_HEIGHT];
extern i32 zoneOfControl[STONE_TYPES], threatenByCounts[BOARD_ARRAY_SIZE];

// For saving
extern double searchDuration;

// For searching (AB and PNS)
extern i32 currDepth;		//for tests on AB
extern u32 searchedNodes;

// Debug constants
#ifdef DEBUG
extern i32 moveAlive, entryAlive, ttHit, ttFound, ttKick, prunedCount, entry2Alive, tt2Kick, tt2Hit, tt2Found, ttCollision;
#endif

// ---------------
// FUNCTIONS -- TZAAR LIB
// ---------------
i32 GetBestMove(i32 ai, i32 time, Move **move1, Move **move2, bool recallAI);

#include "tzaarmoves.h"
#include "tzaarinit.h"
#include "pns.h"
#include "alphaBeta.h"

#endif				// TZAARLIB_H_INCLUDED
