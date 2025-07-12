#include "numa.h"
#include <cstdlib>

#ifdef TOURNAMENT_MODE
#include <iostream>
#include <numa.h>

void bind_search_thread(int core)
{
    int node = numa_node_of_cpu(core);

    if (node < 0)
    {
        std::cerr << "Failed to get NUMA node for core " << core << std::endl;
        std::exit(EXIT_FAILURE);
    }

    struct bitmask* mask = numa_allocate_nodemask();
    numa_bitmask_setbit(mask, node);
    numa_bind(mask);
}

#else
void bind_search_thread(int) { }
#endif