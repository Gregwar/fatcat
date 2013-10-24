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
    cout << endl;

    cout << "Running the recursive differential analysis..." << endl;
    set<int> visited;
    recursiveExploration(chains, visited, system.rootDirectory, false);
    cout << endl;

    cout << "Having a look at the chains..." << endl;
    map<int, FatChain>::iterator it;
    vector<FatChain> orphanedChains;
    int orphaned = 0;

    bool foundNew;
    do {
        foundNew = false;
        for (it=chains.begin(); it!=chains.end(); it++) {
            FatChain &chain = it->second;

            if (chain.orphaned) {
                    if (system.isDirectory(chain.startCluster)) {
                        chain.directory = true;
                        if (recursiveExploration(chains, visited, chain.startCluster, true)) {
                            foundNew = true;
                        }
                        chain.orphaned = true;
                    }
            }
        }
    } while (foundNew);
    cout << endl;
    
    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;

        if (chain.orphaned) {
            orphaned++;
            orphanedChains.push_back(chain);
        }
    }

    if (orphaned) {
        cout << "There is " << orphaned << " orphaned elements:" << endl;
        vector<FatChain>::iterator vit;

        for (vit=orphanedChains.begin(); vit!=orphanedChains.end(); vit++) {
            FatChain &chain = (*vit);
            cout << "Chain from cluster " << chain.startCluster << " to " << chain.endCluster;
            if (chain.directory) {
                cout << " directory (" << chain.elements << " elements)";
            } else {
                cout << " file";
            }
            cout << endl;
        }
    } else {
        cout << "There is no orphaned chains, disk seems clean!" << endl;
    }
    cout << endl;
}
        
bool FatChains::recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster, bool force)
{
    if (visited.find(cluster) != visited.end()) {
        return false;
    }

    if (!force && system.nextCluster(cluster) == 0) {
        return false;
    }
    
    visited.insert(cluster);
    
    bool foundNew = false;
    int myCluster = cluster;

    cout << "Exploring " << cluster << endl;

    vector<FatEntry> entries = system.getEntries(cluster);
    vector<FatEntry>::iterator it;

    cout << myCluster << ": " << chains[myCluster].elements << " elements" << endl;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = (*it);
        int cluster = entry.cluster;
        bool wasOrphaned = false;

        // Search the cluster in the previously visited chains, if it
        // exists, mark it as non-orphaned
        if (chains.find(cluster)!=chains.end()) {
            if (entry.getFilename() != ".." && entry.getFilename() != ".") {
                if (chains[cluster].orphaned) {
                    wasOrphaned = true;
                }
                chains[cluster].orphaned = false;
            }
        } else {
            if (entry.getFilename() == "..") {
                chains[cluster].startCluster = cluster;
                chains[cluster].endCluster = cluster;
                chains[cluster].elements = 1;
                chains[cluster].orphaned = true;
                foundNew = true;
                cout << "NEW!!!" << endl;
            }
        }

        if (entry.isDirectory() && entry.getFilename() != "..") {
            recursiveExploration(chains, visited, cluster, false);
        }
        
        if (wasOrphaned) {
            cout << "Adding " << chains[cluster].elements << " elements to " << myCluster << " from " << cluster << endl;
            chains[myCluster].elements += chains[cluster].elements;
            cout << "Now: " << chains[myCluster].elements << endl;
        }
    }

    return foundNew;
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
