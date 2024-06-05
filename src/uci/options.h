#pragma once

#include "parse.h"
#include "validate_callback.h"

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
        auto action = [this](auto val) { return invoke_with_optional_validation(on_change, val); };
        return consume { name, consume { "value", next_token { to_bool { std::move(action) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
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
                return invoke_with_optional_validation(on_change, val);
            }
        };

        return consume { name, consume { "value", next_token { to_int { std::move(with_validation) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream& os)
    {
        os << name << ", "
           << "int"
           << ", " << default_value << ", " << min_value << ", " << max_value << ", "
           << float(max_value - min_value) / 20 << ", " << 0.002 << "\n";
    }

    const std::string_view name;
    const int default_value;
    const int min_value;
    const int max_value;
    T on_change;
};

// UCI extension: a float parameter that pretends to be 'string'. Useful for automated tuning
template <typename T>
struct float_option
{
    float_option(std::string_view name_, float default_value_, float min_value_, float max_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , min_value(min_value_)
        , max_value(max_value_)
        , on_change(std::move(on_change_))
    {
        assert(min_value <= default_value && default_value <= max_value);
    }

    friend std::ostream& operator<<(std::ostream& os, const float_option<T>& opt)
    {
        return os << "option name " << opt.name << " type string default " << opt.default_value << " min "
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
                return invoke_with_optional_validation(on_change, val);
            }
        };

        return consume { name, consume { "value", next_token { to_float { std::move(with_validation) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream& os)
    {
        os << name << ", "
           << "float"
           << ", " << default_value << ", " << min_value << ", " << max_value << ", " << (max_value - min_value) / 20
           << ", " << 0.002 << "\n";
    }

    const std::string_view name;
    const float default_value;
    const float min_value;
    const float max_value;
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
        return consume { name, invoke { [this]() { return invoke_with_optional_validation(on_change); } } };
    }

    void set_default()
    {
        // do nothing
    }

    void spsa_input_print(std::ostream&)
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
        auto action = [this](auto val) { return invoke_with_optional_validation(on_change, val); };
        return consume { name, consume { "value", next_token { std::move(action) } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
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
        : options(std::move(opts)...)
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

    void spsa_input_print(std::ostream& os)
    {
        std::apply([&](auto&... opts) { (opts.spsa_input_print(os), ...); }, options);
    }

private:
    std::tuple<T...> options;
};
