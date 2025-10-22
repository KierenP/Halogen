#pragma once

#include <cstddef>
#include <errhandlingapi.h>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <type_traits>
#include <winbase.h>

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
    // Use 2MB transparant huge pages
    T* data = static_cast<T*>(std::aligned_alloc(2 * 1024 * 1024, size));
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

    // In case of a failed huge page allocation, we print a error message only the first time
    static std::once_flag print_error_once_flag;
    auto print_error = [](std::string_view context, DWORD error)
    {
        LPSTR messageBuffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::cerr << "info string unable to use huge pages, falling back to regular allocation. " << context
                  << " failed: " << messageBuffer;
        LocalFree(messageBuffer);
    };

    auto fallback_to_regular_alloc = [&](std::string_view context, DWORD error)
    {
        std::call_once(print_error_once_flag, print_error, context, error);
        CloseHandle(hToken);
        return static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    };

    // Get the current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return fallback_to_regular_alloc("OpenProcessToken", GetLastError());
    }

    // Get the LUID for the SeLockMemoryPrivilege
    if (!LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &luid))
    {
        return fallback_to_regular_alloc("LookupPrivilegeValue", GetLastError());
    }

    // Enable the SeLockMemoryPrivilege
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
    {
        return fallback_to_regular_alloc("AdjustTokenPrivileges", GetLastError());
    }

    // Now allocate the huge page
    T* data = static_cast<T*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE));
    if (!data)
    {
        return fallback_to_regular_alloc("VirtualAlloc", GetLastError());
    }
    CloseHandle(hToken);
    return data;
#else
    return new T[size / sizeof(T)];
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
    delete[] ptr;
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