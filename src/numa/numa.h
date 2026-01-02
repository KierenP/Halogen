#pragma once

#include "utility/huge_pages.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

size_t get_numa_node(size_t thread_index);
size_t get_numa_node_count();

void bind_thread(size_t index);

void* numa_alloc_on_node(size_t size, size_t node);
void numa_free_node(void* ptr, size_t size);

// A container that holds one instance of T per NUMA node.
// Threads on the same NUMA node share the same instance.
// Memory is allocated on the appropriate NUMA node using numa_alloc_onnode.
// The get() method returns a raw pointer for fast, repeated access during search.
template <typename T>
class PerNumaAllocation
{
public:
    PerNumaAllocation(const PerNumaAllocation&) = delete;
    PerNumaAllocation& operator=(const PerNumaAllocation&) = delete;
    PerNumaAllocation(PerNumaAllocation&&) = delete;
    PerNumaAllocation& operator=(PerNumaAllocation&&) = delete;

    PerNumaAllocation()
    {
        auto numa_count = get_numa_node_count();
        while (data_.size() < numa_count)
        {
            size_t node = data_.size();
            void* mem = numa_alloc_on_node(sizeof(T), node);
            T* ptr = new (mem) T();
            data_.push_back(ptr);
        }
    }

    ~PerNumaAllocation()
    {
        for (size_t i = 0; i < data_.size(); ++i)
        {
            data_[i]->~T();
            numa_free_node(data_[i], sizeof(T));
        }
    }

    T* get(size_t thread_index)
    {
        return data_[get_numa_node(thread_index)];
    }

    const T* get(size_t thread_index) const
    {
        return data_[get_numa_node(thread_index)];
    }

private:
    std::vector<T*> data_;
};