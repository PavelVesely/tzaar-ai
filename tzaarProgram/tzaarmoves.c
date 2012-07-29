/*
 * The module tzaarmoves contains functions for generating, executing and
 * reverting moves. There are also helping functions for deallocating memory
 * (free a~single move or a~linked list of moves), determining whether someone
 * won in the current position, updating the zone of control after executing
 * or reverting move and converting between a~field index and a~field name
 * (for example field on index 3 has name D1).
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "tzaarmoves.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/// Returns the value of a stack on the field fieldTo with the height stackHeight
inline __attribute__ ((always_inline))
i32 StackQuality(i32 stackHeight, i32 stone, i32 fieldTo)
{
	return StackHeightValue[stackHeight] * StackByCountValue[counts[stone + 3]] +
	    StackValueByField[stackHeight][fieldTo] * FIELD_VALUE_MULT;
}

/// Updating ZOC after removing a stone on the field to
inline __attribute__ ((always_inline))
void ZOCRemovingStoneTo(i32 field)
{				// supposing that stone is still on its old place ...
	FOR(j, 0, DIRECTION_COUNT) {
		i32 cx = field % 9 + dxs[j];
		i32 cy = field / 9 + dys[j];
		i32 curr = cy * 9 + cx;
		while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
			cx += dxs[j];
			cy += dys[j];
			curr = cy * 9 + cx;
		}
		if (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] != BORDER && board[field] * board[curr] < 0) {
			if (stackHeights[field] >= stackHeights[curr]) {
				threatenByCounts[curr]--;
				if (threatenByCounts[curr] == 0) {
					zoneOfControl[board[curr] + 3]--;
				}
			}
			if (stackHeights[field] <= stackHeights[curr]) {
				threatenByCounts[field]--;
				if (threatenByCounts[field] == 0) {
					zoneOfControl[board[field] + 3]--;
				}
			}
		}
	}
}

/// Updating ZOC after removing a stone on the field from
inline __attribute__ ((always_inline))
void ZOCRemovingStoneFrom(i32 field, i32 to)
{				// supposing that stone is still on its old place ...
	i32 opposites[DIRECTION_COUNT / 2];
	FOR(j, 0, DIRECTION_COUNT / 2) opposites[j] = -1;
	FOR(j, 0, DIRECTION_COUNT) {
		i32 cx = field % 9 + dxs[j];
		i32 cy = field / 9 + dys[j];
		i32 curr = cy * 9 + cx;
		while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
			cx += dxs[j];
			cy += dys[j];
			curr = cy * 9 + cx;
		}
		if (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] != BORDER) {
			i32 o = opposites[j / 2];
			if (o == -1)
				opposites[j / 2] = curr;
			else if (board[o] * board[curr] < 0 && o != to && curr != to) {	// stones hidden by removed stone
				if (stackHeights[o] >= stackHeights[curr]) {
					if (threatenByCounts[curr] == 0) {
						zoneOfControl[board[curr] + 3]++;
					}
					threatenByCounts[curr]++;
				}
				if (stackHeights[o] <= stackHeights[curr]) {
					if (threatenByCounts[o] == 0) {
						zoneOfControl[board[o] + 3]++;
					}
					threatenByCounts[o]++;
				}
			}
			if (board[field] * board[curr] < 0) {
				if (stackHeights[field] >= stackHeights[curr]) {
					threatenByCounts[curr]--;
					if (threatenByCounts[curr] == 0) {
						zoneOfControl[board[curr] + 3]--;
					}
				}
				if (stackHeights[field] <= stackHeights[curr]) {
					threatenByCounts[field]--;
					if (threatenByCounts[field] == 0) {
						zoneOfControl[board[field] + 3]--;
					}
				}
			}
		}
	}
}

/// Updating ZOC after adding a stone on the field from
inline __attribute__ ((always_inline))
void ZOCAddedStoneFrom(i32 field, i32 to)
{				// supposing stone is already on its place 
	i32 opposites[DIRECTION_COUNT / 2];
	FOR(j, 0, DIRECTION_COUNT / 2) opposites[j] = -1;
	FOR(j, 0, DIRECTION_COUNT) {
		i32 cx = field % 9 + dxs[j];
		i32 cy = field / 9 + dys[j];
		i32 curr = cy * 9 + cx;
		while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
			cx += dxs[j];
			cy += dys[j];
			curr = cy * 9 + cx;
		}
		if (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] != BORDER) {
			i32 o = opposites[j / 2];
			if (o == -1)
				opposites[j / 2] = curr;
			else if (board[o] * board[curr] < 0 && o != to && curr != to) {	// stones hidden by added stone
				if (stackHeights[o] >= stackHeights[curr]) {
					threatenByCounts[curr]--;
					if (threatenByCounts[curr] == 0) {
						zoneOfControl[board[curr] + 3]--;
					}
				}
				if (stackHeights[o] <= stackHeights[curr]) {
					threatenByCounts[o]--;
					if (threatenByCounts[o] == 0) {
						zoneOfControl[board[o] + 3]--;
					}
				}
			}
			if (board[field] * board[curr] < 0) {
				if (stackHeights[field] >= stackHeights[curr]) {
					if (threatenByCounts[curr] == 0) {
						zoneOfControl[board[curr] + 3]++;
					}
					threatenByCounts[curr]++;
				}
				if (stackHeights[field] <= stackHeights[curr]) {
					if (threatenByCounts[field] == 0) {
						zoneOfControl[board[field] + 3]++;
					}
					threatenByCounts[field]++;
				}
			}
		}
	}
}

/// Updating ZOC after adding a stone on field to
inline __attribute__ ((always_inline))
void ZOCAddedStoneTo(i32 field)
{				// supposing stone is already on its place 
	FOR(j, 0, DIRECTION_COUNT) {
		i32 cx = field % 9 + dxs[j];
		i32 cy = field / 9 + dys[j];
		i32 curr = cy * 9 + cx;
		while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
			cx += dxs[j];
			cy += dys[j];
			curr = cy * 9 + cx;
		}
		if (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] != BORDER && board[field] * board[curr] < 0) {
			if (stackHeights[field] >= stackHeights[curr]) {
				if (threatenByCounts[curr] == 0) {
					zoneOfControl[board[curr] + 3]++;
				}
				threatenByCounts[curr]++;
			}
			if (stackHeights[field] <= stackHeights[curr]) {
				if (threatenByCounts[field] == 0) {
					zoneOfControl[board[field] + 3]++;
				}
				threatenByCounts[field]++;
			}
		}
	}
}

/// Free a move and update counters
inline __attribute__ ((always_inline))
void FreeMove(Move * move)
{
	ASSERT2(move != null, "move to free null");
	free(move);
	DBG(moveAlive--);
}

/// Free linked list of moves, but only the ones that are different from exception
/// Note: exception == null may occure
inline __attribute__ ((always_inline))
void FreeAllMoves(Move * move, Move * exception)
{
	Move *tmp;
	while (move != NULL) {
		tmp = move;
		move = move->next;
		if (tmp != exception)
			FreeMove(tmp);
	}
}

/// Free linked list of moves
inline __attribute__ ((always_inline))
void FreeAllMovesWithoutException(Move * move)
{
	Move *tmp;
	while (move != NULL) {
		tmp = move;
		move = move->next;
		FreeMove(tmp);
	}
}

/// Returns whether a given move is possible in the current position
inline __attribute__ ((always_inline))
i32 IsMovePossible(Move * move)
{				//used by AB to determine TT2 collision
	ASSERT2(move != null, "IsMovePossible: move null");
	i32 from = move->from, to = move->to;
	if (from == -1) {	//pass
		if (moveNumber == 2)
			return true;
		else
			return false;
	}
	// these exceptional cases shouldn't happen
	ASSERT2(!
		(from < 0 || from >= BOARD_ARRAY_SIZE || to < 0 || to >= BOARD_ARRAY_SIZE || board[from] == BORDER
		 || board[to] == BORDER), "IsMovePossible: bad move THIS SHOULD NOT HAPPEN; from %d to %d", from, to);
	if (SIGN(board[from]) != player)
		return false;
	if (board[to] == EMPTY)
		return false;	// empty target
	if (board[from] == EMPTY)
		return false;	// empty source  
	i32 sx = from % 9;
	i32 sy = from / 9;	// source in the x, y coordinates
	i32 tx = to % 9;
	i32 ty = to / 9;	// target in the x, y coordinates

	// source and target aren't on the same line
	if (sx != tx && sy != ty && abs(sx - tx) != abs(sy - ty))
		return false;
	i32 dx = SIGND(sx, tx), dy = SIGND(sy, ty);
	if (dx * dy < 0)
		return false;

	// ensure that all lines between source and target are empty
	i32 cx = sx, cy = sy;
	while (true) {
		cx += dx;
		cy += dy;
		if (cx == tx && cy == ty)
			break;
		i32 c = cy * 9 + cx;
		if (c < 0 || c >= BOARD_ARRAY_SIZE)
			return false;
		if (board[c] != EMPTY)
			return false;

	}
	if (board[from] * board[to] > 0) {	// stacking
		if (counts[board[to] + 3] == 1)
			return false;	// don't stack on last piece
		if (moveNumber == 2)
			return true;	//stacking
		else
			return false;
	} else if (stackHeights[from] >= stackHeights[to])
		return true;	// capturing higher stack is not possible
	return false;
}

void printZOCDebug()
{		// used for debugging Zone of Control
	FOR(i, 0, BOARD_ARRAY_SIZE) {
		printf("%d ", threatenByCounts[i]);
		if ((i + 1) % 9 == 0)
			printf("\n");
	}
	printf("\n");
	FOR(i, 0, STONE_TYPES) {
		printf("%d ", zoneOfControl[i]);
	}
	printf("\n");
}

/// When moveNumber is 1, player has to have a possibility to capture, thus this method searches for a possible capture.
/// Note: using ZOC is much more quicker
bool HasLegalMoves()
{
	ASSERT2(moveNumber == 1, "hasLegalMoves is useless when moveNumber == 2");
	ASSERT(false, "calling has legal moves");
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			if (board[i] * board[curr] > 0)
				continue;
			else if (stackHeights[i] < stackHeights[curr])
				continue;
			return true;
		}
	}
	return false;
}

/// Quickly determine whether one of players has won, ie. run out of one type of stones or has no possibility to capture
inline __attribute__ ((always_inline))
i32 IsEndOfGame()
{
	ASSERT2(value == 0, "IsEndOfGame, pos val %d", value);
	FOR(i, 0, 3) if (counts[i] == 0) {
		DPRINT2("black run out of %d", i);
		return WHITE;
	}
	FOR(i, 4, 7) if (counts[i] == 0) {
		DPRINT2("white run out of %d", i);
		return BLACK;
	}
	if (moveNumber == 1) {
		i32 index = (1 - player) * 2;
		if (zoneOfControl[index] + zoneOfControl[index + 1] + zoneOfControl[index + 2] == 0) {
			ASSERT2(!HasLegalMoves(), "sum of ZOC for %d is 0, but he has legal moves!!!", player);
			DPRINT2("no legal moves for %d", player);
			DBG2(printZOCDebug());
			return -player;

		}
		//else ASSERT2(HasLegalMoves(), "sum of ZOC for %d > 0, but he has NO legal moves!!!", player);
	}
	return false;
}

void printHighestDebug()
{		// for debugging highestStack
	FOR(i, 0, STONE_TYPES) {
		DPRINT("type %d, highest %d", i, highestStack[i]);
		FOR(j, 0, MAX_STACK_HEIGHT) {
			if (countsByHeight[i][j] > 0)
				printf("%d:%d, ", j, countsByHeight[i][j]);
		}
		printf("\n");
	}
}

/// Executes moves and updates state, ZOC, hash ...
inline __attribute__ ((always_inline))
void ExecuteMove(Move * move)
{
	DPRINT2("move to execute: from %s to %s", IndexToFieldName(move->from), IndexToFieldName(move->to));
	ASSERT2(IsMovePossible(move), "EXEC: move not possible!!");
	ASSERT2(value == 0, "EXEC: pos value %d", value);
	ASSERT2(!IsEndOfGame(), "EXEC: end of game");
	i32 from = move->from;
	i32 to = move->to;
	move->value = materialValue;
	ASSERT2(from < 0 || from != to, "exec: from %d to %d", from, to);
	if (from == -1) {	//pass
		ASSERT2(moveNumber == 2, "pass is not allowed when moveNumber == 1");
		//do nothing
		DPRINT2("pass ...");
	} else {
		// save old state
		move->oldStone = board[to];
		move->oldStackHeight = stackHeights[to];
		// update hash -- delete old pieces on fields from and to
		hash ^= HashedPositions[to][move->oldStone + 3][move->oldStackHeight];
		hash ^= HashedPositions[from][board[from] + 3][stackHeights[from]];
		// update ZOC
		ZOCRemovingStoneFrom(from, to);
		ZOCRemovingStoneTo(to);
		if (threatenByCounts[from] > 0)
			zoneOfControl[board[from] + 3]--;
		threatenByCounts[from] = 0;
		if (threatenByCounts[to] > 0)
			zoneOfControl[board[to] + 3]--;
		threatenByCounts[to] = 0;
		// updates highest stack 
		i32 hFrom = stackHeights[from];
		i32 hTo = stackHeights[to];
		i32 stoneIndexFrom = board[from] + 3;
		i32 stoneIndexTo = board[to] + 3;
		ASSERT2(stoneIndexFrom != 3, "exec stoneIndexFrom %d", stoneIndexFrom);
		ASSERT2(stoneIndexTo != 3, "exec stoneIndexTo %d", stoneIndexTo);
		countsByHeight[stoneIndexFrom][hFrom]--;
		countsByHeight[stoneIndexTo][hTo]--;
		if (hTo == highestStack[stoneIndexTo] && countsByHeight[stoneIndexTo][hTo] == 0) {
			i32 hMax = hTo - 1;
			while (countsByHeight[stoneIndexTo][hMax] == 0)
				hMax--;
			highestStack[stoneIndexTo] = hMax;
		}
		// capturing
		if (board[from] * board[move->to] < 0) {
			// recalculate value
			materialValue += player * (StackQuality(move->oldStackHeight, move->oldStone, to)
						   - StackValueByField[hFrom][from] + StackValueByField[hFrom][to]);
			stackHeights[to] = stackHeights[from];
		}
		// stacking
		else {
			// recalculate value
			materialValue += player * (-StackQuality(move->oldStackHeight, move->oldStone, to)
						   - StackQuality(hFrom, board[from], from)
						   + StackQuality(hTo + hFrom, board[from], to));
			stackHeights[to] += stackHeights[from];

		}
		// highest stacks
		i32 hNew = stackHeights[to];
		countsByHeight[stoneIndexFrom][hNew]++;
		if (hNew > highestStack[stoneIndexFrom]) {
			highestStack[stoneIndexFrom] = hNew;
		}
		counts[board[to] + 3]--;
		board[to] = board[from];
		// clear from field
		board[from] = EMPTY;
		stackHeights[from] = 0;
		ASSERT2(HashedPositions[from][EMPTY + 3][0] != 0
			&& HashedPositions[to][board[to] + 3][stackHeights[to]] != 0, "hash 0");
		// update hash -- add new stacks on fields from and to
		hash ^= HashedPositions[from][EMPTY + 3][0];
		hash ^= HashedPositions[to][board[to] + 3][stackHeights[to]];
		//update ZOC
		ZOCAddedStoneTo(to);
		stoneSum--;
	}
	// this is also done for pass move
	history[turnNumber * 2 + moveNumber] = move;	// save executed move to history
	// update player, move number and moveNumber
	moveNumber++;
	if (moveNumber == 3) {	// || (turnNumber == 1 && moveNumber == 2)) { speed up -- not used
		moveNumber = 1;
		hash ^= 1;
		player = -player;
		turnNumber++;
	}
	// is end of game?
	i32 winner = 0;
	if ((winner = IsEndOfGame()) != 0) {
		value = WIN * winner;
	} else
		value = 0;
}

/// Reverts last move in history, updates states, ZOC, hash, material value ...
inline __attribute__ ((always_inline))
void RevertLastMove()
{
	ASSERT2(turnNumber * 2 + moveNumber - 1 >= 0, "revert: turnNumber * 2 + moveNumber - 1 == %d",
		turnNumber * 2 + moveNumber - 1);
	Move *move = history[turnNumber * 2 + moveNumber - 1];	// retrieve move from history
	ASSERT2(move != null, "RevertLastMove: move null on %d", turnNumber * 2 + moveNumber - 1);
	DPRINT2("move to revert: from %s to %s; turnNumber * 2 + moveNumber - 1 == %d", IndexToFieldName(move->from),
		IndexToFieldName(move->to), turnNumber * 2 + moveNumber - 1);
	DPRINT2("reverting start");
	if (move->from != -1) {	// move is not pass
		i32 from = move->from;
		i32 to = move->to;
		ASSERT2(from != to, "revert: from == to == %d", from);
		// update hash -- delete old stacks from it
		hash ^= HashedPositions[from][EMPTY + 3][0];
		hash ^= HashedPositions[to][board[to] + 3][stackHeights[to]];
		//update ZOC
		ZOCRemovingStoneTo(to);
		ASSERT2(threatenByCounts[to] == 0, "revert: threatenByCounts[to]: %d", threatenByCounts[to]);
		if (threatenByCounts[to] > 0)
			zoneOfControl[board[to] + 3]--;
		threatenByCounts[to] = 0;
		i32 oldStoneIndex = move->oldStone + 3;
		ASSERT2(oldStoneIndex != 3, "revert oldStoneIndex %d, from %d, to %d", oldStoneIndex, from, to);
		// counts and highest stacks
		counts[oldStoneIndex]++;
		i32 hOld = stackHeights[to];
		i32 stoneIndexTo = board[to] + 3;
		ASSERT2(stoneIndexTo != 3, "revert stoneIndexTo %d, from %d, to %d", stoneIndexTo, from, to);
		countsByHeight[stoneIndexTo][hOld]--;
		if (hOld == highestStack[stoneIndexTo] && countsByHeight[stoneIndexTo][hOld] == 0) {
			i32 hMax = hOld - 1;
			while (countsByHeight[stoneIndexTo][hMax] == 0)
				hMax--;
			highestStack[stoneIndexTo] = hMax;
		}
		// revert stack height
		bool capt = move->oldStone * board[to] < 0;
		if (capt) {	// capture
			stackHeights[from] = hOld;
		} else {	// stacking   
			stackHeights[from] = hOld - move->oldStackHeight;
		}
		// revert highest stacks
		i32 hNew = move->oldStackHeight;
		countsByHeight[oldStoneIndex][hNew]++;
		if (hNew > highestStack[oldStoneIndex]) {
			highestStack[oldStoneIndex] = hNew;
		}
		i32 hFrom = stackHeights[from];
		countsByHeight[stoneIndexTo][hFrom]++;
		if (hFrom > highestStack[stoneIndexTo]) {
			highestStack[stoneIndexTo] = hFrom;
		}
		// update board
		board[from] = board[to];
		board[to] = move->oldStone;
		stackHeights[to] = move->oldStackHeight;

		ASSERT2(HashedPositions[to][move->oldStone + 3][move->oldStackHeight] != 0
			&& HashedPositions[from][board[from] + 3][stackHeights[from]] != 0, "hash 0");
		// update hash -- add new stacks to it
		hash ^= HashedPositions[to][move->oldStone + 3][move->oldStackHeight];
		hash ^= HashedPositions[from][board[from] + 3][stackHeights[from]];
		// update ZOC
		ZOCAddedStoneFrom(from, to);
		ZOCAddedStoneTo(to);
		if (capt) {
			DPRINT2("threatenByCounts[to] %d threatenByCounts[from] %d", threatenByCounts[to],
				threatenByCounts[from]);
			threatenByCounts[to]--;	//zapocteno dvakrat
			if (stackHeights[from] == stackHeights[to])
				threatenByCounts[from]--;	//zapocteno dvakrat
		}
		// revert material value
		materialValue = move->value;
		stoneSum++;
	}
	// revert player 
	if (moveNumber == 1) {
		hash ^= 1;
		turnNumber--;
		player = -player;
		moveNumber = 2;
	} else
		moveNumber = 1;
	value = 0;
	ASSERT2(!IsEndOfGame(), "after REVERT: is end of game");
}

/// Return index (in board array) of a field determined by its name, ie. A1 or E6
/// It's slow
i32 FieldNameToIndex(const char *field)
{
	if (field == null)
		return INVALID_INPUT;
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (toupper((i32) field[0]) == toupper((i32) FieldNames[i][0]) && field[1] == (i32) FieldNames[i][1])
			return i;
	}
	return INVALID_INPUT;
}

/// Return name of field on given index in the board array, ie. A1 or E6
const char *IndexToFieldName(i32 index)
{
	if (index < 0 || index >= BOARD_ARRAY_SIZE)
		return "-";
	return FieldNames[index];
}

/// Generate all moves and return linked list of them (moveNumber could be 1 or 2)
inline void GenerateAllMoves(Move ** moves)
{
	Move *m = null;
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			bool capt = board[i] * board[curr] < 0;
			if (!capt) {
				if (moveNumber == 1)
					continue;
				if (counts[board[curr] + 3] <= 1)
					continue;	//dont stack on the last piece, better is to pass
			} else if (stackHeights[i] < stackHeights[curr])
				continue;
			Move *n = (Move *) malloc(sizeof(Move));
			DBG(moveAlive++);
			n->from = i;
			n->to = curr;
			n->next = m;
			ASSERT2(IsMovePossible(n), "Gen all moves: move not possible");
			m = n;
		}
	}
	if (moveNumber == 2) {
		Move *n = (Move *) malloc(sizeof(Move));
		DBG(moveAlive++);
		n->from = -1;	//n->to = 
		n->next = m;
		m = n;
	}
	*moves = m;
}

Move *moveArrayToSort[MAX_POSSIBILITIES]; // array for sorting moves

/// Helping method for qsort from stdlib.h
i32 compareMoves(const void *a, const void *b)
{
	return (*(Move **) a)->value - (*(Move **) b)->value;
}

/// Generate all moves, sort them heuristically and return linked list of them (moveNumber could be 1 or 2)
inline void GenerateAllMovesSorted(Move ** moves)
{
	ASSERT2(abs(value) < WIN, " -- generating moves in winning position!!, moveNumber %d", moveNumber);
	DPRINT2("gen moves sorted, pl %d", player);
	i32 count = 0;		//captures = 0, 
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		//DPRINT2("field %d, pl %d", i, player);
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			bool capt = board[i] * board[curr] < 0;
			if (!capt) {
				if (moveNumber == 1)
					continue;
				if (counts[board[curr] + 3] <= 1)
					continue;	//dont stack on last piece
			} else if (stackHeights[i] < stackHeights[curr])
				continue;
			Move *n = (Move *) malloc(sizeof(Move));
			DBG(moveAlive++);
			n->from = i;
			n->to = curr;
			if (capt) {
				n->value = SORT_CAPTURE_COUNT_MULT * counts[board[curr] + 3] - CapturingStackHeightAdvantage[stackHeights[curr]];
			} else {
				n->value = SORT_STACK_BONUS + SORT_STACK_COUNT_MULT * counts[board[i] + 3];
			}
			ASSERT2(IsMovePossible(n), "Gen all moves sorted: move not possible");
			moveArrayToSort[count++] = n;
		}
	}
	if (count == 0) {
		Move *n = (Move *) malloc(sizeof(Move));
		DBG(moveAlive++);
		n->from = n->to = -1;
		n->next = null;
		*moves = n;
		return;
	}
	qsort(moveArrayToSort, count, sizeof(Move *), compareMoves);
	for (i32 i = 1; i < count; i++) {
		moveArrayToSort[i - 1]->next = moveArrayToSort[i];
	}
	if (moveNumber == 2) {	//pass move
		Move *n = (Move *) malloc(sizeof(Move));
		DBG(moveAlive++);
		n->from = n->to = -1;
		n->next = null;
		moveArrayToSort[count - 1]->next = n;
	} else
		moveArrayToSort[count - 1]->next = null;
	*moves = moveArrayToSort[0];
}

/// When the position is in moveNumber 1, generate all moves, sort them by heuristics and return a linked list of them
inline void GenerateAllMovesSortedMove1(Move ** moves)
{
	ASSERT2(abs(value) < WIN, " -- generating moves in winning position!!, moveNumber %d", moveNumber);
	DPRINT2("gen moves sorted, pl %d", player);
	ASSERT2(moveNumber == 1, "gen all moves sorted moveNumber NOT 1, but %d", moveNumber);
	i32 count = 0;
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			bool capt = board[i] * board[curr] < 0;
			if (!capt) {
				continue;
			} else if (stackHeights[i] < stackHeights[curr])
				continue;
			Move *n = (Move *) malloc(sizeof(Move));
			DBG(moveAlive++);
			n->from = i;
			n->to = curr;
			n->value =
			    SORT_CAPTURE_COUNT_MULT * counts[board[curr] + 3] -
			    CapturingStackHeightAdvantage[stackHeights[curr]]
			    - (SORT_HISTORY_PRUNES_MULT * historyPruneMoves[i][curr]);
			ASSERT2(IsMovePossible(n), "Gen all moves sorted: move not possible");
			moveArrayToSort[count++] = n;
		}
	}
	DPRINT2("sorting, pl %d", player);
	ASSERT2(count > 0, "gen all moves sorted moveNumber 1, count %d", count);
	qsort(moveArrayToSort, count, sizeof(Move *), compareMoves);
	for (i32 i = 1; i < count; i++) {
		moveArrayToSort[i - 1]->next = moveArrayToSort[i];
	}
	moveArrayToSort[count - 1]->next = null;
	*moves = moveArrayToSort[0];
}

/// When the position is in moveNumber 2, generate all moves, sort them by heuristics and return a linked list of them
inline void GenerateAllMovesSortedMove2(Move ** moves)
{
	ASSERT2(abs(value) < WIN, " -- generating moves in winning position!!, moveNumber %d", moveNumber);
	DPRINT2("gen moves sorted, pl %d", player);
	ASSERT(moveNumber == 2, "gen all moves sorted moveNumber NOT 2, but %d", moveNumber);
	i32 count = 0;
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			bool capt = board[i] * board[curr] < 0;
			if (!capt) {
				if (counts[board[curr] + 3] <= 1)
					continue;	//dont stack on last piece
			} else if (stackHeights[i] < stackHeights[curr])
				continue;
			Move *n = (Move *) malloc(sizeof(Move));
			DBG(moveAlive++);
			n->from = i;
			n->to = curr;
			if (capt) {
				n->value = SORT_CAPTURE_COUNT_MULT * counts[board[curr] + 3] - CapturingStackHeightAdvantage[stackHeights[curr]];
			} else {
				n->value = SORT_STACK_BONUS + SORT_STACK_COUNT_MULT * counts[board[i] + 3];
			}
			ASSERT2(IsMovePossible(n), "Gen all moves sorted: move not possible");
			moveArrayToSort[count++] = n;
		}
	}
	DPRINT2("sorting, pl %d", player);
	if (count == 0) {
		Move *n = (Move *) malloc(sizeof(Move));
		DBG(moveAlive++);
		n->from = n->to = -1;
		n->next = null;
		*moves = n;
		return;
	}
	qsort(moveArrayToSort, count, sizeof(Move *), compareMoves);
	for (i32 i = 1; i < count; i++) {
		moveArrayToSort[i - 1]->next = moveArrayToSort[i];
	}
	//pass move
	Move *n = (Move *) malloc(sizeof(Move));
	DBG(moveAlive++);
	n->from = n->to = -1;
	n->next = null;
	moveArrayToSort[count - 1]->next = n;	// count > 0 is ensured
	*moves = moveArrayToSort[0];
}

/// Generate all moves, sort them by heuristics and return only maxMoves best
inline void GenerateBestMovesSorted(Move ** moves, i32 maxMoves)
{
	i32 count = 0;
	for (i32 i = 0; i < BOARD_ARRAY_SIZE; i++) {
		if (board[i] == BORDER || board[i] == EMPTY || board[i] * player < 0)
			continue;
		for (i32 j = 0; j < DIRECTION_COUNT; j++) {
			i32 cx = i % 9 + dxs[j];
			i32 cy = i / 9 + dys[j];
			i32 curr = cy * 9 + cx;
			while (curr >= 0 && curr < BOARD_ARRAY_SIZE && board[curr] == EMPTY) {
				cx += dxs[j];
				cy += dys[j];
				curr = cy * 9 + cx;
			}
			if (curr < 0 || curr >= BOARD_ARRAY_SIZE || board[curr] == BORDER)
				continue;
			bool capt = board[i] * board[curr] < 0;
			if (!capt) {
				if (moveNumber == 1)
					continue;
				if (counts[board[curr] + 3] <= 1)
					continue;	//dont stack on last piece
			} else if (stackHeights[i] < stackHeights[curr])
				continue;
			Move *n = (Move *) malloc(sizeof(Move));
			DBG(moveAlive++);
			n->from = i;
			n->to = curr;
			if (capt) {
				n->value = 10 * counts[board[curr] + 3] - CapturingStackHeightAdvantage[stackHeights[curr]];
			} else {
				n->value = 19 + 3 * counts[board[i] + 3];
			}
			ASSERT2(IsMovePossible(n), "Gen some moves: move not possible");
			moveArrayToSort[count++] = n;
		}
	}
	if (count == 0 && moveNumber == 1) {
		*moves = null;
		return;
	}
	qsort(moveArrayToSort, count, sizeof(Move *), compareMoves);
	i32 min = MIN(count, maxMoves);
	for (i32 i = 1; i < min; i++) {
		moveArrayToSort[i - 1]->next = moveArrayToSort[i];
	}
	for (i32 i = min; i < count; i++) {
		FreeMove(moveArrayToSort[i]);
	}
	if (moveNumber == 2 && count < maxMoves) {	//pass move
		Move *n = (Move *) malloc(sizeof(Move));
		DBG(moveAlive++);
		n->from = n->to = -1;
		n->next = null;
		if (min > 0)
			moveArrayToSort[min - 1]->next = n; // put pass move to the end of moves list (it's not good move mostly)
		else {
			*moves = n;
			return;
		}
	} else
		moveArrayToSort[min - 1]->next = null;
	*moves = moveArrayToSort[0];
}

/// Allocates memory for new move and copies move in the parameter into its variables
inline __attribute__ ((always_inline))
Move *CloneMove(Move * move)
{
	ASSERT2(move != null, "move to clone null");
	Move *m = MALLOC(Move);
	m->from = move->from;
	m->to = move->to;
	m->next = move->next;
	m->oldStackHeight = move->oldStackHeight;
	m->oldStone = move->oldStone;
	m->value = move->value;
	return m;
}
