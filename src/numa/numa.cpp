#include "numa.h"
#include "network/arch.hpp"
#include <cstdlib>
#include <vector>

#ifndef TOURNAMENT_MODE
#include <iostream>
#include <numa.h>

[[maybe_unused]] const auto verify_numa = []
{
    if (numa_available() < 0)
    {
        std::cerr << "NUMA is not available on this system" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return true;
}();

std::vector<cpu_set_t> get_cpu_masks_per_numa_node()
{
    const int max_node = numa_max_node();

    std::vector<cpu_set_t> node_cpu_masks;

    for (int node = 0; node <= max_node; ++node)
    {
        struct bitmask* cpumask = numa_allocate_cpumask();

        if (numa_node_to_cpus(node, cpumask) != 0)
        {
            perror("numa_node_to_cpus");
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
    if (pthread_setaffinity_np(handle, sizeof(cpu_set_t), &mapping[node]) != 0)
    {
        perror("pthread_setaffinity_np");
        std::exit(EXIT_FAILURE);
    }
}

int get_current_numa_node()
{
    int cpu = sched_getcpu();
    if (cpu < 0)
    {
        perror("sched_getcpu");
        std::exit(EXIT_FAILURE);
    }

    int node = numa_node_of_cpu(cpu);
    if (node < 0)
    {
        perror("numa_node_of_cpu");
        std::exit(EXIT_FAILURE);
    }

    return node;
}

template <typename T>
NumaLocalAllocationManager<T>::NumaLocalAllocationManager(const T* source_data)
    : source_data_(source_data)
{
    int max_node = numa_max_node();
    per_node_replicas_.resize(max_node + 1, nullptr);

    for (int node = 0; node <= max_node; ++node)
    {
        // Allocate memory on the target NUMA node
        void* replica = numa_alloc_onnode(sizeof(T), node);
        if (!replica)
        {
            std::cerr << "Failed to allocate on NUMA node " << node << "\n";
            std::exit(EXIT_FAILURE);
        }

        // initialize using placement new (copy constructor)
        per_node_replicas_[node] = new (replica) T(*source_data_);

        if (per_node_replicas_[node] != replica)
        {
            std::cout << "ERROR" << std::endl;
        }
    }
}

template <typename T>
NumaLocalAllocationManager<T>::~NumaLocalAllocationManager()
{
    for (size_t node = 0; node < per_node_replicas_.size(); ++node)
    {
        if (per_node_replicas_[node])
        {
            per_node_replicas_[node]->~T();
            numa_free(per_node_replicas_[node], sizeof(T));
        }
    }
}

template <typename T>
const T& NumaLocalAllocationManager<T>::get_numa_local_data() const
{
    int node = get_current_numa_node();
    if (node >= 0 && node < (int)per_node_replicas_.size())
    {
        return *per_node_replicas_[node];
    }
    std::cerr << "Failed to get_numa_local_data on node " << node << "\n";
    std::exit(EXIT_FAILURE);
}

#else

void bind_thread(size_t) { }

template <typename T>
NumaLocalAllocationManager<T>::NumaLocalAllocationManager(const T* source_data)
    : source_data_(source_data)
{
}

template <typename T>
NumaLocalAllocationManager<T>::~NumaLocalAllocationManager()
{
}

template <typename T>
const T& NumaLocalAllocationManager<T>::get_numa_local_data() const
{
    return *source_data_;
}

#endif

// explicit template instantiations
template class NumaLocalAllocationManager<NetworkWeights>;