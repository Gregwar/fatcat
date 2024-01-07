#ifndef _FATCAT_FATCHAINS_H
#define _FATCAT_FATCHAINS_H

#include <list>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include <core/FatSystem.h>
#include <core/FatModule.h>
#include "FatChain.h"

using namespace std;

typedef pair<int, int> Segment;

class FatChains : public FatModule
{
    public:
        FatChains(FatSystem &system);

        /**
         * Do a chain analysis on the fat system
         */
        void chainsAnalysis();

        /**
         * Recursive method to do an exploration and do the differential
         * chains analysis
         */
        bool recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster, vector<FatEntry> *inputEntries=NULL);

        /**
         * Find the chains from the FAT and return a map indexed
         * by the first cluster of each chain
         */
        map<int, FatChain> findChains();

        /**
         * For each chain, we try to tell if it's a directory and run recursive
         * exploration if it is
         */
        void exploreChains(map<int, FatChain> &chains, set<int> &visited);

        /**
         * Get chains that are orphaned
         */
        list<FatChain> getOrphaned(map<int, FatChain> &chains);

        /**
         * Display the orphaned chains
         */
        void displayOrphaned(list<FatChain> orphanedChains);

        /**
         * Size of a chain in the FAT
         */
        int chainSize(int cluster, bool *isContiguous, list<Segment>& segments);

    protected:
        bool saveEntries;
        bool exploreDamaged;
        map<int, vector<FatEntry> > orphanEntries;
        map<int, FatEntry> clusterToEntry;
};

#endif // _FATCAT_FATCHAINS_H
