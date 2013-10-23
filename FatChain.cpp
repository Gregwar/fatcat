#include "FatChain.h"

FatChain::FatChain()
    : orphaned(true),
    directory(false)
{
}

int FatChain::size()
{
    return 1+endCluster-startCluster;
}
