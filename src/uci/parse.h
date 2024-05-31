#pragma once

#include <string_view>

constexpr inline char uci_delimiter = ' ';

// Returns true if we are at the end of the command
struct end_command
{
    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&...)
    {
        return command.size() == 0;
    }
};

// Invokes a callback. Does nothing with the command
template <typename T>
struct invoke
{
    invoke(T&& callback)
        : callback_(std::forward<T>(callback))
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view&, Ctx&&... ctx)
    {
        callback_(std::forward<Ctx>(ctx)...);
        return true;
    }

    T callback_;
};

// Reads the next token and passes to the callback
template <typename T>
struct process
{
    process(T&& callback)
        : callback_(std::forward<T>(callback))
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        auto pos = command.find(uci_delimiter);

        if (pos == command.npos)
        {
            // process all remaining
            auto token = command;
            command = "";
            callback_(token, std::forward<Ctx>(ctx)...);
        }
        else
        {
            // process up to the delimiter, and then update command to be the remaining tokens
            auto token = command.substr(0, pos);
            command = command.substr(std::min(command.size(), pos + 1));
            callback_(token, std::forward<Ctx>(ctx)...);
        }

        return true;
    }

    T callback_;
};

// Reads until the first occurance of the token and passes the substr to the callback
template <typename T>
struct process_until
{
    process_until(std::string_view delimiter, T&& callback)
        : delimiter_(delimiter)
        , callback_(std::forward<T>(callback))
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        auto pos = command.find(delimiter_);

        if (pos == command.npos)
        {
            // process all remaining
            auto token = command;
            command = "";
            callback_(token, std::forward<Ctx>(ctx)...);
        }
        else
        {
            auto token = command.substr(0, std::max(0, (int)pos - 1));
            command = command.substr(std::min(command.size(), pos + 1 + delimiter_.size()));
            callback_(token, std::forward<Ctx>(ctx)...);
        }

        return true;
    }

    std::string_view delimiter_;
    T callback_;
};

// Consumes a token and passes the remaining command to the handler
template <typename T>
struct consume
{
    consume(std::string_view token, T&& handler)
        : handler_(std::forward<T>(handler))
        , token_(token)
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        if (command.substr(0, token_.size()) == token_)
        {
            command = command.substr(std::min(command.size(), token_.size() + 1));
            return handler_.handle(command, std::forward<Ctx>(ctx)...);
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
struct sequence
{
    sequence(T&&... handlers)
        : handlers_(std::forward<T>(handlers)...)
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
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
        else if (!std::get<I>(handlers_).handle(command, std::forward<Ctx>(ctx)...))
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
struct one_of
{
    one_of(T&&... handlers)
        : handlers_(std::forward<T>(handlers)...)
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
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
        else if (std::get<I>(handlers_).handle(command, std::forward<Ctx>(ctx)...))
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
struct repeat
{
    repeat(T&& handler)
        : handler_(std::forward<T>(handler))
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        while (!command.empty())
        {
            if (!handler_.handle(command, std::forward<Ctx>(ctx)...))
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
struct with_context
{
    with_context(new_Ctx&& ctx, T&& handler)
        : context_(std::forward<new_Ctx>(ctx))
        , handler_(std::forward<T>(handler))
    {
    }

    template <typename... Ctx>
    bool handle(std::string_view& command, Ctx&&... ctx)
    {
        return handler_.handle(command, std::forward<Ctx>(ctx)..., context_);
    }

    new_Ctx context_;
    T handler_;
};