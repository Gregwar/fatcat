#include <iostream>
#include <string>
#include "FatFix.h"

using namespace std;

FatFix::FatFix(FatSystem &system)
    : FatWalk(system)
{
}

void FatFix::fix()
{
    cout << "Searching for damaged files & directories" << endl;
    system.enableWrite();
    walk();
}
        
void FatFix::onEntry(FatEntry &parent, FatEntry &entry, string name)
{
    int cluster = entry.cluster;

    if (system.freeCluster(cluster)) {
        if (entry.isDirectory()) {
            int size;
            system.getEntries(entry.cluster, &size);
            cout << "Directory " << name << " (" << cluster << ") seems broken, trying to repair FAT..." << endl;
            fixChain(cluster, size);
        } else {
            cout << "File " << name << "/" << entry.getFilename() << " seems broken" << endl;
            fixChain(entry.cluster, entry.size/system.bytesPerCluster+1);
        }
    }
}
        
void FatFix::fixChain(int cluster, int size)
{
    bool fixIt = true;

    if (!size) {
        cout << "Size is zero, not fixing" << endl;
        return;
    }

    cout << "Fixing the FAT (" << size << " clusters)" << endl;

    for (int i=0; i<size; i++) {
        if (!system.freeCluster(cluster+i)) {
            fixIt = false;
        }
    }

    if (fixIt) {
        cout << "Clusters are free, fixing" << endl;
        for (int i=0; i<size; i++) {
            if (system.freeCluster(cluster+i)) {
                if (i == size-1) {
                    system.writeNextCluster(cluster+i, FAT_LAST, 0);
                    system.writeNextCluster(cluster+i, FAT_LAST, 1);
                } else {
                    system.writeNextCluster(cluster+i, cluster+i+1, 0);
                    system.writeNextCluster(cluster+i, cluster+i+1, 1);
                }
            }
        }
    } else {
        cout << "There is allocated clusters in the list, not fixing" << endl;
    }
}
