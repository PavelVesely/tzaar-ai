/*
 * The module tzaarinit has functions for initializing arrays and variables
 * with information about the current position.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "tzaarinit.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void CountZoneOfControl()
{				//could be speeded up, but it probably doesn't matter; use not often; it's slow
	DPRINT2("Counting ZOC slowly ...");
	FOR(i, 0, STONE_TYPES) {
		zoneOfControl[i] = 0;
	}
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		threatenByCounts[i] = 0;
	}
	i32 mv = moveNumber, pl = player;
	moveNumber = 1;
	player = WHITE;
	Move *m, *tmp;
	GenerateAllMoves(&m);
	while (m != null) {
		if (threatenByCounts[m->to] == 0)
			zoneOfControl[board[m->to] + 3]++;
		threatenByCounts[m->to]++;

		tmp = m;
		m = m->next;
		FreeMove(tmp);
	}
	m = null;
	player = BLACK;
	GenerateAllMoves(&m);
	while (m != null) {
		if (threatenByCounts[m->to] == 0)
			zoneOfControl[board[m->to] + 3]++;
		threatenByCounts[m->to]++;
		tmp = m;
		m = m->next;
		FreeMove(tmp);
	}
	moveNumber = mv;
	player = pl;
}

void CountMaterialValue()
{
	DPRINT2("Counting material slowly ...");
	materialValue = 0;
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		DPRINT2("%d(%d) ", board[i], stackHeights[i]);
		if (board[i] != BORDER && board[i] != EMPTY)
			materialValue += (board[i] > 0 ? 1 : -1) * StackQuality(stackHeights[i], board[i], i);
	}
}

void CountHash()
{
	DPRINT2("Counting hash slowly ...");
	hash = 0;
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		if (board[i] != BORDER)
			hash ^= HashedPositions[i][board[i] + 3][stackHeights[i]];
	}
	if (player == BLACK)
		hash ^= 1;
}

void InitBoard(i32 setup)
{				//not used
	if (setup == STANDARD) {
		for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++)
			board[i] = StandardBoard[i];
	} else if (setup == RANDOM) {
		srand(time(NULL));
		i32 rnd[BOARD_ARRAY_SIZE];
		for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
			rnd[i] = StandardBoard[i];
		}
		i32 st = TOTAL_STONES;
		for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
			if (StandardBoard[i] != BORDER) {
				i32 j = rand() % st, k = 0;
				while (j > -1) {
					if (rnd[k] != BORDER)
						j--;
					k++;
				}
				k--;
				st--;
				board[i] = rnd[k];
				rnd[k] = BORDER;
			} else
				board[i] = BORDER;
		}
	}
	// for every board setup, stack heights are equally 1
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		stackHeights[i] = 1;
	}
	for (i32 i = 0; i < STONE_TYPES; i++) {
		counts[i] = countsByHeight[1][i] = InitialStoneCounts[i];
		highestStack[i] = 1;
	}
	player = WHITE;
	moveNumber = 1;
	turnNumber = 1;
	stoneSum = 60;
	CountZoneOfControl();
	CountMaterialValue();
	CountHash();
	value = 0;
}
