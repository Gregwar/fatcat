#include <map>
#include <list>
#include <stdio.h>
#include <set>
#include <vector>
#include <iostream>
#include "FatChains.h"
#include "FatChain.h"
#include "FatEntry.h"
#include "utils.h"

using namespace std;

bool compare_size(FatChain &A, FatChain &B)
{
    return A.size > B.size;
}
        
FatChains::FatChains(FatSystem &system)
    : FatModule(system),
    saveEntries(false)
{
}

void FatChains::chainsAnalysis()
{
    system.enableCache();

    cout << "Building the chains..." << endl;
    map<int, FatChain> chains = findChains();

    /*
    map<int, FatChain>::iterator mit;
    for (mit=chains.begin(); mit!=chains.end(); mit++) {
        cout << mit->first << endl;
    } 
    */

    cout << "Found " << chains.size() << " chains" << endl;
    cout << endl;

    cout << "Running the recursive differential analysis..." << endl;
    set<int> visited;
    saveEntries = false;
    recursiveExploration(chains, visited, system.rootDirectory, false);
    visited.insert(0);
    cout << endl;

    cout << "Having a look at the chains..." << endl;
    saveEntries = true;
    exploreChains(chains, visited);
   
    map<int, FatChain>::iterator it;
    int orphaned = 0;
    list<FatChain> orphanedChains;
    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;

        if (chain.startCluster == 0) {
            chain.orphaned = false;
        }

        if (chain.orphaned) {
            if (!chain.directory) {
                chain.size = chain.length*system.bytesPerCluster;
            }
            orphaned++;
            orphanedChains.push_back(chain);
        }
    }

    if (orphaned) {
        orphanedChains.sort(compare_size);
        unsigned long long totalSize = 0;
        cout << "There is " << orphaned << " orphaned elements:" << endl;
        list<FatChain>::iterator vit;

        for (vit=orphanedChains.begin(); vit!=orphanedChains.end(); vit++) {
            FatChain &chain = (*vit);
            if (chain.directory) {
                cout << "* Directory ";
            } else {
                cout << "* File ";
            }

            cout << "clusters " << chain.startCluster << " to " << chain.endCluster;

            if (chain.directory) {
                cout << ": " << chain.elements << " elements, ";
            } else {
                cout << ": ~";
            }
            cout << prettySize(chain.size);
            totalSize += chain.size;
            cout << endl;
        }
        cout << endl;
        cout << "Estimation of orphan files total sizes: " << totalSize << " (" << prettySize(totalSize) << ")" << endl;
        cout << endl;
        if (orphanEntries.size()) {
            map<int, vector<FatEntry> >::iterator mit;
            cout << "Listing of found elements with known entry:" << endl;

            for (mit=orphanEntries.begin(); mit!=orphanEntries.end(); mit++) {
                cout << "In directory with cluster " << mit->first;
                if (clusterToEntry.find(mit->first) != clusterToEntry.end()) {
                    cout << " (" << clusterToEntry[mit->first].getFilename() << ") ";
                }
                cout << ":" << endl;
                system.list(mit->second);
                cout << endl;
            }
        }
    } else {
        cout << "There is no orphaned chains, disk seems clean!" << endl;
    }
    cout << endl;
}

void FatChains::exploreChains(map<int, FatChain> &chains, set<int> &visited)
{
    map<int, FatChain>::iterator it;
    bool foundNew;
    do {
        foundNew = false;
        for (it=chains.begin(); it!=chains.end(); it++) {
            FatChain &chain = it->second;

            if (chain.orphaned) {
                // cout << "Trying " << chain.startCluster << endl;
                vector<FatEntry> entries = system.getEntries(chain.startCluster);
                if (entries.size()) {
                    chain.directory = true;
                    if (recursiveExploration(chains, visited, chain.startCluster, true, &entries)) {
                        foundNew = true;
                    }
                    chain.orphaned = true;
                }
            }
        }
    } while (foundNew);
}
        
bool FatChains::recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster, bool force, vector<FatEntry> *inputEntries)
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

    vector<FatEntry> entries;
    if (inputEntries != NULL) {
        entries = *inputEntries;
    } else {
        entries = system.getEntries(cluster);
    }
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = (*it);
        int cluster = entry.cluster;
        bool wasOrphaned = false;

        if (entry.isErased()) {
            continue;
        }

        // Search the cluster in the previously visited chains, if it
        // exists, mark it as non-orphaned
        if (chains.find(cluster)!=chains.end()) {
            if (entry.getFilename() != ".." && entry.getFilename() != ".") {
                if (chains[cluster].orphaned) {
                    wasOrphaned = true;

                    if (saveEntries) {
                        orphanEntries[myCluster].push_back(entry);
                        clusterToEntry[cluster] = entry;
                    }
                }
                // cout << "Unorphaning " << cluster << " from " << myCluster << endl;
                chains[cluster].orphaned = false;

                if (!entry.isDirectory()) {
                    chains[cluster].size = entry.size;
                }
            }
        } else {
            // Creating the entry
            if (force && entry.getFilename() != ".") {
                chains[cluster].startCluster = cluster;
                chains[cluster].endCluster = cluster;
                chains[cluster].directory = entry.isDirectory();
                chains[cluster].elements = 1;
                chains[cluster].orphaned = (entry.getFilename() == "..");

                // cout << "Discovering new directory " << entry.getFilename() << " " << cluster << " from " << myCluster << endl;

                if (!chains[cluster].orphaned) {
                    wasOrphaned = true;
                    if (saveEntries) {
                        orphanEntries[myCluster].push_back(entry);
                        clusterToEntry[cluster] = entry;
                    }
                }
                    
                foundNew = true;
            }
        }

        if (entry.isDirectory() && entry.getFilename() != "..") {
            recursiveExploration(chains, visited, cluster, force);
        }
        
        if (wasOrphaned) {
            chains[myCluster].elements += chains[cluster].elements;
            chains[myCluster].size += chains[cluster].size;
        }
    }

    return foundNew;
}

map<int, FatChain> FatChains::findChains()
{
    set<int> seen;
    map<int, FatChain> chains;

    for (int cluster=system.rootDirectory; cluster<system.totalClusters; cluster++) {
        // This cluster is new
        if (seen.find(cluster) == seen.end()) {
            // If this is an allocated cluster
            if (!system.freeCluster(cluster)) {
                set<int> localSeen;
                int next = cluster;
                int length = 1;
                // Walking through the chain
                while (true) {
                    int tmp = system.nextCluster(next);
                    if (tmp == FAT_LAST || !system.validCluster(tmp)) {
                        break;
                    }
                    if (localSeen.find(tmp) != localSeen.end()) {
                        fprintf(stderr, "! Loop\n");
                        break;
                    }
                    next = tmp;
                    length++;
                    seen.insert(next);
                    localSeen.insert(next);
                }

                FatChain chain;
                chain.startCluster = cluster;
                chain.endCluster = next;
                chain.length = length;

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
