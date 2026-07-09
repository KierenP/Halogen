#pragma once

#include "uci/validate_callback.h"

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace UCI
{

constexpr inline char uci_delimiter = ' ';

// A parser consumes some prefix of the command and returns the unconsumed remainder on success, or
// std::nullopt on failure. Because the input is taken by value, a failed alternative leaves the
// caller's view untouched, so OneOf can backtrack simply by retrying with the original command.
//
// Leaf handlers (the user-supplied lambdas, and the ToInt/ToFloat/ToBool token converters) instead
// return void or bool via invoke_with_optional_validation; the combinator that owns them adapts
// that into the parser contract.

// Succeeds only if the whole command has been consumed
struct EndCommand
{
    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&...) const
    {
        if (command.empty())
        {
            return command;
        }

        return std::nullopt;
    }
};

// Invokes a callback. Consumes nothing from the command
template <typename T>
struct Invoke
{
    constexpr Invoke(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        if (!invoke_with_optional_validation(callback_, ctx...))
        {
            return std::nullopt;
        }

        return command;
    }

    T callback_;
};

// Converts a token to an int and passes it to the callback. Used as the leaf of a NextToken
template <typename T>
struct ToInt
{
    constexpr ToInt(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view token, Ctx&... ctx) const
    {
        int result {};
        auto [ptr, _] = std::from_chars(token.data(), token.end(), result);

        if (ptr != token.end())
        {
            return false;
        }

        return invoke_with_optional_validation(callback_, result, ctx...);
    }

    T callback_;
};

// Converts a token to a float and passes it to the callback. Used as the leaf of a NextToken
template <typename T>
struct ToFloat
{
    constexpr ToFloat(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view token, Ctx&... ctx) const
    {
        // from_chars(float) only supported in GCC 11+
        float result = std::stof(std::string(token));
        return invoke_with_optional_validation(callback_, result, ctx...);
    }

    T callback_;
};

// Converts a token to a boolean and passes it to the callback. Used as the leaf of a NextToken
template <typename T>
struct ToBool
{
    constexpr ToBool(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view token, Ctx&... ctx) const
    {
        if (token == "true")
        {
            return invoke_with_optional_validation(callback_, true, ctx...);
        }
        else if (token == "false")
        {
            return invoke_with_optional_validation(callback_, false, ctx...);
        }
        else
        {
            return false;
        }
    }

    T callback_;
};

// Reads the next token and passes it to the callback, consuming that token
template <typename T>
struct NextToken
{
    constexpr NextToken(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        auto pos = command.find(uci_delimiter);
        auto token = (pos == command.npos) ? command : command.substr(0, pos);
        auto rest = (pos == command.npos) ? std::string_view {} : command.substr(pos + 1);

        if (!invoke_with_optional_validation(callback_, token, ctx...))
        {
            return std::nullopt;
        }

        return rest;
    }

    T callback_;
};

// Reads until the first occurance of the delimiter and passes the preceding substr to the callback,
// consuming the substr and the delimiter
template <typename T>
struct TokensUntil
{
    constexpr TokensUntil(std::string_view delimiter, T&& callback)
        : delimiter_(delimiter)
        , callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        // for a command like 'position fen A B C moves X Y Z' we are trying to consume 'A B C' for example
        auto pos = command.find(delimiter_);
        auto token = (pos == command.npos) ? command : command.substr(0, std::max(0, (int)pos - 1));
        auto rest = (pos == command.npos) ? std::string_view {}
                                          : command.substr(std::min(command.size(), pos + 1 + delimiter_.size()));

        if (!invoke_with_optional_validation(callback_, token, ctx...))
        {
            return std::nullopt;
        }

        return rest;
    }

    std::string_view delimiter_;
    T callback_;
};

// Consumes a leading token and passes the remaining command to the nested parser
template <typename T>
struct Consume
{
    constexpr Consume(std::string_view token, T&& handler)
        : handler_(std::move(handler))
        , token_(token)
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        // try to match the token as the prefix to command. To avoid matching 'uci' when the token is actually
        // 'ucinewgame', we also need to either follow with the delimiter or consume the whole token
        if (command.substr(0, token_.size()) == token_
            && (command.size() == token_.size() || command[token_.size()] == uci_delimiter))
        {
            return handler_(command.substr(std::min(command.size(), token_.size() + 1)), ctx...);
        }

        return std::nullopt;
    }

    T handler_;
    std::string_view token_;
};

// Executes a series of parsers in order, threading the remainder of each into the next
template <typename... T>
struct Sequence
{
    constexpr Sequence(T&&... handlers)
        : handlers_(std::move(handlers)...)
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        return handle<0>(command, ctx...);
    }

    template <size_t I, typename... Ctx>
    constexpr std::optional<std::string_view> handle(std::string_view command, Ctx&... ctx) const
    {
        if constexpr (I >= sizeof...(T))
        {
            return command;
        }
        else if (auto rest = std::get<I>(handlers_)(command, ctx...))
        {
            return handle<I + 1>(*rest, ctx...);
        }
        else
        {
            return std::nullopt;
        }
    }

    std::tuple<T...> handlers_;
};

// Tries each parser against the (unconsumed) command until one succeeds
template <typename... T>
struct OneOf
{
    constexpr OneOf(T&&... handlers)
        : handlers_(std::move(handlers)...)
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        return handle<0>(command, ctx...);
    }

    template <size_t I, typename... Ctx>
    constexpr std::optional<std::string_view> handle(std::string_view command, Ctx&... ctx) const
    {
        if constexpr (I >= sizeof...(T))
        {
            return std::nullopt;
        }
        // each alternative gets the original command, so a failed branch backtracks for free
        else if (auto rest = std::get<I>(handlers_)(command, ctx...))
        {
            return rest;
        }
        else
        {
            return handle<I + 1>(command, ctx...);
        }
    }

    std::tuple<T...> handlers_;
};

// Repeatedly applies the parser until the command is empty
template <typename T>
struct Repeat
{
    constexpr Repeat(T&& handler)
        : handler_(std::move(handler))
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        while (!command.empty())
        {
            auto rest = handler_(command, ctx...);

            if (!rest)
            {
                return std::nullopt;
            }

            // Guard against a handler that succeeds without consuming anything, which would loop forever
            if (rest->size() >= command.size())
            {
                return std::nullopt;
            }

            command = *rest;
        }

        return command;
    }

    T handler_;
};

// Wraps the nested parser in a context, useful for storing state between handlers. A fresh copy of
// the context is made per invocation so the parser can be reused. WithContext can be nested to wrap
// multiple contexts.
template <typename new_Ctx, typename T>
struct WithContext
{
    constexpr WithContext(new_Ctx&& ctx, T&& handler)
        : context_(std::move(ctx))
        , handler_(std::move(handler))
    {
    }

    template <typename... Ctx>
    constexpr std::optional<std::string_view> operator()(std::string_view command, Ctx&... ctx) const
    {
        new_Ctx local = context_;
        return handler_(command, ctx..., local);
    }

    new_Ctx context_;
    T handler_;
};

}
