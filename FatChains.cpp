#include <map>
#include <stdio.h>
#include <set>
#include <vector>
#include <iostream>
#include "FatChains.h"
#include "FatChain.h"

using namespace std;
        
FatChains::FatChains(FatSystem &system)
    : FatModule(system)
{
}

void FatChains::findChains()
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
                chains[next] = chain;
            }

            seen.insert(cluster);
        }
    }

    map<int, FatChain>::iterator it;

    for (it=chains.begin(); it!=chains.end(); it++) {
        FatChain &chain = it->second;
        cout << chain.startCluster << " ~> " << chain.endCluster << endl;
    }
}
