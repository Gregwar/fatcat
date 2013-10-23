#ifndef _FATCAT_FATCHAINS_H
#define _FATCAT_FATCHAINS_H

#include <set>
#include <map>
#include <vector>
#include <iostream>
#include "FatSystem.h"
#include "FatModule.h"
#include "FatChain.h"

using namespace std;

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
        void recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster);

        /**
         * Find the chains from the FAT and return a map indexed
         * by the first cluster of each chain
         */
        map<int, FatChain> findChains();
};

#endif // _FATCAT_FATCHAINS_H
