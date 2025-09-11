#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <vector>

void bind_thread(size_t index);

template <typename T>
class NumaLocalAllocationManager
{
    static_assert(std::is_trivially_copyable_v<T>, "NumaLocalAllocationManager requires trivially copyable types");

public:
    NumaLocalAllocationManager(const T* source_data);
    ~NumaLocalAllocationManager();

    [[nodiscard]] const T& get_numa_local_data() const;

private:
    const T* source_data_;
    std::vector<const T*> per_node_replicas_;
};