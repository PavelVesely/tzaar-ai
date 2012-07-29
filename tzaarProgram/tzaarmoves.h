/*
 * In this header file there are constants for the Move Ordering and arrays
 * for possible directions that are used in the functions for generating moves.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef TZAARMOVES_H_INCLUDED
#define TZAARMOVES_H_INCLUDED

#include "tzaarlib.h"

// -----------------
// CONSTANTS
// ----------------
// ordering moves
#define SORT_CAPTURE_COUNT_MULT 20
#define SORT_STACK_COUNT_MULT 3
#define SORT_HISTORY_PRUNES_MULT 20
#define SORT_STACK_BONUS 6

static __attribute__ ((unused))
i32 CapturingStackHeightAdvantage[] = { 0, 0, 15, 50, 160, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310 };

// for generating moves
#define DIRECTION_COUNT 6
// difference in x direction
static __attribute__ ((unused))
i32 dxs[] = { 1, -1, 1, -1, 0, 0 };
// difference in y direction
static __attribute__ ((unused))
i32 dys[] = { 1, -1, 0, 0, 1, -1 };

// for converting between field index and name
static __attribute__((unused))
char *FieldNames[] = {
	"A1", "B1", "C1", "D1", "E1", "-" , "-" , "-" , "-" ,
	"A2", "B2", "C2", "D2", "E2", "F1", "-" , "-" , "-" ,
	"A3", "B3", "C3", "D3", "E3", "F2", "G1", "-" , "-" ,
	"A4", "B4", "C4", "D4", "E4", "F3", "G2", "H1", "-" ,
	"A5", "B5", "C5", "D5", "-" , "F4", "G3", "H2", "I1",
	"-" , "B6", "C6", "D6", "E5", "F5", "G4", "H3", "I2",
	"-" , "-" , "C7", "D7", "E6", "F6", "G5", "H4", "I3",
	"-" , "-" , "-" , "D8", "E7", "F7", "G6", "H5", "I4",
	"-" , "-" , "-" , "-" , "E8", "F8", "G7", "H6", "I5"
};


// ---------------
// FUNCTIONS -- TZAAR MOVES
// ---------------

i32 StackQuality(i32 stackHeight, i32 stone, i32 fieldTo);
void ZOCAddedStoneFrom(i32 field, i32 to);
void ZOCAddedStoneTo(i32 field);
void ZOCRemovingStoneFrom(i32 field, i32 to);
void ZOCRemovingStoneTo(i32 field);
i32 IsEndOfGame();
i32 IsMovePossible(Move * move);
void ExecuteMove(Move * move);
void RevertLastMove();
i32 FieldNameToIndex(const char *field);
const char *IndexToFieldName(i32 index);
void GenerateAllMoves(Move ** moves);
bool HasLegalMoves();
void GenerateAllMovesSorted(Move ** moves);
void GenerateAllMovesSortedMove1(Move ** moves);	//, i32 depth
void GenerateAllMovesSortedMove2(Move ** moves);
void GenerateBestMovesSorted(Move ** moves, i32 maxMoves);
void FreeMove(Move * move);
void FreeAllMoves(Move * move, Move * exception);
void FreeAllMovesWithoutException(Move * move);
Move *CloneMove(Move * move);

void printZOCDebug();
void printHighestDebug();

#endif				// TZAARMOVES_H_INCLUDED
