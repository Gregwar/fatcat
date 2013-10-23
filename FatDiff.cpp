#include <iostream>
#include <stdio.h>
#include <string>
#include "FatDiff.h"

using namespace std;
        
FatDiff::FatDiff(FatSystem &system)
    : FatModule(system)
{
}

bool FatDiff::compare()
{
    bool diff = false;
    bool mergeable = true;
    cout << "Comparing the FATs" << endl;

    for (int cluster=0; cluster<system.totalClusters; cluster++) {
        int A = system.nextCluster(cluster, 0);
        int B = system.nextCluster(cluster, 1);
        if (A != B) {
            diff = true;
            printf("[%08x] 1:%08x 2:%08x\n", cluster, A, B);

            if (A!=0 && B!=0) {
                mergeable = false;
            }
        }
    }
 
    cout << endl;

    if (diff) {
        cout << "FATs differs" << endl;
        if (mergeable) {
            cout << "It seems mergeable" << endl;
        } else {
            cout << "It doesn't seems mergeable" << endl;
        }
    } else {
        cout << "FATs are exactly equals" << endl;
    }

    return mergeable;
}

void FatDiff::merge()
{
    int merged = 0;
    cout << "Begining the merge..." << endl;
    system.enableWrite();

    for (int cluster=0; cluster<system.totalClusters; cluster++) {
        int A = system.nextCluster(cluster, 0);
        int B = system.nextCluster(cluster, 1);

        if (A != B && (A==0 || B==0)) {
            printf("Merging cluster %d\n", cluster);
            system.writeNextCluster(cluster, A+B);
            merged++;
        }
    }

    cout << "Merge complete, " << merged << " clusters merged" << endl;
}
