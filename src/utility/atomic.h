#pragma once

#include <atomic>

template <typename T>
class AtomicRelaxed : public std::atomic<T>
{
    static_assert(std::atomic<T>::is_always_lock_free);

public:
    using std::atomic<T>::atomic;

    auto& operator=(T desired) noexcept
    {
        this->store(desired, std::memory_order_relaxed);
        return *this;
    }

    operator T() const noexcept
    {
        return this->load(std::memory_order_relaxed);
    }
};

template <typename T>
class AtomicRelAcq : public std::atomic<T>
{
    static_assert(std::atomic<T>::is_always_lock_free);

public:
    using std::atomic<T>::atomic;

    auto& operator=(T desired) noexcept
    {
        this->store(desired, std::memory_order_release);
        return *this;
    }

    operator T() const noexcept
    {
        return this->load(std::memory_order_acquire);
    }
};
