#pragma once

#include "parse.h"

#include <cassert>
#include <ostream>
#include <string_view>
#include <tuple>

template <typename T>
struct check_option
{
    check_option(std::string_view name_, bool default_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const check_option<T>& opt)
    {
        return os << std::boolalpha << "option name " << opt.name << " type check default " << opt.default_value
                  << "\n";
    }

    auto handler()
    {
        return consume { name, consume { "value", next_token { to_bool { [this](auto val) { on_change(val); } } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    const std::string_view name;
    const bool default_value;
    T on_change;
};

template <typename T>
struct spin_option
{
    spin_option(std::string_view name_, int default_value_, int min_value_, int max_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , min_value(min_value_)
        , max_value(max_value_)
        , on_change(std::move(on_change_))
    {
        assert(min_value <= default_value && default_value <= max_value);
    }

    friend std::ostream& operator<<(std::ostream& os, const spin_option<T>& opt)
    {
        return os << "option name " << opt.name << " type spin default " << opt.default_value << " min "
                  << opt.min_value << " max " << opt.max_value << "\n";
    }

    auto handler()
    {
        auto with_validation = [this](auto val)
        {
            if (val < min_value || val > max_value)
            {
                return false;
            }
            else
            {
                on_change(val);
                return true;
            }
        };

        return consume { name, consume { "value", next_token { to_int { std::move(with_validation) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    const std::string_view name;
    const int default_value;
    const int min_value;
    const int max_value;
    T on_change;
};

template <typename T>
struct button_option
{
    button_option(std::string_view name_, T&& on_change_)
        : name(name_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const button_option<T>& opt)
    {
        return os << "option name " << opt.name << " type button\n";
    }

    auto handler()
    {
        return consume { name, invoke { [this]() { on_change(); } } };
    }

    void set_default()
    {
        // do nothing
    }

    const std::string_view name;
    T on_change;
};

template <typename T>
struct string_option
{
    string_option(std::string_view name_, std::string_view default_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const string_option<T>& opt)
    {
        return os << "option name " << opt.name << " type string default " << opt.default_value << "\n";
    }

    auto handler()
    {
        return consume { name, next_token { [this](auto str) { on_change(str); } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    const std::string_view name;
    const std::string_view default_value;
    T on_change;
};

template <typename... T>
class uci_options
{
public:
    uci_options(T&&... opts)
        : options(std::forward<T>(opts)...)
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const uci_options<T...>& opt)
    {
        return std::apply([&](auto&... opts) -> decltype(auto) { return (os << ... << opts); }, opt.options);
    }

    auto build_handler()
    {
        return consume { "name", std::apply([&](auto&... opts) { return one_of { opts.handler()... }; }, options) };
    }

    void set_defaults()
    {
        std::apply([&](auto&... opts) { (opts.set_default(), ...); }, options);
    }

private:
    std::tuple<T...> options;
};
