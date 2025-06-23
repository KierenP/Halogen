#pragma once

#include "uci/validate_callback.h"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

namespace UCI
{

constexpr inline char uci_delimiter = ' ';

// Returns true if we are at the end of the command
struct EndCommand
{
    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&...)
    {
        return command.size() == 0;
    }
};

// Invokes a callback. Does nothing with the command
template <typename T>
struct Invoke
{
    Invoke(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view&, Ctx&&... ctx)
    {
        return invoke_with_optional_validation(callback_, std::forward<Ctx>(ctx)...);
    }

    T callback_;
};

// Invokes a callback, converting the token to a int
template <typename T>
struct ToInt
{
    ToInt(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& token, Ctx&&... ctx)
    {
        int result {};
        auto [ptr, _] = std::from_chars(token.data(), token.end(), result);

        if (ptr != token.end())
        {
            return false;
        }

        return invoke_with_optional_validation(callback_, result, std::forward<Ctx>(ctx)...);
    }

    T callback_;
};

// Invokes a callback, converting the token to a float
template <typename T>
struct ToFloat
{
    ToFloat(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& token, Ctx&&... ctx)
    {
        // from_chars(float) only supported in GCC 11+
        float result = std::stof(std::string(token));
        return invoke_with_optional_validation(callback_, result, std::forward<Ctx>(ctx)...);
    }

    T callback_;
};

// Invokes a callback, converting the token to a boolean
template <typename T>
struct ToBool
{
    ToBool(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& token, Ctx&&... ctx)
    {
        if (token == "true")
        {
            return invoke_with_optional_validation(callback_, true, std::forward<Ctx>(ctx)...);
        }
        else if (token == "false")
        {
            return invoke_with_optional_validation(callback_, false, std::forward<Ctx>(ctx)...);
        }
        else
        {
            return false;
        }
    }

    T callback_;
};

// Reads the next token and passes to the callback
template <typename T>
struct NextToken
{
    NextToken(T&& callback)
        : callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        auto pos = command.find(uci_delimiter);

        if (pos == command.npos)
        {
            // process all remaining
            auto token = command;
            command = "";
            return invoke_with_optional_validation(callback_, token, std::forward<Ctx>(ctx)...);
        }
        else
        {
            // process up to the delimiter, and then update command to be the remaining tokens
            auto token = command.substr(0, pos);
            command = command.substr(std::min(command.size(), pos + 1));
            return invoke_with_optional_validation(callback_, token, std::forward<Ctx>(ctx)...);
        }
    }

    T callback_;
};

// Reads until the first occurance of the token and passes the substr to the callback
template <typename T>
struct TokensUntil
{
    TokensUntil(std::string_view delimiter, T&& callback)
        : delimiter_(delimiter)
        , callback_(std::move(callback))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        // for a command like 'position fen A B C moves X Y Z' we are trying to consume 'A B C' for example
        auto pos = command.find(delimiter_);

        if (pos == command.npos)
        {
            // process all remaining
            auto token = command;
            command = "";
            return invoke_with_optional_validation(callback_, token, std::forward<Ctx>(ctx)...);
        }
        else
        {
            auto token = command.substr(0, std::max(0, (int)pos - 1));
            command = command.substr(std::min(command.size(), pos + 1 + delimiter_.size()));
            return invoke_with_optional_validation(callback_, token, std::forward<Ctx>(ctx)...);
        }
    }

    std::string_view delimiter_;
    T callback_;
};

// Consumes a token and passes the remaining command to the handler
template <typename T>
struct Consume
{
    Consume(std::string_view token, T&& handler)
        : handler_(std::move(handler))
        , token_(token)
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        // try to match the token as the prefix to command. To avoid matching 'uci' when the token is actually
        // 'ucinewgame', we also need to either follow with the delimiter or consume the whole token
        if (command.substr(0, token_.size()) == token_
            && (command.size() == token_.size() || command[token_.size()] == uci_delimiter))
        {
            command = command.substr(std::min(command.size(), token_.size() + 1));
            return invoke_with_optional_validation(handler_, command, std::forward<Ctx>(ctx)...);
        }
        else
        {
            return false;
        }
    }

    T handler_;
    std::string_view token_;
};

// Executes a series of handlers in order
template <typename... T>
struct Sequence
{
    Sequence(T&&... handlers)
        : handlers_(std::move(handlers)...)
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        return handle<0>(command, std::forward<Ctx>(ctx)...);
    }

    template <size_t I, typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        if constexpr (I >= sizeof...(T))
        {
            return true;
        }
        else if (!invoke_with_optional_validation(std::get<I>(handlers_), command, std::forward<Ctx>(ctx)...))
        {
            return false;
        }
        else
        {
            return handle<I + 1>(command, std::forward<Ctx>(ctx)...);
        }
    }

    std::tuple<T...> handlers_;
};

// Tries each handler until one returns true
template <typename... T>
struct OneOf
{
    OneOf(T&&... handlers)
        : handlers_(std::move(handlers)...)
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        return handle<0>(command, std::forward<Ctx>(ctx)...);
    }

    template <size_t I, typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        if constexpr (I >= sizeof...(T))
        {
            return false;
        }
        else if (invoke_with_optional_validation(std::get<I>(handlers_), command, std::forward<Ctx>(ctx)...))
        {
            return true;
        }
        else
        {
            return handle<I + 1>(command, std::forward<Ctx>(ctx)...);
        }
    }

    std::tuple<T...> handlers_;
};

// Repeatedly calls the handler until the command is empty
template <typename T>
struct Repeat
{
    Repeat(T&& handler)
        : handler_(std::move(handler))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        while (!command.empty())
        {
            if (!invoke_with_optional_validation(handler_, command, std::forward<Ctx>(ctx)...))
            {
                return false;
            }
        }

        return true;
    }

    T handler_;
};

// Wraps the nested handler in a context, useful for storing state between handlers. with_context can be used recursivly
// to wrap multiple contexts
template <typename new_Ctx, typename T>
struct WithContext
{
    WithContext(new_Ctx&& ctx, T&& handler)
        : context_(std::move(ctx))
        , handler_(std::move(handler))
    {
    }

    template <typename... Ctx>
    bool operator()(std::string_view& command, Ctx&&... ctx)
    {
        return invoke_with_optional_validation(handler_, command, std::forward<Ctx>(ctx)..., context_);
    }

    new_Ctx context_;
    T handler_;
};

}