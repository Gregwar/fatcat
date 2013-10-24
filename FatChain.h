#ifndef _FATCAT_FATCHAIN_H
#define _FATCAT_FATCHAIN_H

class FatChain
{
    public:
        FatChain();

        int size();

        int startCluster;
        int endCluster;
        bool orphaned;
        bool directory;
        int elements;
};

#endif // _FATCAT_FATCHAIN_H
