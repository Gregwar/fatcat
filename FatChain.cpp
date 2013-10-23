#include "FatChain.h"

FatChain::FatChain()
    : orphaned(true)
{
}

int FatChain::size()
{
    return 1+endCluster-startCluster;
}
