#include <map>
#include <list>
#include <stdio.h>
#include <set>
#include <vector>
#include <iostream>

#include <FatUtils.h>
#include <core/FatEntry.h>
#include "FatChains.h"
#include "FatChain.h"

using namespace std;

bool compare_size(FatChain &A, FatChain &B)
{
    return A.size > B.size;
}
        
FatChains::FatChains(FatSystem &system)
    : FatModule(system),
    saveEntries(false),
    exploreDamaged(false)
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
        cout << mit->first << "~>" << mit->second.endCluster << endl;
    } 
    */

    cout << "Found " << chains.size() << " chains" << endl;
    cout << endl;

    cout << "Running the recursive differential analysis..." << endl;
    set<int> visited;
    saveEntries = false;
    exploreDamaged = false;
    recursiveExploration(chains, visited, system.rootDirectory);
    visited.insert(0);
    cout << endl;

    cout << "Having a look at the chains..." << endl;
    saveEntries = true;
    exploreChains(chains, visited);
   
    // Getting orphaned elements from the map
    list<FatChain> orphanedChains = getOrphaned(chains);
    displayOrphaned(orphanedChains);
}

void FatChains::exploreChains(map<int, FatChain> &chains, set<int> &visited)
{
    map<int, FatChain>::iterator it;
    bool foundNew;
    exploreDamaged = true;
    do {
        foundNew = false;
        for (it=chains.begin(); it!=chains.end(); it++) {
            FatChain &chain = it->second;

            if (chain.orphaned) {
                // cout << "Trying " << chain.startCluster << endl;
                vector<FatEntry> entries = system.getEntries(chain.startCluster);
                if (entries.size()) {
                    chain.directory = true;
                    if (recursiveExploration(chains, visited, chain.startCluster, &entries)) {
                        foundNew = true;
                    }
                    chain.orphaned = true;
                }
            }
        }
    } while (foundNew);
}
    
/**
 * Explore a directory
 */
bool FatChains::recursiveExploration(map<int, FatChain> &chains, set<int> &visited, int cluster, vector<FatEntry> *inputEntries)
{
    if (visited.find(cluster) != visited.end()) {
        return false;
    }

    if (!exploreDamaged && system.nextCluster(cluster) == 0) {
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
            if (exploreDamaged && entry.getFilename() != ".") {
                chains[cluster].startCluster = cluster;
                chains[cluster].endCluster = cluster;
                chains[cluster].directory = entry.isDirectory();
                chains[cluster].elements = 1;
                chains[cluster].orphaned = (entry.getFilename() == "..");

                // cout << "Discovering new entry " << entry.getFilename() << endl;

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
            recursiveExploration(chains, visited, cluster);
        }
        
        if (wasOrphaned) {
            chains[myCluster].elements += chains[cluster].elements;
            chains[myCluster].size += chains[cluster].size;
        }
    }

    return foundNew;
}

/**
 * Find all cluster chains in the FAT
 */
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
    
list<FatChain> FatChains::getOrphaned(map<int, FatChain> &chains)
{
    list<FatChain> orphanedChains;
    map<int, FatChain>::iterator it;

    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;

        if (chain.startCluster < 2) {
            chain.orphaned = false;
        }

        if (chain.orphaned) {
            if (!chain.directory) {
                chain.size = chain.length*system.bytesPerCluster;
            }
            orphanedChains.push_back(chain);
        }
    }

    return orphanedChains;
}

void FatChains::displayOrphaned(list<FatChain> orphanedChains)
{
    if (orphanedChains.size()) {
        orphanedChains.sort(compare_size);
        unsigned long long totalSize = 0;
        cout << "There is " << orphanedChains.size() << " orphaned elements:" << endl;
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

int FatChains::chainSize(int cluster, list<Segment>& segments)
{
    set<int> visited;
    int length = 0;
    bool stop;
    int start = cluster;

    do {
        stop = true;
        int currentCluster = cluster;
        visited.insert(cluster);
        length++;
        cluster = system.nextCluster(cluster);
        if (cluster==FAT_LAST) {
            segments.push_back(pair(start, currentCluster));
        } else if (system.validCluster(cluster)) {
            if (currentCluster+1 != cluster) {
                segments.push_back(pair(start, currentCluster));
                start = cluster;
            }
            if (visited.find(cluster) != visited.end()) {
                cerr << "! Loop detected, " << currentCluster << " points to " << cluster << " that I already met" << endl;
            } else {
                stop = false;
            }
        }
    } while (!stop);

    return length;
}
