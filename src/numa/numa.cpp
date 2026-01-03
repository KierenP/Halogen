#include "numa.h"
#include <cstddef>

#ifdef TOURNAMENT_MODE
#include <cstdlib>
#include <iostream>
#include <numa.h>
#include <vector>

namespace
{

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

const std::vector<cpu_set_t>& get_mapping()
{
    static auto mapping = get_cpu_masks_per_numa_node();
    return mapping;
}

} // namespace

size_t get_numa_node(size_t thread_index)
{
    return thread_index % get_mapping().size();
}

size_t get_numa_node_count()
{
    return get_mapping().size();
}

void bind_thread(size_t index)
{
    size_t node = get_numa_node(index);
    pthread_t handle = pthread_self();
    pthread_setaffinity_np(handle, sizeof(cpu_set_t), &get_mapping()[node]);
}

void* numa_alloc_on_node(size_t size, size_t node)
{
    return numa_alloc_onnode(size, static_cast<int>(node));
}

void numa_free_node(void* ptr, size_t size)
{
    numa_free(ptr, size);
}

#else

#include <cstdlib>
#ifdef _WIN32
#include <malloc.h>
#endif

size_t get_numa_node(size_t)
{
    return 0;
}

size_t get_numa_node_count()
{
    return 1;
}

void bind_thread(size_t) { }

void* numa_alloc_on_node(size_t size, size_t)
{
    return allocate_huge_page<std::max_align_t>(size);
}

void numa_free_node(void* ptr, size_t)
{
    deallocate_huge_page(static_cast<std::max_align_t*>(ptr));
}

#endif