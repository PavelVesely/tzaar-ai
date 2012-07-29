/*
 * The module tzaarlib starts the search according to the chosen algorithm and
 * does the control of the time limit via the Iterative Deepening for the
 * Alpha-beta based algorithms and for DFPNS via the estimation of the maximal
 * number of nodes that can be searched.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "tzaarlib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

// Position reprezentation
i32 board[BOARD_ARRAY_SIZE];
i32 stackHeights[BOARD_ARRAY_SIZE];
i32 player, moveNumber;

// History
i32 turnNumber;
Move *history[MAX_MOVES];

// Position properties useful for the evaluation function and the Move Ordering
i32 counts[STONE_TYPES];
i32 stoneSum;
i32 value;
i32 materialValue;
thash hash;
i32 highestStack[STONE_TYPES];
i32 countsByHeight[STONE_TYPES][MAX_STACK_HEIGHT];
i32 zoneOfControl[STONE_TYPES], threatenByCounts[BOARD_ARRAY_SIZE];

// For saving
double searchDuration;

// For searching (AB and PNS)
i32 currDepth;	//for test
u32 searchedNodes;

// Debug constants
#ifdef DEBUG
i32 moveAlive, entryAlive, ttHit, ttFound, ttKick, prunedCount, entry2Alive, tt2Kick, tt2Hit, tt2Found, ttCollision;
#endif

typedef int_fast64_t ttimestamp;

/// Returns current time in microseconds
static ttimestamp get_timer()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return 1000000LL * t.tv_sec + t.tv_usec;
}

/// Returns difference between tEnd time and tStart times in seconds
double getDurationInSecs(ttimestamp tStart, ttimestamp tEnd)
{
	double res = (double) (tEnd - tStart) / 1e6;
	while (res < 0)
		res += 86400;
	return res;
}

/// Searches for the best move in a given position using algorithm determined by the parameter ai.
/// RecallAI is a parameter for DFPNS -- when it's not able to prove or disprove position, it calls Alpha-beta if recallAI is true.
/// Returns false when an error occurred, otherwise true.
i32 GetBestMove(i32 ai, i32 time, Move ** move1, Move ** move2, bool recallAI)
{
	ASSERT2(moveNumber == 1, "GetBest: moveNumber should be 1");
	if (moveNumber == 2) return false;
	*move1 = null;
	*move2 = null;
	if (ai == -1)
		ai = MAIN_AI;
	// switch between different search methods or AIs
	if (ai == AIALPHABETA) {	// Alpha-beta without iterative deepening (ID)
		searchedNodes = 0;
		// set debug counters
		DBG(moveAlive = entryAlive = ttHit = ttFound = ttKick = prunedCount = ttCollision = 0);
		i32 ret = 0;
		ttimestamp tStart = get_timer();
		DPRINT("ALPHA BETA WITH TT, sum of stones: %d", stoneSum);
		ret = AlphaBeta(ALPHABETA_DEPTH, -WIN - 1, WIN + 1);
		ttimestamp tEnd = get_timer();
		searchDuration = getDurationInSecs(tStart, tEnd);
		value = ret;	// because of saving
		DPRINT("Alpha-Beta: time %0.3f s, searched: %d, return: %d, pruned %d", searchDuration, searchedNodes,
		       ret, prunedCount);
		DPRINT("Alive: move %d, entries %d, kicks from TT %d, ttHits %d, ttFound %d", moveAlive,
		       entryAlive, ttKick, ttHit, ttFound);
		ASSERT(ttCollision == 0 || ttCollision > 1000000, "FOUND TT COLLISION: %d", ttCollision);
		TTEntry *saved = LookupPositionInTT();
		if (saved == null) {
			DPRINT("Error: cannot find position in TT!!!\n");
			return false;
		}
		*move1 = CloneMove(saved->bestMove1);
		if (stoneSum < 60)
			*move2 = CloneMove(saved->bestMove2);
		else
			*move2 = null;
	} else if (ai >= AIALPHABETA_ID && ai <= AIALPHABETA_MAX) {	//Alpha-beta with ID
		i32 ret = 0;
		ttimestamp tStart = get_timer();
		ttimestamp tID;
		i32 depth = 2;
		double lastTime, currTime = 0;
		Move *m1 = null, *m2 = null, *lastm1, *lastm2;	//for better moves in losen positions (when using TT)
		i32 mult;
		do { // iterative deepening
			lastm1 = m1, lastm2 = m2;
			currDepth = depth;	// for getting branching factor on top level of the search
			searchedNodes = 0;
			// set debug counters
			DBG(moveAlive = entryAlive = ttHit = ttFound = ttKick = prunedCount = ttCollision = 0);
			if (ai == AIALPHABETA_ID) {	// alpha beta with TT and iterative deepening
				DPRINT("ALPHA BETA WITH TT and ID, sum of stones: %d", stoneSum);
				ret = AlphaBeta(depth, -WIN, WIN);
			} else if (ai == AIALPHABETA_ID_PV) {	// alpha beta with TT, iterative deepening and move from TT
				DPRINT("ALPHA BETA WITH TT and ID and PV, sum of stones: %d", stoneSum);
				ret = AlphaBetaPV(depth, -WIN, WIN);
			} else if (ai == AIALPHABETA_ID_PV_MO) {
				DPRINT("ALPHA BETA WITH TT and ID and PV and MO, sum of stones: %d", stoneSum);
				ret = AlphaBetaPVMO(depth, -WIN, WIN);
			} else if (ai == AIALPHABETA_ID_MO) {
				DPRINT("ALPHA BETA WITH ID and MO, sum of stones: %d", stoneSum);
				ret = AlphaBetaMO(depth, -WIN, WIN, &m1, &m2);
			} else if (ai == AIALPHABETA_RANDOM) {	// alpha beta with random move selecting
				DPRINT("ALPHA BETA RANDOM WITH TT and ID and PV and MO, sum of stones: %d, pl %d",
				       stoneSum, player);
				ttCollision = 0;
				ret = AlphaBetaPVMORandom(depth, AI_RANDOM_MARGIN);
			} else if (ai == AIALPHABETA_ID_PV_MO_SCOUT) {
				DPRINT("ALPHA BETA WITH TT and ID and PV and MO and NEGASCOUT, sum of stones: %d",
				       stoneSum);
				ret = AlphaBetaPVMONegascout(depth, -WIN, WIN);
			} else if (ai == AIALPHABETA_ID_PV_MO_HISTORY) {
				DPRINT("ALPHA BETA WITH TT and ID and PV and MO and HISTORY, sum of stones: %d",
				       stoneSum);
				ret = AlphaBetaPVMOHistory(depth, -WIN, WIN);
			} else if (ai == AIALPHABETA_ID_PV_MO_SCOUT_HISTORY) {
				DPRINT("ALPHA BETA WITH TT and ID, PV, MO, NEGASCOUT and HISTORY, sum of stones: %d",
				       stoneSum);
				ret = AlphaBetaPVMOHistoryNegascout(depth, -WIN, WIN);
			}
			DBG2(printZOCDebug());
			tID = get_timer();
			lastTime = currTime;
			currTime = getDurationInSecs(tStart, tID);
			DPRINT("Alpha-Beta: pl %d, depth %d, time %0.3f s, searched: %d, return: %d, pruned %d", player,
			       depth, currTime, searchedNodes, ret, prunedCount);
			DPRINT("Alive: move %d, entries %d, kicks from TT %d, ttHits %d, ttFound %d",
			       moveAlive, entryAlive, ttKick, ttHit, ttFound);
			ASSERT(ttCollision == 0 || ttCollision > 1000000, "FOUND TT COLLISION: %d", ttCollision);
			if (ai != AIALPHABETA_ID_MO) {
				TTEntry *saved = LookupPositionInTT();
				if (saved == null) {
					DPRINT("Error: cannot find position in TT!!!\n");
					return false;
				}
				m1 = CloneMove(saved->bestMove1); // the clone is needed because of possible kicks from TT
				if (saved->bestMove2 != null)
					m2 = CloneMove(saved->bestMove2);
			}
			if (abs(ret) == WIN) {
				if (ret == -WIN && lastm1 != null && lastm2 != null) {
					DPRINT("AB: I am looser :(");
					m1 = lastm1, m2 = lastm2;
				}
				if (ret == WIN)
					DPRINT("AB: I am winner!!!");
				break;
			}
			// estimate time for the next depth -- it's mult * current duration
			mult = AB_ID_timeMultByFreeFieldsEvenDepth[TOTAL_STONES - stoneSum + depth];
			if (depth % 2 == 0)
				mult = AB_ID_timeMultByFreeFieldsOddDepth[TOTAL_STONES - stoneSum + depth];
			DPRINT("mult for next depth: %d", mult);
			depth += 1;
		} while ((depth <= MIN_AB_DEPTH || currTime + (currTime - lastTime) * mult < time) && abs(ret) < WIN);
		searchDuration = currTime;
		value = ret;	// because of saving
		*move1 = m1;
		if (stoneSum < 60 && m2 != null)
			*move2 = m2;
	} else if (ai >= DFPNS && ai <= DFPNS_MAX) { // depth-first proof-number search
		ASSERT(abs(value) != WIN, "dfpns cannot be called when someone won");
		searchedNodes = 0;
		ttimestamp tStart;
		DBG(tStart = get_timer());
		ttimestamp tID;
		maxDfpnsSearchedNodes = DFPNS_SEARCH_NODES;
		TT2Entry *saved = null;
		searchDuration = 0;
		FullMove *fm = null;
		do {
			// set debug counters
			DBG(moveAlive = entry2Alive = tt2Hit = tt2Found = tt2Kick = prunedCount = ttCollision = 0);
			if (ai == DFPNS) {
				DPRINT("DFPNS obycejne, sum of stones %d:", stoneSum);
				fm = dfpns(1, INFINITY, INFINITY);
			} else if (ai == DFPNS_EPS_TRICK) {
				DPRINT("DFPNS s EPSILON TRIKEM, sum of stones %d:", stoneSum);
				fm = dfpnsEpsTrick(1, INFINITY, INFINITY);
			} else if (ai == WEAK_PNS) {
				DPRINT("DFPNS WEAK, sum of stones %d:", stoneSum);
				fm = weakpns(1, INFINITY, INFINITY);
			} else if (ai == DFPNS_EVAL_BASED) {
				DPRINT("DFPNS EVAL BASED, sum of stones %d:", stoneSum);
				fm = dfpnsEvalBased(1, INFINITY, INFINITY);
			} else if (ai == DFPNS_WEAK_EPS_EVAL) {
				DPRINT("DFPNS + WEAK, EPS. TRICK, EVAL BASED INIT, sum of stones %d:", stoneSum);
				fm = dfpnsWeakEpsEval(1, INFINITY, INFINITY);
			} else if (ai == DFPNS_DYNAMIC_WIDENING_EPS_EVAL) {
				DPRINT("DFPNS + DYNAMIC WIDENING, EPS. TRICK, EVAL BASED INIT, sum of stones %d:", stoneSum);
				fm = dfpnsDynWideningEpsEval(1, INFINITY, INFINITY);
			}
			DBG(tID = get_timer());
			float tm = (float) getDurationInSecs(tStart, tID);
			i32 ff = 0, tt = 0;
			if (fm != null) {	//fm null in not solved position
				ff = fm->m1->from, tt = fm->m1->to;
			}
			DPRINT("DFPNS: time %0.3f s, searched: %d", tm, searchedNodes);
			if (fm != null) {
				ASSERT(fm->m1->from != fm->m1->to, "fm->m1->from %d, fm->m1->to %d SE ROVNA",
				       fm->m1->from, fm->m1->to);
				fm->m1->from = ff;
				fm->m1->to = tt;
			}
			searchDuration = tm;
			DPRINT("Alive: pl %d, move %d, entries %d, kicks from TT %d, ttHits %d, ttFound %d", player, moveAlive, entry2Alive, tt2Kick, tt2Hit, tt2Found);
			maxDfpnsSearchedNodes = (i32) (((time - searchDuration) * searchedNodes * 1.1f) / searchDuration) + searchedNodes;	//* 1.1f because the estimation is too pesimistic
			if (maxDfpnsSearchedNodes < 1000) maxDfpnsSearchedNodes = 1000;
			DPRINT("next max dfpns searched nodes: %d", maxDfpnsSearchedNodes);
			saved = LookupPositionInTT2();
			if (saved == null) {
				DPRINT("Error: cannot find position in TT2!!!\n");
				return false;
			}
			DPRINT("DFPNS: pn = %d, dn = %d, searched %d", saved->pn, saved->dn, saved->searchedNodes);
		} while (searchDuration < time * 4 / 5.0f && saved->pn > 0 && saved->dn > 0 && saved->pn < INFINITY && saved->dn < INFINITY);	//AI_TIME_LIMIT * 4 / 5 -- because sometimes a few seconds are missing to time limit
		value = 0;
		if (saved->pn == 0 || saved->dn >= INFINITY) {
			ASSERT(fm != null, "fm in win pos null");
			const char *f1f = IndexToFieldName(fm->m1->from);
			const char *f1t = IndexToFieldName(fm->m1->to);
			DPRINT("saving move 1: from %s (%d), to %s (%d)", f1f, fm->m1->from, f1t, fm->m1->to);
			*move1 = fm->m1;
			*move2 = fm->m2;
			value = WIN;
			DPRINT("DFPNS: I am winner!!!");
		} else if (saved->dn == 0 || saved->pn >= INFINITY) {	//moves from TT are moves to pos with lost in highest depth
			ASSERT(fm != null, "fm in lost pos null");
			*move1 = fm->m1;
			*move2 = fm->m2;
			DPRINT("DFPNS: I'm looser :(");
			value = -WIN;
			DPRINT2("move 2: f %d t %d", (*move2)->from, (*move2)->to);
		} else {
			value = 0;
			if (recallAI) {
				i32 oldVal = value;
				value = 0;
				DPRINT("calling alpha-beta:");
				//call alpha-beta
				GetBestMove(AIALPHABETA_BEST, time, move1, move2, false);
				DPRINT("end call alpha-beta");
				searchDuration += getDurationInSecs(tStart, tID);
				value = oldVal;
			}
		}
	} else if (ai == BEGINNERS_AI) { // AI for beginners
		DPRINT("AI FOR BEGINNERS: AB PVMO beginner");
		FOR(i,0,15) { // set beginner material value constants
				StackHeightValue[i] = StackHeightValueBeginner[i];
		}
		i32 ret = 0;
		ttimestamp tStart = get_timer();
		ttimestamp tID;
		i32 depth = 2;
		double lastTime, currTime = 0;
		Move *m1 = null, *m2 = null, *lastm1, *lastm2;	//for better moves in losen positions (when using TT)
		do {
			lastm1 = m1, lastm2 = m2;
			currDepth = depth;
			searchedNodes = 0;
			DBG(moveAlive = entryAlive = ttHit = ttFound = ttKick = prunedCount = ttCollision = 0);
			ret = AlphaBetaPVMORandomBeginner(depth, AI_RANDOM_MARGIN_BIGGER);
			DBG2(printZOCDebug());
			tID = get_timer();
			lastTime = currTime;
			currTime = getDurationInSecs(tStart, tID);
			DPRINT("Alpha-Beta: pl %d, depth %d, time %0.3f s, searched: %d, return: %d, pruned %d", player,
			       depth, currTime, searchedNodes, ret, prunedCount);
			DPRINT("Alive: move %d, entries %d, kicks from TT %d, ttHits %d, ttFound %d",
			       moveAlive, entryAlive, ttKick, ttHit, ttFound);
			ASSERT(ttCollision == 0 || ttCollision > 1000000, "FOUND TT COLLISION: %d", ttCollision);
			TTEntry *saved = LookupPositionInTT();
			if (saved == null) {
				DPRINT("Error: cannot find position in TT!!!\n");
				return false;
			}
			m1 = CloneMove(saved->bestMove1);
			if (saved->bestMove2 != null)
				m2 = CloneMove(saved->bestMove2);
			if (abs(ret) == WIN) {
				if (ret == -WIN && lastm1 != null && lastm2 != null) {
					DPRINT("AB: I am looser :(");
					m1 = lastm1, m2 = lastm2;
				}
				if (ret == WIN)
					DPRINT("AB: I am winner!!!");
				break;
			}
			depth += 1;
		} while (depth <= BEGINNER_AB_DEPTH && abs(ret) < WIN);
		searchDuration = currTime;
		value = ret;	// because of saving
		*move1 = m1;
		if (stoneSum < 60 && m2 != null)
			*move2 = m2;
	} else if (ai == INTERMEDIATE_AI) { // AI for intermediate players
		DPRINT("AI INTERMEDIATE: AB PVMO");
		i32 ret = 0;
		ttimestamp tStart = get_timer();
		ttimestamp tID;
		i32 depth = 2;
		double lastTime, currTime = 0;
		Move *m1 = null, *m2 = null, *lastm1, *lastm2;	//for better moves in losen positions (when using TT)
		do {
			lastm1 = m1, lastm2 = m2;
			currDepth = depth;
			searchedNodes = 0;
			DBG(moveAlive = entryAlive = ttHit = ttFound = ttKick = prunedCount = ttCollision = 0);
			ret = AlphaBetaPVMORandom(depth, AI_RANDOM_MARGIN);
			DBG2(printZOCDebug());
			tID = get_timer();
			lastTime = currTime;
			currTime = getDurationInSecs(tStart, tID);
			DPRINT("Alpha-Beta: pl %d, depth %d, time %0.3f s, searched: %d, return: %d, pruned %d", player,
			       depth, currTime, searchedNodes, ret, prunedCount);
			DPRINT("Alive: move %d, entries %d, kicks from TT %d, ttHits %d, ttFound %d",
			       moveAlive, entryAlive, ttKick, ttHit, ttFound);
			ASSERT(ttCollision == 0 || ttCollision > 1000000, "FOUND TT COLLISION: %d", ttCollision);
			TTEntry *saved = LookupPositionInTT();
			if (saved == null) {
				DPRINT("Error: cannot find position in TT!!!\n");
				return false;
			}
			m1 = CloneMove(saved->bestMove1); // the clone is needed because of possible kicks from transposition table
			if (saved->bestMove2 != null)
				m2 = CloneMove(saved->bestMove2);
			if (abs(ret) == WIN) {
				if (ret == -WIN && lastm1 != null && lastm2 != null) {
					DPRINT("AB: I am looser :(");
					m1 = lastm1, m2 = lastm2;
				}
				if (ret == WIN)
					DPRINT("AB: I am winner!!!");
				break;
			}
			depth += 1;
		} while (depth <= INTERMEDIATE_AB_DEPTH && abs(ret) < WIN);
		searchDuration = currTime;
		value = ret;	//because of saving
		*move1 = m1;
		if (stoneSum < 60 && m2 != null)
			*move2 = m2;
	} else if (ai == AICOMBI_RANDOM_AB_PNS) { // the AI for experts
		DPRINT("COMBINATION OF AI: RANDOM, BEST ALPHA-BETA, BEST DFPNS");
		bool ab = false;
		if (stoneSum > 55) {
			ab = GetBestMove(AIALPHABETA_RANDOM, time, move1, move2, true);
		} else if (stoneSum > 23) {
			ab = GetBestMove(AIALPHABETA_BEST, time, move1, move2, true);
		} else {
			GetBestMove(DFPNS_BEST, time, move1, move2, true);
		}
	} else {
		DPRINT("Error: Unknown AI!!!\n");
		return false;
	}
	ASSERT(turnNumber == 1 && moveNumber == 1,
		"Moves not reverted! turn number %d, moveNumber %d", turnNumber, moveNumber);
	if (searchDuration > time * 3) {
		DPRINT("HIGH SEARCH DURATION!");
	}
	return true;
}

