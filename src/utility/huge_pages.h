#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

#ifdef __linux__
#include <sys/mman.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

template <typename T>
using unique_ptr_huge_page = std::conditional_t<std::is_array_v<T>, //
    std::unique_ptr<T, std::function<void(std::remove_all_extents_t<T>*)>>, //
    std::unique_ptr<T, std::function<void(T*)>> //
    >;

template <typename T>
T* allocate_huge_page(std::size_t size)
{
#ifdef __linux__
    T* data = static_cast<T*>(std::aligned_alloc(2 * 1024 * 1024, size));
    if (!data)
    {
        std::cout << "Failed to allocate huge page" << std::endl;
        std::terminate();
    }
    madvise(data, size, MADV_HUGEPAGE);
    return data;
#elif defined(_WIN32)
    T* data = static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE));
    if (!data)
    {
        std::cout << "Failed to allocate huge page" << std::endl;
        std::terminate();
    }
    return data;
#else
    T* data = static_cast<T*>(std::aligned_alloc(alignof(T), size));
    if (!data)
    {
        std::cout << "Failed to allocate" << std::endl;
        std::terminate();
    }
    return data;
#endif
}

template <typename T>
void deallocate_huge_page(T* ptr)
{
#ifdef __linux__
    std::free(ptr);
#elif defined(_WIN32)
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    std::free(ptr);
#endif
}

template <class T, class... Args>
    requires(!std::is_array_v<T>)
unique_ptr_huge_page<T> make_unique_huge_page(Args&&... args)
{
    T* data = allocate_huge_page<T>(sizeof(T));
    std::construct_at(data, std::forward<Args>(args)...);
    return unique_ptr_huge_page<T>(data,
        [](T* ptr)
        {
            std::destroy_at(ptr);
            deallocate_huge_page(ptr);
        });
}

template <class T>
    requires std::is_unbounded_array_v<T>
unique_ptr_huge_page<T> make_unique_huge_page(std::size_t n)
{
    using E = std::remove_all_extents_t<T>;
    E* data = allocate_huge_page<E>(n * sizeof(E));
    std::uninitialized_value_construct_n(data, n);
    return unique_ptr_huge_page<T>(data,
        [n](E* ptr)
        {
            std::destroy_n(ptr, n);
            deallocate_huge_page(ptr);
        });
}

template <class T, class... Args>
    requires std::is_bounded_array_v<T>
void make_unique_huge_page(Args&&...) = delete;

template <class T>
    requires(!std::is_array_v<T>)
unique_ptr_huge_page<T> make_unique_for_overwrite_huge_page()
{
    T* data = allocate_huge_page<T>(sizeof(T));
    new (data) T;
    return unique_ptr_huge_page<T>(data,
        [](T* ptr)
        {
            std::destroy_at(ptr);
            deallocate_huge_page(ptr);
        });
}

template <class T>
    requires std::is_unbounded_array_v<T>
unique_ptr_huge_page<T> make_unique_for_overwrite_huge_page(std::size_t n)
{
    using E = std::remove_all_extents_t<T>;
    E* data = allocate_huge_page<E>(n * sizeof(E));
    std::uninitialized_default_construct_n(data, n);
    return unique_ptr_huge_page<T>(data,
        [n](E* ptr)
        {
            std::destroy_n(ptr, n);
            deallocate_huge_page(ptr);
        });
}

template <class T, class... Args>
    requires std::is_bounded_array_v<T>
void make_unique_for_overwrite_huge_page(Args&&...) = delete;