#ifndef _FATCAT_FATCHAINS_H
#define _FATCAT_FATCHAINS_H

#include <set>
#include <map>
#include <string>
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
        bool recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster, bool force, vector<FatEntry> *inputEntries=NULL);

        /**
         * Find the chains from the FAT and return a map indexed
         * by the first cluster of each chain
         */
        map<int, FatChain> findChains();

        /**
         * Explore the chains
         */
        void exploreChains(map<int, FatChain> &chains, set<int> &visited);

        /**
         * Fix reachable directories
         */
        void fixReachable();
        void fixReachable(set<int> &visited, int cluster, string name);
        void fixChain(int cluster, int size);

        /**
         * Reference finder
         */
        void findEntry(int cluster);
        void findEntry(set<int> &visited, int cluster, int search, string name = "/");

    protected:
        bool saveEntries;
        map<int, vector<FatEntry> > orphanEntries;
        map<int, FatEntry> clusterToEntry;
};

#endif // _FATCAT_FATCHAINS_H
