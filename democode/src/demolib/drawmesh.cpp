#include "drawmesh.h"


void AddToRegion(uint8_t *p, uint8_t N, int8_t offs)
{
    for(uint8_t i = 0; i < N; ++i)
        p[i] += offs;
}
