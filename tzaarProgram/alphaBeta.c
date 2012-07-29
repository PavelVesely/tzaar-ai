/*
 * The module alphaBeta contains functions that implement the Alpha-beta
 * pruning algorithm with its enhancements, functions for working with
 * the Transposition Table (TT) and static evaluation functions. 
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "alphaBeta.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

/// Statical evaluation function
/// Should be as quick as possible
inline __attribute__ ((always_inline))
i32 StaticValue()
{
	i32 val = 0; // static value

	// bonus for a stack of a type that is higher than any opponents stack
	i32 maxW = MAX(MAX(highestStack[4], highestStack[5]), highestStack[6]);
	i32 maxB = MAX(MAX(highestStack[0], highestStack[1]), highestStack[2]);
	bool secW = true, secB = true;
	FOR(i, 0, 3) {
		if (highestStack[i] > maxW) {
			val -= SECURE_TYPE;
		} else
			secB = false;
	}
	FOR(i, 4, 7) {
		if (highestStack[i] > maxB) {
			val += SECURE_TYPE;
		} else
			secW = false;
	}
	if (secW)
		val += FULL_SECURE;
	if (secB)
		val -= FULL_SECURE;

	// bonus for stack of height >= 2 of every stone type
	i32 minW = MIN(MIN(highestStack[4], highestStack[5]), highestStack[6]);
	i32 minB = MIN(MIN(highestStack[0], highestStack[1]), highestStack[2]);
	if (minW >= 2)
		val += STACK_OF_ALL_TYPES_BONUS;
	if (minB >= 2)
		val -= STACK_OF_ALL_TYPES_BONUS;

	// a small number of stacks that can be captured is disadvantage for opponent
	i32 ZOCw = zoneOfControl[4] + zoneOfControl[5] + zoneOfControl[6];
	if (ZOCw <= ZOC_CRITICAL_SUM)
		val += ZOC_CRITICAL_BONUS; // black has nearly nothing that can be captured, thus it's advantage for white
	i32 ZOCb = zoneOfControl[0] + zoneOfControl[1] + zoneOfControl[2];
	if (ZOCb <= ZOC_CRITICAL_SUM)
		val -= ZOC_CRITICAL_BONUS; // the same for black

	// zone of control
	FOR(i, 0, 3) {
		if (counts[i] <= 2 && zoneOfControl[i] == counts[i])
			val += THREAT_HEIGHT * (3 - counts[i]) * (player == WHITE ? THREAT_HEIGHT_MULT : 1);
		val += (zoneOfControl[i] * (InitialStoneCounts[i] - counts[i]) * ZONE_OF_CONTROL_HEIGHT) / InitialStoneCounts[i];
	}
	FOR(i, 4, 7) {
		if (counts[i] <= 2 && zoneOfControl[i] == counts[i])
			val -= THREAT_HEIGHT * (3 - counts[i]) * (player == BLACK ? THREAT_HEIGHT_MULT : 1);
		val -= (zoneOfControl[i] * (InitialStoneCounts[i] - counts[i]) * ZONE_OF_CONTROL_HEIGHT) / InitialStoneCounts[i];
	}
	return val;
}

/// Statical evaluation function for beginner's AI
inline __attribute__ ((always_inline))
i32 StaticValueBeginner()
{
	return 0;
}

inline __attribute__ ((always_inline))
TTEntry *LookupPositionInTT()
{
	i32 index = hash % TTSIZE;
	if (TranspositionTable[index] != null && TranspositionTable[index]->hash == hash)
		return TranspositionTable[index];
	else if (TranspositionTable[index + TTSIZE] != null && TranspositionTable[index + TTSIZE]->hash == hash)
		return TranspositionTable[index + TTSIZE];
	else
		return null;
}

inline __attribute__ ((always_inline))
bool CompareTTEntries(TTEntry * a, TTEntry * b)
{
	if (a->valueType != EXACT_VALUE) {
		if (b->valueType != EXACT_VALUE)
			return a->searchedNodes < b->searchedNodes;
		else
			return true;
	} else if (b->valueType != EXACT_VALUE)
		return false;
	return a->searchedNodes < b->searchedNodes;
}

inline __attribute__ ((always_inline))
void FreeTTEntry(TTEntry * entry)
{
	ASSERT2(entry != null, "entry to free not null");
	if (entry->bestMove1 != null)
		FreeMove(entry->bestMove1);
	if (entry->bestMove2 != null)
		FreeMove(entry->bestMove2);
	DBG(entryAlive--);
	free(entry);
}

inline __attribute__ ((always_inline))
void AddPositionToTT(i32 value, i32 type, i32 searchDepth, u32 searchedNodes, Move * bestMove1, Move * bestMove2)
{
	i32 index = hash % TTSIZE;
	if (TranspositionTable[index] != null && TranspositionTable[index]->hash == hash) {
		if (searchDepth > TranspositionTable[index]->searchDepth || searchedNodes >= TranspositionTable[index]->searchedNodes) {
			TranspositionTable[index]->searchedNodes = searchedNodes;
			TranspositionTable[index]->searchDepth = searchDepth;
			Move *tmp = TranspositionTable[index]->bestMove1;
			if (tmp != null)
				FreeMove(tmp);
			tmp = TranspositionTable[index]->bestMove2;
			if (tmp != null)
				FreeMove(tmp);
			TranspositionTable[index]->bestMove1 = bestMove1;
			TranspositionTable[index]->bestMove2 = bestMove2;
			TranspositionTable[index]->value = value;
			TranspositionTable[index]->valueType = type;
		}
		return;
	}
	if (TranspositionTable[index + TTSIZE] != null && TranspositionTable[index + TTSIZE]->hash == hash) {
		if (searchDepth > TranspositionTable[index + TTSIZE]->searchDepth
		    || searchedNodes >= TranspositionTable[index + TTSIZE]->searchedNodes) {
			TranspositionTable[index + TTSIZE]->searchedNodes = searchedNodes;
			TranspositionTable[index + TTSIZE]->searchDepth = searchDepth;
			TranspositionTable[index + TTSIZE]->bestMove1 = bestMove1;
			TranspositionTable[index + TTSIZE]->bestMove2 = bestMove2;
			TranspositionTable[index + TTSIZE]->value = value;
			TranspositionTable[index + TTSIZE]->valueType = type;
		}
		return;
	}
	TTEntry *entry = MALLOC(TTEntry);
	DBG(entryAlive++);
	entry->bestMove1 = bestMove1;
	entry->bestMove2 = bestMove2;
	entry->value = value;
	entry->hash = hash;
	entry->searchDepth = searchDepth;
	entry->searchedNodes = searchedNodes;
	entry->valueType = type;
	TTEntry *toSave = entry;
	if (TranspositionTable[index] == null || CompareTTEntries(TranspositionTable[index], entry)) {
		toSave = TranspositionTable[index];
		TranspositionTable[index] = entry;
	}
	if (toSave != null) {	// it holds: TranspositionTable[index] > TranspositionTable[index + TTSIZE]
		TTEntry *old = TranspositionTable[index + TTSIZE];
		TranspositionTable[index + TTSIZE] = toSave;
		if (old != null) {
			FreeTTEntry(old);
			DBG(ttKick++);
		}
	}
}

/// AlphaBeta without enhancements
i32 AlphaBeta(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("ab - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
	}
	bool pruned = false;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMoves(&move);
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			break;
		}
		else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					break;
				}
			}
		} else {
			GenerateAllMoves(&move2);
			start2 = move2;
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				val = -AlphaBeta(depth - 2, -beta, -alpha);
				RevertLastMove();
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						break;
					}
					ttType = EXACT_VALUE;
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
			}
		}
		RevertLastMove();
		if (pruned) {
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with TT and Principal Variation Move
i32 AlphaBetaPV(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("abPVMO - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		ASSERT2(saved->bestMove1 != null, "saved->bestMove1 == null");
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValue());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DPRINT2("pruned by pv move, d = 1, alpha %d, beta %d", alpha, beta);
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						return max;
					}
				}
			}
			RevertLastMove();
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			max = -AlphaBetaPV(depth - 2, -beta, -alpha);
			RevertLastMove();
			RevertLastMove();
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
			
		}
	}
	bool pruned = false;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMoves(&move);
	
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		DPRINT2("exec move 1, depth %d", depth);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			break;
		}
		else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					break;
				}
			}
		} else {
			GenerateAllMoves(&move2);
			start2 = move2;
			
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				val = -AlphaBetaPV(depth - 2, -beta, -alpha);
				RevertLastMove();
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						
						break;
					}
					ttType = EXACT_VALUE;
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			DPRINT2("pruned");
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with TT and Principal Variation Move and Move Ordering
i32 AlphaBetaPVMO(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("abPVMO - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValue());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DPRINT2("pruned by pv move, d = 1, alpha %d, beta %d", alpha, beta);
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						return max;
					}
				}
			}
			RevertLastMove();
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			max = -AlphaBetaPVMO(depth - 2, -beta, -alpha);
			RevertLastMove();
			RevertLastMove();
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
			
		}
	}
	bool pruned = false;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMovesSortedMove1(&move);
	
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			break;
		}
		else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					break;
				}
			}
		} else {
			GenerateAllMovesSortedMove2(&move2);
			
			start2 = move2;
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				val = -AlphaBetaPVMO(depth - 2, -beta, -alpha);
				RevertLastMove();
				DPRINT2("rev move 2");
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
							
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						break;
					}
					ttType = EXACT_VALUE;
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			DPRINT2("pruned");
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with Move Ordering
i32 AlphaBetaMO(i32 depth, i32 alpha, i32 beta, Move ** m1, Move ** m2)
{
	DPRINT2("abPVMO - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	bool pruned = false;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMoves(&move);
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);					
				}
				
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					break;
				}
			}
		} else {
			GenerateAllMoves(&move2);
			start2 = move2;
			
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				Move *tm1 = null, *tm2 = null;
				val = -AlphaBetaMO(depth - 2, -beta, -alpha, &tm1, &tm2);
				RevertLastMove();
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
							
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						break;
					}
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			DPRINT2("pruned");
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;//DPRINT2("done del best 1");
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	*m1 = best1;
	*m2 = best2;
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// random AlphaBeta with TT and Principal Variation Move and Move Ordering
i32 AlphaBetaPVMORandom(i32 depth, i32 randomMargin)
{
	DPRINT2("abPVMO RANDMO -> alpha = -beta = -WIN; depth %d, searched %d, pl %d", depth, searchedNodes, player);
	ASSERT2(depth == currDepth, "alfa-beta RANDOM PVMO not in top level of search");
	ASSERT2(depth > 1, "random alfa-beta should have depth > 1");
	searchedNodes++;
	i32 moveCount = 0, initSearchedNodes = searchedNodes;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	Move *move, *move2, *start1, *start2;
	FullMovesList *allMoves = null;
	GenerateAllMovesSortedMove1(&move);
	start1 = move;
	while (move != NULL) {
		ExecuteMove(move);
		GenerateAllMovesSortedMove2(&move2);
		start2 = move2;
		while (move2 != NULL) {
			ExecuteMove(move2);
			moveCount++;
			val = -AlphaBetaPVMO(depth - 2, -WIN, -max + randomMargin + 1);
			if (val >= max - randomMargin) {
				FullMovesList *nMove = MALLOC(FullMovesList);
				nMove->move1 = move;
				nMove->move2 = move2;
				nMove->value = val;
				nMove->next = allMoves;
				allMoves = nMove;
			}
			RevertLastMove();
			if (val > max) {
				max = val;
			}
			move2 = move2->next;
		}
		RevertLastMove();
		move = move->next;
	}
	DPRINT2("random selecting started");
	i32 goodEnoughMoves = 0;
	FullMovesList *curr = allMoves, *selected = null;
	while (curr != null) {
		if (curr->value >= max - randomMargin)
			goodEnoughMoves++;
		curr = curr->next;
	}
	ASSERT2(goodEnoughMoves > 0, "there are no moves for random choice!");
	srand(time(null));
	i32 selectedMove = rand() % goodEnoughMoves;
	DPRINT("found %d good moves, selected random move: %d", goodEnoughMoves, selectedMove);
	curr = allMoves;
	i32 i = 0;
	while (curr != null) {
		if (curr->value >= max - randomMargin) {
			if (i == selectedMove) {
				selected = curr;
			} else {
				FreeMove(curr->move2);
				if ((curr->next == null || curr->next->move1 != curr->move1)
				    && (selected == null || curr->move1 != selected->move1))
					FreeMove(curr->move1);
				
			}
			i++;
		} else {
			FreeMove(curr->move2);
			if ((curr->next == null || curr->next->move1 != curr->move1)
			    && (selected == null || curr->move1 != selected->move1))
				FreeMove(curr->move1);
		}
		curr = curr->next;
	}
	DPRINT2("move count on top level: %d", moveCount);
	ASSERT2(selected != null, "random select == null");
	DPRINT("max value: %d, selected move value: %d", max, selected->value);
	AddPositionToTT(max, EXACT_VALUE, depth, searchedNodes - initSearchedNodes, selected->move1, selected->move2);	//because of retrieving in GetBestMove; it's called only once
	return max;
}

/// AlphaBeta with TT and Principal Variation Move and Move Ordering and NegaScout
i32 AlphaBetaPVMONegascout(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("abPVMO negascout - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		ASSERT2(saved->bestMove1 != null, "saved->bestMove1 == null");
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DPRINT2("pruned by TT values search depth %d, depth %d", saved->searchDepth, depth);
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValue());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						return max;
					}
				}
			}
			RevertLastMove();
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			max = -AlphaBetaPVMONegascout(depth - 2, -beta, -alpha);
			RevertLastMove();
			RevertLastMove();
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
		}
	}
	bool pruned = false;
	i32 beta2 = beta;	// negascout
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMovesSortedMove1(&move);
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
					
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			break;
		} else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					break;
				}
			}
		} else {
			GenerateAllMovesSortedMove2(&move2);
			
			start2 = move2;
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				if (depth > 2 && beta2 < beta) {	// negascount
					val = -AlphaBetaPVMONegascout(depth - 2, -beta2, -alpha);
					if (val > alpha && val < beta && beta != beta2)
						val = -AlphaBetaPVMONegascout(depth - 2, -beta, -alpha);
				} else
					val = -AlphaBetaPVMONegascout(depth - 2, -beta, -alpha);
				RevertLastMove();
				DPRINT2("rev move 2");
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						break;
					}
					ttType = EXACT_VALUE;
				}
				beta2 = alpha + 1;	//negascout
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			DPRINT2("pruned");
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with TT and Principal Variation Move and Move Ordering and History Heurstic
i32 AlphaBetaPVMOHistory(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("abPVMO history - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		ASSERT2(saved->bestMove1 != null, "saved->bestMove1 == null");
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DPRINT2("pruned by TT values search depth %d, depth %d", saved->searchDepth, depth);
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValue());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						historyPruneMoves[pv1->from][pv1->to] += 1 << depth;
						return max;
					}
				}
			}
			RevertLastMove();
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			max = -AlphaBetaPVMOHistory(depth - 2, -beta, -alpha);
			RevertLastMove();
			RevertLastMove();
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					historyPruneMoves[pv1->from][pv1->to] += 1 << depth;
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
			
		}
	}
	bool pruned = false;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMovesSortedMove1(&move);
	
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		DPRINT2("exec move 1, depth %d", depth);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			historyPruneMoves[best1->from][best1->to] += 1 << depth;
			
			break;
		}
		else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					historyPruneMoves[best1->from][best1->to] += 1 << depth;
					
					break;
				}
			}
		} else {
			GenerateAllMovesSortedMove2(&move2);
			
			start2 = move2;
			while (move2 != NULL) {
				DPRINT2("start exec move 2");
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				DPRINT2("exec move 2, depth %d", depth);
				moveCount++;
				val = -AlphaBetaPVMOHistory(depth - 2, -beta, -alpha);
				RevertLastMove();
				DPRINT2("rev move 2");
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						
						historyPruneMoves[best1->from][best1->to] += 1 << depth;
						
						break;
					}
					ttType = EXACT_VALUE;
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with TT and Principal Variation Move and Move Ordering and History Heurstic and NegaScout
i32 AlphaBetaPVMOHistoryNegascout(i32 depth, i32 alpha, i32 beta)
{
	DPRINT2("abPVMO negascout & history - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta,
		searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		i32 v = materialValue + StaticValue();
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	//i32 beta2 = -WIN-2;//negascout
	if (saved != null) {
		ASSERT2(saved->bestMove1 != null, "saved->bestMove1 == null");
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DPRINT2("pruned by TT values search depth %d, depth %d", saved->searchDepth, depth);
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValue());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DPRINT2("pruned by pv move, d = 1, alpha %d, beta %d", alpha, beta);
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						historyPruneMoves[pv1->from][pv1->to] += 1 << depth;
						return max;
					}
				}
			}
			RevertLastMove();
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			max = -AlphaBetaPVMOHistoryNegascout(depth - 2, -beta, -alpha);
			RevertLastMove();
			RevertLastMove();
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					historyPruneMoves[pv1->from][pv1->to] += 1 << depth;
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
		}
	}
	bool pruned = false;
	i32 beta2 = beta;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMovesSortedMove1(&move);
	
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		DPRINT2("exec move 1, depth %d", depth);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			historyPruneMoves[best1->from][best1->to] += 1 << depth;
			break;
		} else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValue());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					historyPruneMoves[best1->from][best1->to] += 1 << depth;
					
					break;
				}
			}
		} else {
			GenerateAllMovesSortedMove2(&move2);
			
			start2 = move2;
			
			while (move2 != NULL) {
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				moveCount++;
				if (depth > 2 && beta2 < beta) {	// negascount
					val = -AlphaBetaPVMOHistoryNegascout(depth - 2, -beta2, -alpha);
					if (val > alpha && val < beta)
						val = -AlphaBetaPVMOHistoryNegascout(depth - 2, -beta, -alpha);	// NegaScout
				} else
					val = -AlphaBetaPVMOHistoryNegascout(depth - 2, -beta, -alpha);
				RevertLastMove();
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						DPRINT2("pruned, alpha %d, beta %d", alpha, beta);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						historyPruneMoves[best1->from][best1->to] += 1 << depth;
						
						break;
					}
					ttType = EXACT_VALUE;
				}
				beta2 = alpha + 1;	//negascout
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
				
			}
		}
		RevertLastMove();
		if (pruned) {
			DPRINT2("pruned");
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// AlphaBeta with TT and Principal Variation Move and Move Ordering and beginner evaluation function
i32 AlphaBetaPVMOBeginner(i32 depth, i32 alpha, i32 beta)
{				//the only difference from AlphaBetaPVMO is calling StaticValueBeginner instead of StaticValue
	DPRINT2("abPVMO beginner - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	ASSERT2(depth >= 0, "depth < 0");
	ASSERT2(moveNumber == 1, "AB starting: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	searchedNodes++;
	if (abs(value) == WIN) {
		DPRINT2("found win %d, pl %d, depth %d", value, player, depth);
		return player * value;
	}
	ASSERT2(!IsEndOfGame(), "AB end of game, val %d, depth %d", value, depth);
	if (depth == 0) {
		DPRINT2("depth 0, val %d", player * (materialValue + StaticValueBeginner()));
		i32 v = materialValue + StaticValueBeginner();
		ASSERT2(abs(v) < WIN / 10,
			"moc velka materialValue + StaticValueBeginner(): %d; z toho material value %d", v,
			materialValue);
		return player * v;
	}
	i32 initSearchedNodes = searchedNodes;
	i32 ttType = LOWER_BOUND;
	i32 moveCount = 0;
	Move *best1 = null, *best2 = null;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	TTEntry *saved = LookupPositionInTT();
	if (saved != null) {
		ASSERT2(saved->bestMove1 != null, "saved->bestMove1 == null");
		if ((saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))
		    || (saved->bestMove1 != null && !IsMovePossible(saved->bestMove1))) {
			saved = null;
			DPRINT("HashCollision");
			DBG(ttCollision++);
		}
	}
	if (saved != null) {
		DBG(ttFound++);
		if (saved->searchDepth >= depth) {
			DBG(ttHit++);
			if (saved->valueType == EXACT_VALUE) {
				ASSERT2(moveNumber == 1, "AB ret 1: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			} else if (saved->valueType == LOWER_BOUND && saved->value > alpha) {	//update lowerbound if needed
				alpha = saved->value;
			} else if (saved->valueType == UPPER_BOUND && saved->value < beta) {	//update upperbound if needed
				beta = saved->value;
			}
			if (alpha >= beta) {
				DPRINT2("pruned by TT values search depth %d, depth %d", saved->searchDepth, depth);
				DBG(prunedCount++);
				ASSERT2(moveNumber == 1, "AB ret 2: bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
				return saved->value;
			}
		}
		Move *pv1 = saved->bestMove1;
		Move *pv2 = saved->bestMove2;
		if (depth == 1 || pv2 == null) {
			DPRINT2("pv move for depth == 1");
			ExecuteMove(pv1);
			i32 plVal = player * value;
			if (depth == 1 || plVal == WIN) {
				DPRINT2("pv move executed, d = 1");
				if (plVal == WIN)
					max = plVal;
				else
					max = player * (materialValue + StaticValueBeginner());
				best1 = CloneMove(pv1);
				if (max > alpha) {
					alpha = max;
					if (alpha >= beta) {
						RevertLastMove();
						DPRINT2("pruned by pv move, d = 1, alpha %d, beta %d", alpha, beta);
						DBG(prunedCount++);
						ASSERT2(moveNumber == 1, "AB ret 3: bad moveNumber turnNumber %d, moveNumber %d",
							turnNumber, moveNumber);
						return max;
					}
				}
			}
			RevertLastMove();
			DPRINT2("pv move done");
		} else if (pv2 != null) {
			pv1 = CloneMove(pv1);	//because of TT kicks
			pv2 = CloneMove(pv2);
			ExecuteMove(pv1);
			ExecuteMove(pv2);
			DPRINT2("starting PV moves");
			max = -AlphaBetaPVMOBeginner(depth - 2, -beta, -alpha);
			DPRINT2("done PV moves");
			RevertLastMove();
			RevertLastMove();
			DPRINT2("best are pv moves");
			best1 = pv1;
			best2 = pv2;
			if (max > alpha) {
				alpha = max;
				if (alpha >= beta) {
					DBG(prunedCount++);
					ASSERT2(moveNumber == 1, "AB ret 4: bad moveNumber turnNumber %d, moveNumber %d", turnNumber,
						moveNumber);
					return max;	//no need to save anything to TT
				}
				ttType = EXACT_VALUE;
			}
		}
	}
	bool pruned = false;;
	Move *tmp, *move, *move2, *start1, *start2;
	GenerateAllMovesSortedMove1(&move);
	start1 = move;
	while (move != NULL) {
		ASSERT2(IsMovePossible(move), "AB: move 1 not possible");
		ExecuteMove(move);
		DPRINT2("exec move 1, depth %d", depth);
		i32 plVal = player * value;
		if (plVal == WIN) {
			max = WIN;
			if (best1 != move) {	// do not cut a branch under you
				if (best1 != null) {
					FreeMove(best1);
				}
				best1 = move;
			}
			if (best2 != null) {
				FreeMove(best2);
				best2 = null;
			}
			
			//pruning
			alpha = WIN;
			pruned = true;
			RevertLastMove();
			DBG(prunedCount++);
			DPRINT2("pruned, d = 1 or win alpha %d, beta %d", alpha, beta);
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			ttType = UPPER_BOUND;
			break;
		} else if (depth == 1) {
			searchedNodes++;
			i32 val = 0;
			val = player * (materialValue + StaticValueBeginner());
			if (val > max) {
				max = val;
				if (best1 != move) {	// do not cut a branch under you
					if (best1 != null) {
						FreeMove(best1);
					}
					best1 = move;
				}
			}
			if (val > alpha) {
				alpha = val;
				ttType = EXACT_VALUE;
				if (alpha >= beta) {
					pruned = true;
					RevertLastMove();
					DBG(prunedCount++);
					DPRINT2("pruned, d = 1");
					do {
						tmp = move;
						move = move->next;
						if (tmp != best1)
							FreeMove(tmp);
					} while (move != NULL);
					ttType = UPPER_BOUND;
					break;
				}
			}
		} else {
			GenerateAllMovesSortedMove2(&move2);
			start2 = move2;
			
			while (move2 != NULL) {
				DPRINT2("start exec move 2");
				ASSERT2(IsMovePossible(move2), "AB: move 2 not possible");
				ExecuteMove(move2);
				DPRINT2("exec move 2, depth %d", depth);
				moveCount++;
				val = -AlphaBetaPVMOBeginner(depth - 2, -beta, -alpha);
				RevertLastMove();
				DPRINT2("rev move 2");
				if (val > max) {
					max = val;
					if (best1 != move) {	// do not cut a branch under you
						if (best1 != null) {
							FreeMove(best1);
						}
						best1 = move;
					}
					if (best2 != null)
						FreeMove(best2);
					best2 = move2;
				}
				if (val > alpha) {
					alpha = val;
					if (alpha >= beta) {
						pruned = true;
						DBG(prunedCount++);
						do {
							tmp = move2;
							move2 = move2->next;
							if (best2 != tmp)
								FreeMove(tmp);
						} while (move2 != NULL);
						ttType = UPPER_BOUND;
						break;
					}
					ttType = EXACT_VALUE;
				}
				tmp = move2;
				move2 = move2->next;
				if (tmp != best2)
					FreeMove(tmp);
			}
		}
		RevertLastMove();
		if (pruned) {
			do {
				tmp = move;
				move = move->next;
				if (tmp != best1)
					FreeMove(tmp);
			} while (move != NULL);
			break;
		}
		tmp = move;
		move = move->next;
		if (tmp != best1)
			FreeMove(tmp);
	}
	ASSERT2(max > -WIN - 1, "AB: too low max %d", max);
	DPRINT2("abPVMO END - depth %d, alpha %d, beta %d, searched %d", depth, alpha, beta, searchedNodes);
	AddPositionToTT(max, ttType, depth, searchedNodes - initSearchedNodes, best1, best2);
	ASSERT2(moveNumber == 1, "bad moveNumber turnNumber %d, moveNumber %d", turnNumber, moveNumber);
	return max;
}

/// random AlphaBeta with TT and Principal Variation Move and Move Ordering and beginner evaluation function
i32 AlphaBetaPVMORandomBeginner(i32 depth, i32 randomMargin)
{
	DPRINT2("abPVMO RANDOM beginner -> alpha = -beta = -WIN; depth %d, searched %d, pl %d", depth, searchedNodes,
		player);
	ASSERT2(depth == currDepth, "alfa-beta RANDOM PVMO not in top level of search");
	ASSERT2(depth > 1, "random alfa-beta should have depth > 1");
	searchedNodes++;
	i32 moveCount = 0, initSearchedNodes = searchedNodes;
	i32 val, max = -WIN - 1;
	moveNumber = 1;
	Move *move, *move2, *start1, *start2;
	FullMovesList *allMoves = null;
	GenerateAllMovesSortedMove1(&move);
	start1 = move;
	while (move != NULL) {
		ExecuteMove(move);
		GenerateAllMovesSortedMove2(&move2);
		start2 = move2;
		while (move2 != NULL) {
			ExecuteMove(move2);
			moveCount++;
			val = -AlphaBetaPVMOBeginner(depth - 2, -WIN, -max + randomMargin + 1);
			if (val >= max - randomMargin) {
				FullMovesList *nMove = MALLOC(FullMovesList);
				nMove->move1 = move;
				nMove->move2 = move2;
				nMove->value = val;
				nMove->next = allMoves;
				allMoves = nMove;
			}
			RevertLastMove();
			if (val > max) {
				max = val;
			}
			move2 = move2->next;
		}
		RevertLastMove();
		move = move->next;
	}
	DPRINT2("random selecting started");
	i32 goodEnoughMoves = 0;
	FullMovesList *curr = allMoves, *selected = null;
	while (curr != null) {
		if (curr->value >= max - randomMargin)
			goodEnoughMoves++;
		curr = curr->next;
	}
	ASSERT2(goodEnoughMoves > 0, "there are no moves for random choice!");
	srand(time(null));
	i32 selectedMove = rand() % goodEnoughMoves;
	DPRINT("found %d good moves, selected random move: %d", goodEnoughMoves, selectedMove);
	curr = allMoves;
	i32 i = 0;
	while (curr != null) {
		if (curr->value >= max - randomMargin) {
			if (i == selectedMove) {
				selected = curr;
			} else {
				FreeMove(curr->move2);
				if ((curr->next == null || curr->next->move1 != curr->move1)
				    && (selected == null || curr->move1 != selected->move1))
					FreeMove(curr->move1);
			}
			i++;
		} else {
			FreeMove(curr->move2);
			if ((curr->next == null || curr->next->move1 != curr->move1)
			    && (selected == null || curr->move1 != selected->move1))
				FreeMove(curr->move1);
		}
		curr = curr->next;
	}
	DPRINT("move count on top level: %d", moveCount);
	ASSERT2(selected != null, "random select == null");
	DPRINT("max value: %d, selected move value: %d", max, selected->value);
	AddPositionToTT(max, EXACT_VALUE, depth, searchedNodes - initSearchedNodes, selected->move1, selected->move2);	//because of retrieving in GetBestMove; it's called only once
	return max;
}
