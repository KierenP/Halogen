#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <type_traits>

#ifdef __linux__
#include <sys/mman.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

template <typename T>
T* allocate_huge_page(std::size_t size)
{
#ifdef __linux__
    // Use 2MB transparent huge pages
    constexpr static auto huge_page_size = 2 * 1024 * 1024;
    size = ((size + huge_page_size - 1) / huge_page_size) * huge_page_size;
    T* data = static_cast<T*>(std::aligned_alloc(huge_page_size, size));
    madvise(data, size, MADV_HUGEPAGE);
    return data;
#elif defined(_WIN32)
    // 1. Obtain the SeLockMemoryPrivilege privilege by calling the AdjustTokenPrivileges function. For more
    //    information, see Assigning Privileges to an Account and Changing Privileges in a Token.
    // 2. Retrieve the minimum large-page size by calling the GetLargePageMinimum function.
    // 3. Include the MEM_LARGE_PAGES value when calling the VirtualAlloc function. The size and alignment must be a
    //    multiple of the large-page minimum.
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    // Get the current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    // Get the LUID for the SeLockMemoryPrivilege
    if (!LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &luid))
    {
        CloseHandle(hToken);
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    // Enable the SeLockMemoryPrivilege
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr))
    {
        CloseHandle(hToken);
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    // Even if AdjustTokenPrivileges returns success, must check GetLastError for ERROR_NOT_ALL_ASSIGNED
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        CloseHandle(hToken);
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    // Now allocate the huge page
    SIZE_T largePageMinimum = GetLargePageMinimum();
    SIZE_T roundedSize = ((size + largePageMinimum - 1) / largePageMinimum) * largePageMinimum;
    T* data = static_cast<T*>(
        VirtualAlloc(nullptr, roundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE));
    if (!data)
    {
        CloseHandle(hToken);
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }
    CloseHandle(hToken);
    return data;
#else
    size = ((size + alignof(T) - 1) / alignof(T)) * alignof(T);
    return static_cast<T*>(std::aligned_alloc(alignof(T), size));
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

template <typename T>
struct HugePageDeleter
{
    void operator()(T* ptr) const
    {
        std::destroy_at(ptr);
        deallocate_huge_page(ptr);
    }
};

template <typename T>
struct HugePageDeleter<T[]>
{
    std::size_t count = 0;

    void operator()(T* ptr) const
    {
        std::destroy_n(ptr, count);
        deallocate_huge_page(ptr);
    }
};

template <typename T>
using unique_ptr_huge_page = std::unique_ptr<T, HugePageDeleter<T>>;

template <class T, class... Args>
    requires(!std::is_array_v<T>)
unique_ptr_huge_page<T> make_unique_huge_page(Args&&... args)
{
    T* data = allocate_huge_page<T>(sizeof(T));
    std::construct_at(data, std::forward<Args>(args)...);
    return unique_ptr_huge_page<T>(data, HugePageDeleter<T> {});
}

template <class T>
    requires std::is_unbounded_array_v<T>
unique_ptr_huge_page<T> make_unique_huge_page(std::size_t n)
{
    using E = std::remove_all_extents_t<T>;
    E* data = allocate_huge_page<E>(n * sizeof(E));
    std::uninitialized_value_construct_n(data, n);
    return unique_ptr_huge_page<T>(data, HugePageDeleter<T> { n });
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
    return unique_ptr_huge_page<T>(data, HugePageDeleter<T> {});
}

template <class T>
    requires std::is_unbounded_array_v<T>
unique_ptr_huge_page<T> make_unique_for_overwrite_huge_page(std::size_t n)
{
    using E = std::remove_all_extents_t<T>;
    E* data = allocate_huge_page<E>(n * sizeof(E));
    std::uninitialized_default_construct_n(data, n);
    return unique_ptr_huge_page<T>(data, HugePageDeleter<T> { n });
}

template <class T, class... Args>
    requires std::is_bounded_array_v<T>
void make_unique_for_overwrite_huge_page(Args&&...) = delete;