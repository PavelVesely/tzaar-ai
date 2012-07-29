/*
 * In the header file there are constants for DFPNS enhancements.
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef PNS_H_INCLUDED
#define PNS_H_INCLUDED

#include "tzaarlib.h"
// array for hasing
#include "hashedpositions.h"

#define INFINITY 2000000000u

#define TT2SIZE (1 << 20)
#define DFPNS_EPS_DIV 8
#define DFPNS_SEARCH_NODES 10000000	//for estimating search duration
// for eval based PNS
#define EFBPNS_T 50000000
#define EFBPNS_A 10
#define EFBPNS_B 10
// weak PNS heuristic counting
#define WPNS_T 1000000
#define WPNS_H 1
// for dynamic widening
#define DWPNS_J 5

extern u32 maxDfpnsSearchedNodes;

typedef struct tt2Entry {
	thash hash;
	u32 pn, dn;
	u32 minWinningDepth, maxLosingDepth;
	u32 searchedNodes;

} TT2Entry;
TT2Entry *DFPNSTranspositionTable[2 * TT2SIZE];	//2* because of replacement schema Twobig

FullMove *dfpns(u32 depth, u32 tpn, u32 tdn);
FullMove *dfpnsEpsTrick(u32 depth, u32 tpn, u32 tdn);
FullMove *weakpns(u32 depth, u32 tpn, u32 tdn);
FullMove *dfpnsEvalBased(u32 depth, u32 tpn, u32 tdn);
FullMove *dfpnsWeakEpsEval(u32 depth, u32 tpn, u32 tdn);
FullMove *dfpnsDynWideningEpsEval(u32 depth, u32 tpn, u32 tdn);
TT2Entry *LookupPositionInTT2();
void FreeTT2Entry(TT2Entry * entry);
void AddPositionToTT2(u32 pn, u32 dn, u32 minWinningDepth, u32 maxLosingDepth, u32 searchedNodes);

#endif				// PNS_H_INCLUDED
