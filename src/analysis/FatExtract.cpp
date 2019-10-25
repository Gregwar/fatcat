#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <mingw/utimes.h>
#else
#include <sys/time.h>
#endif
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
#ifdef _WIN32
    CreateDirectory(directory.c_str(), NULL);
#else
    mkdir(directory.c_str(), 0755);
#endif
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
        FILE *output = fopen(target.c_str(), "wb+");
        system.readFile(entry.cluster, entry.size, output, contiguous);
        fclose(output);

        time_t mtime = entry.changeDate.timestamp();
        if (mtime == (time_t)-1) {
            // Files on FAT can have dates up to 2107 year inclusive (which is
            // more than 2038), so it's theoretically possible that on a dated
            // system with 4-byte time_t some dates cannot be represented.
            // Too bad.
            cerr << "! Unable to set file timestamps for " << name
                 << ": value cannot be represented" << endl;
        } else {
            struct timeval times[2];
            // Modification time
            times[1].tv_sec = mtime;
            times[1].tv_usec = 0;
            // Access time
            times[0] = times[1];

            if (utimes(target.c_str(), times) != 0) {
                cerr << "! Unable to set file timestamps for " << name
                     << ": " << strerror(errno) << endl;
            }
        }
    }
}

void FatExtract::extract(unsigned int cluster, string directory, bool erased)
{
    walkErased = erased;
    targetDirectory = directory;
    walk(cluster);
}
