#pragma once

#include <type_traits>
#include <utility>

// Check if the callable returns something convertable to a bool, if so we return that. If not, return true. This gives
// handlers flexibility to decide if they want to implement validation or not
template <typename T, typename... Args>
bool invoke_with_optional_validation(T&& t, Args&&... args)
{
    // lambda with bool return, used for validation
    if constexpr (std::is_invocable_r_v<bool, T, Args...>)
    {
        return std::forward<T>(t)(std::forward<Args>(args)...);
    }
    // lambda without validation, always return true
    else
    {
        std::forward<T>(t)(std::forward<Args>(args)...);
        return true;
    }
}