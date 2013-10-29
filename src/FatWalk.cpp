#include "FatWalk.h"
        
FatWalk::FatWalk(FatSystem &system)
    : FatModule(system),
    walkErased(false)
{
}

void FatWalk::walk()
{
    FatEntry root = system.rootEntry();
    set<int> visited;
    onEntry(root, root, "/");
    doWalk(visited, root, "/");
}

void FatWalk::doWalk(set<int> &visited, FatEntry &currentEntry, string name)
{
    int cluster = currentEntry.cluster;

    if (visited.find(cluster) != visited.end()) {
        return;
    }

    visited.insert(cluster);

    vector<FatEntry> entries = system.getEntries(cluster);
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = *it;

        if ((!walkErased) && entry.isErased()) {
            continue;
        }

        if (entry.getFilename() != "." && entry.getFilename() != "..") {
            string subname = name;
            
            if (subname != "") {
                subname += "/";
            }

            subname += entry.getFilename();

            onEntry(currentEntry, entry, subname);

            if (entry.isDirectory()) {
                doWalk(visited, entry, subname);
            }
        }
    }
}
        
void FatWalk::onEntry(FatEntry &parent, FatEntry &entry, string name)
{
}
