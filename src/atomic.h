#pragma once

#include <atomic>

template <typename T>
class atomic_relaxed : public std::atomic<T>
{
    static_assert(std::atomic<T>::is_always_lock_free);

public:
    using std::atomic<T>::atomic;

    T operator=(T desired) noexcept
    {
        this->store(desired, std::memory_order_relaxed);
        return desired;
    }

    operator T() const noexcept
    {
        return this->load(std::memory_order_relaxed);
    }
};

template <typename T>
class atomic_rel_acq : public std::atomic<T>
{
    static_assert(std::atomic<T>::is_always_lock_free);

public:
    using std::atomic<T>::atomic;

    T operator=(T desired) noexcept
    {
        this->store(desired, std::memory_order_release);
        return desired;
    }

    operator T() const noexcept
    {
        return this->load(std::memory_order_acquire);
    }
};
