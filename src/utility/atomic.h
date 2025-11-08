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

template <typename T>
class SingleWriterAtomicCounter
{
    static_assert(std::atomic<T>::is_always_lock_free);
    std::atomic<T> atomic;

public:
    SingleWriterAtomicCounter() noexcept = default;
    SingleWriterAtomicCounter(T desired)
        : atomic(desired)
    {
    }

    auto& operator=(T desired) noexcept
    {
        atomic.store(desired, std::memory_order_relaxed);
        return *this;
    }

    operator T() const noexcept
    {
        return atomic.load(std::memory_order_relaxed);
    }

    void inc() noexcept
    {
        // fetch_add produces a LOCK ADD instruction to synchronize multiple threads incrementing the same counter,
        // which has a 18 cycle latency on SkyLake. Because we have only one writer, we can use an unsynchronized
        // load/store which compiles down to just a `inc` instruction.
        atomic.store(atomic.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
};