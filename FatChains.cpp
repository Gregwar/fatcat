#include <map>
#include <stdio.h>
#include <set>
#include <vector>
#include <iostream>
#include "FatChains.h"
#include "FatChain.h"
#include "FatEntry.h"

using namespace std;
        
FatChains::FatChains(FatSystem &system)
    : FatModule(system)
{
}

void FatChains::chainsAnalysis()
{
    cout << "Building the chains..." << endl;
    map<int, FatChain> chains = findChains();

    cout << "Found " << chains.size() << " chains" << endl;

    cout << "Running the recursive differential analysis..." << endl;
    set<int> visited;
    recursiveExploration(chains, visited, system.rootDirectory);

    cout << "Having a look at the chains..." << endl;
    map<int, FatChain>::iterator it;
    vector<FatChain> orphanedChains;
    int orphaned = 0;
    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;
        if (chain.orphaned) {
            orphaned++;
            orphanedChains.push_back(chain);
        }
    }

    if (orphaned) {
        cout << "There is " << orphaned << " orphaned chains:" << endl;
        vector<FatChain>::iterator vit;

        for (vit=orphanedChains.begin(); vit!=orphanedChains.end(); vit++) {
            FatChain &chain = (*vit);
            cout << "Chain from cluster " << chain.startCluster << " to " << chain.endCluster << " (size " << chain.size() << ")" << endl;
        }
    } else {
        cout << "There is no orphaned chains, disk seems clean!" << endl;
    }
}
        
void FatChains::recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster)
{
    if (visited.find(cluster) != visited.end()) {
        return;
    }
    visited.insert(cluster);

    vector<FatEntry> entries = system.getEntries(cluster);
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = (*it);
        int cluster = entry.cluster;

        // Search the cluster in the previously visited chains, if it
        // exists, mark it as non-orphaned
        if (chains.find(cluster) != chains.end()) {
            chains[cluster].orphaned = false;
        }

        if (entry.isDirectory()) {
            recursiveExploration(chains, visited, entry.cluster);
        }
    }
}

map<int, FatChain> FatChains::findChains()
{
    set<int> seen;
    map<int, FatChain> chains;

    for (int cluster=system.rootDirectory; cluster<system.totalClusters; cluster++) {
        if (seen.find(cluster) == seen.end()) {
            if (!system.freeCluster(cluster)) {
                set<int> localSeen;
                int next = cluster;
                while (true) {
                    int tmp = system.nextCluster(next);
                    if (tmp == FAT_LAST) {
                        break;
                    }
                    if (localSeen.find(tmp) != localSeen.end()) {
                        fprintf(stderr, "! Loop\n");
                        break;
                    }
                    next = tmp;
                    seen.insert(next);
                    localSeen.insert(next);
                }

                FatChain chain;
                chain.startCluster = cluster;
                chain.endCluster = next;

                if (chain.startCluster == system.rootDirectory) {
                    chain.orphaned = false;
                }
                    
                chains[next] = chain;
            }

            seen.insert(cluster);
        }
    }

    map<int, FatChain> chainsByStart;
    map<int, FatChain>::iterator it;
    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;
        chainsByStart[chain.startCluster] = chain;
    }

    return chainsByStart;
}
