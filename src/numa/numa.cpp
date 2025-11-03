#include "numa.h"

#ifdef TOURNAMENT_MODE
#include <cstdlib>
#include <iostream>
#include <numa.h>
#include <vector>

std::vector<cpu_set_t> get_cpu_masks_per_numa_node()
{
    if (numa_available() == -1)
    {
        std::cerr << "NUMA is not available on this system" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const int max_node = numa_max_node();

    std::vector<cpu_set_t> node_cpu_masks;

    for (int node = 0; node <= max_node; ++node)
    {
        struct bitmask* cpumask = numa_allocate_cpumask();

        if (numa_node_to_cpus(node, cpumask) != 0)
        {
            std::cerr << "Failed to get CPUs for NUMA node: " << node << std::endl;
            std::exit(EXIT_FAILURE);
        }

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        for (size_t cpu = 0; cpu < cpumask->size; ++cpu)
        {
            if (numa_bitmask_isbitset(cpumask, cpu))
            {
                CPU_SET(cpu, &cpuset);
            }
        }

        numa_free_cpumask(cpumask);
        node_cpu_masks.push_back(cpuset);
    }

    return node_cpu_masks;
}

void bind_thread(size_t index)
{
    static auto mapping = get_cpu_masks_per_numa_node();
    size_t node = index % mapping.size();
    pthread_t handle = pthread_self();
    pthread_setaffinity_np(handle, sizeof(cpu_set_t), &mapping[node]);
}

#else
void bind_thread(size_t) { }
#endif