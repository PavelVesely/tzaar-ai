/*
 * The module tzaarSaveLoad contains functions for loading and saving positions
 * and a function for saving best moves.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "tzaarSaveLoad.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/// Save position also with counted information like ZOC, hash, stack counts ...
i32 SaveWholePosition(const char *fileName)
{
	ASSERT(fileName != null, "save: fileName null");
	FILE *f = fopen(fileName, "w");
	if (f == null) {
		printf("Cannot save to file '%s'\n", fileName);
		return ERROR;
	}
	fprintf(f, "%d %d %d %d %d %llu\n", turnNumber, player, moveNumber, materialValue, value, hash);
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		fprintf(f, "%d ", board[i]);
	}
	fprintf(f, "\n");
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		fprintf(f, "%d ", threatenByCounts[i]);
	}
	fprintf(f, "\n");
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		fprintf(f, "%d ", stackHeights[i]);
	}
	fprintf(f, "\n");
	FOR(i, 0, STONE_TYPES) {
		fprintf(f, "%d ", counts[i]);
	}
	fprintf(f, "\n");
	FOR(i, 0, STONE_TYPES) {
		fprintf(f, "%d ", zoneOfControl[i]);
	}
	fprintf(f, "\n");
	if (fclose(f) == EOF) {
		DPRINT("error closing file %s", fileName);
		return ERROR;
	}
	return OK;
}

/// Save only basic information about the position
i32 SavePosition(const char *fileName)
{
	ASSERT(fileName != null, "save: fileName null");
	FILE *f = fopen(fileName, "w");
	if (f == null) {
		printf("Cannot save to file '%s'\n", fileName);
		return ERROR;
	}
	if (abs(value) == WIN) {
		fprintf(f, "END, SOMEONE HAS WON, value:\n%d\n", value);
	} else {
		ASSERT(!IsEndOfGame(), "saving end position!! val %d", value);
	}
	fprintf(f, "%d\n", player); //now moveNumber should be always 1 (it's because of first move and simplicity)
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		if (i > 0 && i % 9 == 0)
			fprintf(f, "\n");
		fprintf(f, "%d ", board[i]);
	}
	fprintf(f, "\n");
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		if (i > 0 && i % 9 == 0)
			fprintf(f, "\n");
		fprintf(f, "%d ", stackHeights[i]);
	}
	fprintf(f, "\n");
	if (fclose(f) == EOF) {
		DPRINT("error closing file %s", fileName);
		return ERROR;
	}
	return OK;
}

/// Save basic information about the position
i32 LoadPosition(const char *fileName)
{
	ASSERT(fileName != null, "open: fileName null");
	FILE *f = fopen(fileName, "r");
	if (f == null) {
		printf("Cannot open file '%s'\n", fileName);
		return ERROR;
	}
	fscanf(f, "%d", &player);
	DPRINT2("player %d", player);
	FOR(i, 0, STONE_TYPES) {
		counts[i] = 0;
		highestStack[i] = 0;
		FOR(j, 0, MAX_STACK_HEIGHT) {
			countsByHeight[i][j] = 0;
		}
	}
	i32 cnt = 0;
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		fscanf(f, "%d", &(board[i]));
		if (board[i] != 100) {
			counts[board[i] + 3]++;
			cnt++;
		}
	}
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		i32 c;
		fscanf(f, "%d", &c);
		stackHeights[i] = c;
		if (board[i] != BORDER) {
			countsByHeight[board[i] + 3][c]++;
			if (c > highestStack[board[i] + 3]) {
				highestStack[board[i] + 3] = c;
			}
		}
	}
	value = 0;
	stoneSum = 0;
	FOR(i, 0, 7) stoneSum += counts[i];
	stoneSum -= counts[3];
	turnNumber = 1;	//no special handeling for the first move
	moveNumber = 1;
	CountZoneOfControl();
	CountMaterialValue();
	CountHash();
	DBG2(printZOCDebug());
	DBG2(printHighestDebug());
	if (fclose(f) == EOF) {
		DPRINT("error closing file %s", fileName);
		return ERROR;
	}
	ASSERT(!IsEndOfGame(), "LOAD END POSITION!!!")
	return OK;
}

i32 SaveBestMoves(const char *fileName, Move * m1, Move * m2)
{
	DBG2(printHighestDebug());
	ASSERT(fileName != null, "save: fileName null");
	ASSERT(m1 != null, "save: m1 null");
	//for the first move, m2 == null; and also for winning pos (win with one move)
	FILE *f = fopen(fileName, "w");
	if (f == null) {
		printf("Cannot save to file '%s'\n", fileName);
		return ERROR;
	}
	const char *f1f = IndexToFieldName(m1->from);
	const char *f1t = IndexToFieldName(m1->to);
	DPRINT("saving move 1: from %s, to %s", f1f, f1t);
	fprintf(f, "%s %s\n", f1f, f1t);
	if (m2 == null) {	//first move of white
		fprintf(f, "-2\n");
	} else if (m2->from == -1) {
		fprintf(f, "-1\n");
	} else {
		const char *f2f = IndexToFieldName(m2->from);
		const char *f2t = IndexToFieldName(m2->to);
		fprintf(f, "%d %s %s\n", board[m2->from] * board[m2->to] < 0, f2f, f2t);
	}
	//saving other information about position and search
	fprintf(f, "%0.3f %d\n", searchDuration, value);
	if (fclose(f) == EOF) {
		DPRINT("error closing file %s", fileName);
		return ERROR;
	}
	return OK;
}
