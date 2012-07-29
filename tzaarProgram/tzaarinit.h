/*
 * In the header file there are constants for initializing the board.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef TZAARINIT_H_INCLUDED
#define TZAARINIT_H_INCLUDED

#include "tzaarlib.h"

// -----------------
// CONSTANTS
// ----------------

// board setup
#define STANDARD 0
#define RANDOM 1
#define MANUAL 2

//standard board
static __attribute__((unused))
i32 StandardBoard[] = {
	-1  ,  1  ,  1  ,  1  ,  1  , 100 , 100 , 100 , 100 ,
	-1  , -2  ,  2  ,  2  ,  2  , -1  , 100 , 100 , 100 ,
	-1  , -2  , -3  ,  3  ,  3  , -2  , -1  , 100 , 100 ,
	-1  , -2  , -3  , -1  ,  1  , -3  , -2  , -1  , 100 ,
	 1  ,  2  ,  3  ,  1  , 100 , -1  , -3  , -2  , -1  ,
	100 ,  1  ,  2  ,  3  , -1  ,  1  ,  3  ,  2  ,  1  ,
	100 , 100 ,  1  ,  2  , -3  , -3  ,  3  ,  2  ,  1  ,
	100 , 100 , 100 ,  1  , -2  , -2  , -2  ,  2  ,  1  ,
	100 , 100 , 100 , 100 , -1  , -1  , -1  , -1  ,  1
};

// ---------------
// FUNCTIONS -- TZAAR INIT
// ---------------

void CountMaterialValue();
void CountHash();
void CountZoneOfControl();
void InitBoard(i32 setup);

#endif	// TZAARINIT_H_INCLUDED
