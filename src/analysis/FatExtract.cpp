#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "FatExtract.h"

using namespace std;

FatExtract::FatExtract(FatSystem &system)
    : FatWalk(system)
{
}
        
void FatExtract::onDirectory(FatEntry &parent, FatEntry &entry, string name)
{
    string directory = targetDirectory + "/" + name;
    mkdir(directory.c_str(), 0755);
}

void FatExtract::onEntry(FatEntry &parent, FatEntry &entry, string name)
{
    if (!entry.isDirectory()) {
        bool contiguous = false;

        if (entry.isErased()) {
            cerr << "! Trying to read deleted file, enabling contiguous mode" << endl;
            contiguous = true;
        }

        string target = targetDirectory + name;
        cout << "Extracting " << name << " to " << target << endl;
        FILE *output = fopen(target.c_str(), "w+");
        system.readFile(entry.cluster, entry.size, output, contiguous);
        fclose(output);
    }
}
        
void FatExtract::extract(unsigned int cluster, string directory, bool erased)
{
    walkErased = erased;
    targetDirectory = directory;
    walk(cluster);
}
