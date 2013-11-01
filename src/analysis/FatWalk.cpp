#include "FatWalk.h"
        
FatWalk::FatWalk(FatSystem &system)
    : FatModule(system),
    walkErased(false)
{
}

void FatWalk::walk(int cluster)
{
    FatEntry root;

    if (cluster == system.rootDirectory) {
        root = system.rootEntry();
    } else {
        root.cluster = cluster;
        root.attributes = FAT_ATTRIBUTES_DIR;
        root.longName = "/";
    }
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
            
            if (subname != "" && subname != "/") {
                subname += "/";
            }

            subname += entry.getFilename();

            if (entry.isDirectory()) {
                onDirectory(currentEntry, entry, subname);
                doWalk(visited, entry, subname);
            }
            
            onEntry(currentEntry, entry, subname);
        }
    }
}
        
void FatWalk::onEntry(FatEntry &parent, FatEntry &entry, string name)
{
}
        
void FatWalk::onDirectory(FatEntry &parent, FatEntry &entr, string name)
{
}
