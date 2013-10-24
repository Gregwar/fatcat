#include "FatChain.h"

FatChain::FatChain()
    : orphaned(true),
    directory(false),
    elements(1)
{
}

int FatChain::size()
{
    return 1+endCluster-startCluster;
}
