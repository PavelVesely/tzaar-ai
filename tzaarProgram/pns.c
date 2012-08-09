/*
 * The module pns contains the implementation of the Depth-first Proof-number
 * Search (DFPNS) with some enhancements and the Transposition Table (TT) used
 * by DFPNS
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#include "pns.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

u32 maxDfpnsSearchedNodes;

inline __attribute__ ((always_inline))
TT2Entry *LookupPositionInTT2()
{
	u32 index = hash % TT2SIZE;
	if (DFPNSTranspositionTable[index] != null && DFPNSTranspositionTable[index]->hash == hash)
		return DFPNSTranspositionTable[index];
	else if (DFPNSTranspositionTable[index + TT2SIZE] != null
		 && DFPNSTranspositionTable[index + TT2SIZE]->hash == hash)
		return DFPNSTranspositionTable[index + TT2SIZE];
	else
		return null;
}

inline __attribute__ ((always_inline))
void FreeTT2Entry(TT2Entry * entry)
{
	ASSERT2(entry != null, "entry to free null");
	DBG(entry2Alive--);
	free(entry);
}

inline __attribute__ ((always_inline))
void AddPositionToTT2(u32 pn, u32 dn, u32 minWinningDepth, u32 maxLosingDepth, u32 searchedNodes)
{
	u32 index = hash % TT2SIZE;
	if (DFPNSTranspositionTable[index] != null && DFPNSTranspositionTable[index]->hash == hash) {
		if (searchedNodes > DFPNSTranspositionTable[index]->searchedNodes) {
			DFPNSTranspositionTable[index]->searchedNodes = searchedNodes;
			DFPNSTranspositionTable[index]->pn = pn;
			DFPNSTranspositionTable[index]->dn = dn;
			DFPNSTranspositionTable[index]->minWinningDepth = minWinningDepth;
			DFPNSTranspositionTable[index]->maxLosingDepth = maxLosingDepth;
		}
		return;
	}
	if (DFPNSTranspositionTable[index + TT2SIZE] != null && DFPNSTranspositionTable[index + TT2SIZE]->hash == hash) {
		if (searchedNodes > DFPNSTranspositionTable[index + TT2SIZE]->searchedNodes) {
			DFPNSTranspositionTable[index + TT2SIZE]->searchedNodes = searchedNodes;
			DFPNSTranspositionTable[index + TT2SIZE]->pn = pn;
			DFPNSTranspositionTable[index + TT2SIZE]->dn = dn;
			DFPNSTranspositionTable[index + TT2SIZE]->minWinningDepth = minWinningDepth;
			DFPNSTranspositionTable[index + TT2SIZE]->maxLosingDepth = maxLosingDepth;
		}
		return;
	}
	TT2Entry *entry = MALLOC(TT2Entry);
	DBG(entry2Alive++);
	entry->pn = pn;
	entry->dn = dn;
	entry->minWinningDepth = minWinningDepth;
	entry->maxLosingDepth = maxLosingDepth;
	entry->hash = hash;
	entry->searchedNodes = searchedNodes;
	TT2Entry *toSave = entry;
	if (DFPNSTranspositionTable[index] == null || DFPNSTranspositionTable[index]->searchedNodes < searchedNodes) {
		toSave = DFPNSTranspositionTable[index];
		DFPNSTranspositionTable[index] = entry;
	}
	if (toSave != null) {	// this holds: DFPNSTranspositionTable[index]->searchedNodes > DFPNSTranspositionTable[index + TT2SIZE]->searchedNodes
		TT2Entry *old = DFPNSTranspositionTable[index + TT2SIZE];
		DFPNSTranspositionTable[index + TT2SIZE] = toSave;
		if (old != null) {
			FreeTT2Entry(old);
			DBG(tt2Kick++);
			DPRINT2("tt2 kick");
		}
	}
}

/// dfpns without enhancements
FullMove *dfpns(u32 depth, u32 tpn, u32 tdn)
{
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	GenerateAllMovesSorted(&move);
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, sumDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, sumDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, sumDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							pn = 1;
							dn = 1;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(sumDN <= INFINITY && sumDN >= 0, "sumDN > INFINITY, sumDN = %d", sumDN);
					if (dn == INFINITY || sumDN == INFINITY)
						sumDN = INFINITY;
					else sumDN += dn; // not weak

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, sumDN %d", depth,
								minPN, sumDN);
							ASSERT2(sumDN == INFINITY, "minPN == 0 and sumDN == %d", sumDN);
							AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (sumDN == 0 || sumDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && sumDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		dfpns(depth + 1, tdn - sumDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}

/// dfpns + 1 + Epsilon Trick
FullMove *dfpnsEpsTrick(u32 depth, u32 tpn, u32 tdn)
{				
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	GenerateAllMovesSorted(&move);
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, sumDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, sumDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, sumDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							pn = 1;
							dn = 1;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(sumDN <= INFINITY && sumDN >= 0, "sumDN > INFINITY, sumDN = %d", sumDN);
					if (dn == INFINITY || sumDN == INFINITY)
						sumDN = INFINITY;
					else sumDN += dn; // not weak

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, sumDN %d", depth,
								minPN, sumDN);
							ASSERT2(sumDN == INFINITY, "minPN == 0 and sumDN == %d", sumDN);
							AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (sumDN == 0 || sumDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && sumDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2 + minPN2 / DFPNS_EPS_DIV);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		dfpnsEpsTrick(depth + 1, tdn - sumDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}

/// dfpns + Weak PNS
FullMove *weakpns(u32 depth, u32 tpn, u32 tdn)
{				
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	GenerateAllMovesSorted(&move);
	int currVal = materialValue + StaticValue();
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, maxDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, maxDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, maxDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							pn = 1;
							dn = 1;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(maxDN <= INFINITY && maxDN >= 0, "maxDN > INFINITY, maxDN = %d", maxDN);
					if (dn == INFINITY || maxDN == INFINITY)
						maxDN = INFINITY;
					else
						maxDN = MAX(maxDN, dn);	//weak
					//else maxDN += dn; // not weak

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, maxDN %d", depth,
								minPN, maxDN);
							ASSERT2(maxDN == INFINITY, "minPN == 0 and maxDN == %d", maxDN);
							AddPositionToTT2(minPN, maxDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		if (maxDN > 0) {
			//maxDN += moveCount - 1;	//weak (usual)
			// weak with heuristic counting -- using a similar step function as eval based pns
			i32 step = 0;
			if (currVal >= -WPNS_T)
				step++;
			if (currVal >= WPNS_T)
				step++;
			maxDN += (moveCount - 1) * (step * WPNS_H);	// + 1
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (maxDN == 0 || maxDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, maxDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && maxDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		weakpns(depth + 1, tdn - maxDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}

/// dfpns + Evaluation Function Based Enhancement
FullMove *dfpnsEvalBased(u32 depth, u32 tpn, u32 tdn)
{				
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	GenerateAllMovesSorted(&move);
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, sumDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, sumDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, sumDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							//step function
							i32 step = 0;
							i32 val = materialValue + StaticValue();
							if (val >= -EFBPNS_T)
								step++;
							if (val >= EFBPNS_T)
								step++;
							pn = ((2 - step) * EFBPNS_B + 1)
								* BranchingFactorByFreeFields[TOTAL_STONES - stoneSum];
							dn = 1 + EFBPNS_A * step;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(sumDN <= INFINITY && sumDN >= 0, "sumDN > INFINITY, sumDN = %d", sumDN);
					if (dn == INFINITY || sumDN == INFINITY)
						sumDN = INFINITY;
					else sumDN += dn; // not weak

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, sumDN %d", depth,
								minPN, sumDN);
							ASSERT2(sumDN == INFINITY, "minPN == 0 and sumDN == %d", sumDN);
							AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (sumDN == 0 || sumDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && sumDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		dfpnsEvalBased(depth + 1, tdn - sumDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}

/// dfpns + Weak PNS, 1 + Epsilon Trick, Evaluation Function Based Enhancement
FullMove *dfpnsWeakEpsEval(u32 depth, u32 tpn, u32 tdn)
{				
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	GenerateAllMovesSorted(&move);
	int currVal = materialValue + StaticValue();
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, maxDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, maxDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, maxDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							//step function
							i32 step = 0;
							i32 val = materialValue + StaticValue();
							if (val >= -EFBPNS_T)
								step++;
							if (val >= EFBPNS_T)
								step++;
							pn = ((2 - step) * EFBPNS_B + 1)
								* BranchingFactorByFreeFields[TOTAL_STONES - stoneSum];
							dn = 1 + EFBPNS_A * step;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(maxDN <= INFINITY && maxDN >= 0, "maxDN > INFINITY, maxDN = %d", maxDN);
					if (dn == INFINITY || maxDN == INFINITY)
						maxDN = INFINITY;
					else
						maxDN = MAX(maxDN, dn);	//weak
					//else maxDN += dn; // not weak

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, maxDN %d", depth,
								minPN, maxDN);
							ASSERT2(maxDN == INFINITY, "minPN == 0 and maxDN == %d", maxDN);
							AddPositionToTT2(minPN, maxDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		if (maxDN > 0) {
			//maxDN += moveCount - 1;	//weak (usual)
			// weak with heuristic counting -- using a similar step function as eval based pns
			i32 step = 0;
			if (currVal >= -WPNS_T)
				step++;
			if (currVal >= WPNS_T)
				step++;
			maxDN += (moveCount - 1) * (step * WPNS_H);	// + 1
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (maxDN == 0 || maxDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, maxDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && maxDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2 + minPN2 / DFPNS_EPS_DIV);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		dfpnsWeakEpsEval(depth + 1, tdn - maxDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}

/// dfpns + Dynamic Widening, 1 + Epsilon Trick, Evaluation Function Based Enhancement
FullMove *dfpnsDynWideningEpsEval(u32 depth, u32 tpn, u32 tdn)
{				
	u32 initSearchedNodes = searchedNodes;
	DPRINT2("PNS: player %d tpn %d tdn %d depth %d searched %u", player, tpn, tdn, depth, searchedNodes);
	ASSERT2(!IsEndOfGame(), "pns starting in a final position, val %d, depth %d", value, depth);
	Move *move, *move2, *curr, *curr2, *tmp;
	Move *maxLooseMove1 = null, *maxLooseMove2 = null;	// for counting the best move in lost position
	u32 maxDNArray[DWPNS_J + 1], minPNArray[DWPNS_J + 1];
	GenerateAllMovesSorted(&move);
	while (1) {
		curr = move;
		Move *minPNm1 = null, *minPNm2 = null;
		u32 minPN = INFINITY, minPN2 = INFINITY, maxDN = 0, minDN = INFINITY, moveCount = 0;
		u32 minWinningDepth = INFINITY, maxLosingDepth = 0;	// for counting the best move in lost position
		FOR(i, 0, DWPNS_J) maxDNArray[i] = 0;
		FOR(i, 0, DWPNS_J) minPNArray[i] = INFINITY + 1;
		maxDNArray[DWPNS_J] = INFINITY + 1; // stopper
		minPNArray[DWPNS_J] = INFINITY + 1; // stopper
		while (curr != null) {
			ExecuteMove(curr);
			i32 plVal = player * value;
			if (plVal == WIN) {
				searchedNodes++;
				//imediate pruning
				RevertLastMove();
				DPRINT2("PNS: pruning fast after one move, depth %d, minPN 0, maxDN INFTY", depth);
				if (minPNm2 != null) {
					FreeMove(minPNm2);
				}
				AddPositionToTT2(0, INFINITY, 1, INFINITY, 1);	//searchedNodes - initSearchedNodes == 0
				if (depth == 1) {
					FreeAllMoves(move, curr);
					FullMove *fm = MALLOC(FullMove);
					fm->m1 = curr;
					fm->m2 = null;
					return fm;
				} else {
					FreeAllMovesWithoutException(move);
					return null;
				}
			}
			else {
				GenerateAllMovesSorted(&move2);
				curr2 = move2;
				while (curr2 != null) {
					ExecuteMove(curr2);
					searchedNodes++;
					u32 pn, dn, winningDepth, losingDep;	// pn and dn are swaped between tree layers
					if (abs(value) == WIN) {
						if (player * value > 0) {
							pn = INFINITY;
							dn = 0;
							winningDepth = INFINITY;
							losingDep = 2;
						} else {
							//imediate pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning fast, depth %d, minPN 0, maxDN INFTY",
								depth);
							if (minPNm2 != null) {
								FreeMove(minPNm2);
							}
							AddPositionToTT2(0, INFINITY, 2, INFINITY, 2);	//searchedNodes - initSearchedNodes == 0; 2 is depth
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
					} else {
						TT2Entry *entry2 = LookupPositionInTT2();
						if (entry2 != null) {
							DBG(tt2Found++);
							pn = entry2->dn;
							dn = entry2->pn;
							winningDepth = entry2->maxLosingDepth + 2;
							losingDep = entry2->minWinningDepth + 2;
							if (winningDepth > INFINITY)
								winningDepth = INFINITY;
						} else {
							//step function
							i32 step = 0;
							i32 val = materialValue + StaticValue();
							if (val >= -EFBPNS_T)
								step++;
							if (val >= EFBPNS_T)
								step++;
							pn = ((2 - step) * EFBPNS_B + 1)
								* BranchingFactorByFreeFields[TOTAL_STONES - stoneSum];
							dn = 1 + EFBPNS_A * step;
							winningDepth = INFINITY;
							losingDep = 3;
						}
					}
					moveCount++;

					if (winningDepth < minWinningDepth)
						minWinningDepth = winningDepth;
					if (losingDep > maxLosingDepth) {
						maxLosingDepth = losingDep;
						maxLooseMove1 = curr;
						maxLooseMove2 = CloneMove(curr2);	// otherwise it will be deallocated
					}

					ASSERT2(dn <= INFINITY && dn >= 0, "dn > INFINITY, dn = %d", dn);
					ASSERT2(maxDNArray[0] <= INFINITY && maxDNArray[0] >= 0, "maxDNArray[0] > INFINITY, maxDNArray[0] = %d", maxDNArray[0]);
					if (dn == INFINITY || maxDNArray[0] == INFINITY)
						maxDNArray[0] = INFINITY;
					else { // dynamic widening
						u32 i = 0;
						while (pn >= minPNArray[i] && i < DWPNS_J)
							i++;
						if (i < DWPNS_J) {
							for (u32 j = DWPNS_J - 1; j > i; j--) {
								maxDNArray[j] = maxDNArray[j - 1];
								minPNArray[j] = minPNArray[j - 1];
							}
							maxDNArray[i] = dn;
							minPNArray[i] = pn;
						}
					}

					if (pn < minPN || (pn == minPN && dn < minDN)) {
						minPN2 = minPN;
						minPN = pn;
						minDN = dn;
						if (minPNm2 != null) {
							FreeMove(minPNm2);
						}
						if (minPN == 0) {	// pruning
							RevertLastMove();
							RevertLastMove();
							DPRINT2("PNS: pruning, depth %d, minPN %d, maxDN %d", depth,
								minPN, maxDN);
							ASSERT2(maxDN == INFINITY, "minPN == 0 and maxDN == %d", maxDN);
							AddPositionToTT2(minPN, maxDN, minWinningDepth, maxLosingDepth,
									 searchedNodes - initSearchedNodes);
							if (depth == 1) {
								FreeAllMovesWithoutException(curr2->next);
								FreeAllMoves(move, curr);
								FullMove *fm = MALLOC(FullMove);
								fm->m1 = curr;
								fm->m2 = curr2;
								return fm;
							} else {
								FreeAllMovesWithoutException(curr2);
								FreeAllMovesWithoutException(move);
								return null;
							}
						}
						minPNm1 = curr;
						minPNm2 = curr2;
					} else if (pn < minPN2)
						minPN2 = pn;
					RevertLastMove();
					tmp = curr2;
					curr2 = curr2->next;
					if (tmp != minPNm2)
						FreeMove(tmp);
				}
			}
			RevertLastMove();
			curr = curr->next;
		}
		// dynamic widening
		u32 sumDN = 0;
		FOR(i, 0, DWPNS_J) {
		  sumDN += maxDNArray[i];
		}
		ASSERT2(minPN > 0, "dfpns: minPN should be > 0, but it's %d", minPN);
		if (sumDN == 0 || sumDN >= tdn || minPN >= tpn || searchedNodes > maxDfpnsSearchedNodes) {
			AddPositionToTT2(minPN, sumDN, minWinningDepth, maxLosingDepth, searchedNodes - initSearchedNodes);
			if (depth == 1 && sumDN == 0) {
				DPRINT("I'm loser and max losing depth is %d", maxLosingDepth);
				DPRINT("move 1: f %d t %d", maxLooseMove1->from, maxLooseMove1->to);
				DPRINT("move 2: f %d t %d", maxLooseMove2->from, maxLooseMove2->to);
				FreeAllMoves(move, maxLooseMove1);
				if (minPNm2 != null)
					FreeMove(minPNm2);
				FullMove *fm = MALLOC(FullMove);
				fm->m1 = maxLooseMove1;
				fm->m2 = maxLooseMove2;
				return fm;
			} else {
				if (minPNm2 != null)
					FreeMove(minPNm2);
				ASSERT2(minPN > 0, "exiting dfpns and minPN %d", minPN);
				FreeAllMovesWithoutException(move);
				return null;
			}
		}
		u32 ntdn = MIN(tpn, 1 + minPN2 + minPN2 / DFPNS_EPS_DIV);
		if (minPN2 == INFINITY)
			ntdn = tpn;
		ExecuteMove(minPNm1);
		ExecuteMove(minPNm2);
		dfpnsDynWideningEpsEval(depth + 1, tdn - sumDN + minDN, ntdn);
		RevertLastMove();
		RevertLastMove();
		FreeMove(minPNm2);
	}
}
