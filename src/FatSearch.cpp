#include <iostream>
#include <string>
#include "FatSearch.h"

using namespace std;

FatSearch::FatSearch(FatSystem &system)
    : FatWalk(system),
    searchCluster(-1),
    found(0)
{
    walkErased = true;
}

void FatSearch::search(int cluster)
{
    cout << "Searching for an entry referencing " << cluster << " ..." << endl;
    searchCluster = cluster;
    walk();
    if (!found) {
        cout << "No entry found" << endl;
    }
}
        
void FatSearch::onEntry(FatEntry &parent, FatEntry &entry, string name)
{
    if (entry.cluster == searchCluster) {
        cout << "Found " << name << " in directory " << parent.getFilename() << " (" << parent.cluster << ")" << endl;
        vector<FatEntry> tmp;
        tmp.push_back(entry);
        system.list(tmp);
        found++;
    }
}
