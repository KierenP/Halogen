#pragma once

#include "uci/parse.h"
#include "uci/validate_callback.h"

#include <cassert>
#include <optional>
#include <ostream>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace UCI
{

template <typename T>
struct CheckOption
{
    CheckOption(std::string_view name_, bool default_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const CheckOption<T>& opt)
    {
        return os << std::boolalpha << "option name " << opt.name << " type check default " << opt.default_value
                  << "\n";
    }

    auto handler()
    {
        auto action = [this](auto val) { return invoke_with_optional_validation(on_change, val); };
        return Consume { name, Consume { "value", NextToken { ToBool { std::move(action) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
    }

    std::string_view name;
    bool default_value;
    T on_change;
};

template <typename T>
struct SpinOption
{
    SpinOption(std::string_view name_, int default_value_, int min_value_, int max_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , min_value(min_value_)
        , max_value(max_value_)
        , on_change(std::move(on_change_))
    {
        assert(min_value <= default_value && default_value <= max_value);
    }

    friend std::ostream& operator<<(std::ostream& os, const SpinOption<T>& opt)
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

        return Consume { name, Consume { "value", NextToken { ToInt { std::move(with_validation) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream& os)
    {
        auto c_end = float(max_value - min_value) / 20;
        auto r_end = 0.002 / (std::min(0.5f, c_end) / 0.5);
        os << name << ", int, " << default_value << ", " << min_value << ", " << max_value << ", " << c_end << ", "
           << r_end << "\n";
    }

    std::string_view name;
    int default_value;
    int min_value;
    int max_value;
    T on_change;
};

// UCI extension: a float parameter that pretends to be 'string'. Useful for automated tuning
template <typename T>
struct FloatOption
{
    FloatOption(std::string_view name_, float default_value_, float min_value_, float max_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , min_value(min_value_)
        , max_value(max_value_)
        , on_change(std::move(on_change_))
    {
        assert(min_value <= default_value && default_value <= max_value);
    }

    friend std::ostream& operator<<(std::ostream& os, const FloatOption<T>& opt)
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

        return Consume { name, Consume { "value", NextToken { ToFloat { std::move(with_validation) } } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream& os)
    {
        os << name << ", float, " << default_value << ", " << min_value << ", " << max_value << ", "
           << (max_value - min_value) / 20 << ", " << 0.002 << "\n";
    }

    std::string_view name;
    float default_value;
    float min_value;
    float max_value;
    T on_change;
};

template <typename T>
struct ButtonOption
{
    ButtonOption(std::string_view name_, T&& on_change_)
        : name(name_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const ButtonOption<T>& opt)
    {
        return os << "option name " << opt.name << " type button\n";
    }

    auto handler()
    {
        return Consume { name, Invoke { [this]() { return invoke_with_optional_validation(on_change); } } };
    }

    void set_default()
    {
        // do nothing
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
    }

    std::string_view name;
    T on_change;
};

template <typename T>
struct StringOption
{
    StringOption(std::string_view name_, std::string_view default_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const StringOption<T>& opt)
    {
        return os << "option name " << opt.name << " type string default " << opt.default_value << "\n";
    }

    auto handler()
    {
        auto action = [this](auto val) { return invoke_with_optional_validation(on_change, val); };
        return Consume { name, Consume { "value", NextToken { std::move(action) } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
    }

    std::string_view name;
    std::string_view default_value;
    T on_change;
};

// In order to define a combo option on an enum, there needs to be a valid specialization of to_enum, a valid overload
// of operator<<, and the enum must start from 0, end with ENUM_END, and be dense (no gaps).
template <typename T>
std::optional<T> to_enum(std::string_view str);

template <typename Enum, typename T>
struct ComboOption
{
    ComboOption(std::string_view name_, Enum default_value_, T&& on_change_)
        : name(name_)
        , default_value(default_value_)
        , on_change(std::move(on_change_))
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const ComboOption<Enum, T>& opt)
    {
        os << "option name " << opt.name << " type combo default " << opt.default_value;
        using U = std::underlying_type_t<Enum>;

        for (U i = 0; i != static_cast<U>(Enum::ENUM_END); i++)
        {
            os << " var " << static_cast<Enum>(i);
        }

        return os << "\n";
    }

    auto handler()
    {
        auto action = [this](auto str)
        {
            if (auto converted = to_enum<Enum>(str); converted.has_value())
            {
                return invoke_with_optional_validation(on_change, *converted);
            }
            else
            {
                return false;
            }
        };

        return Consume { name, Consume { "value", NextToken { std::move(action) } } };
    }

    void set_default()
    {
        on_change(default_value);
    }

    void spsa_input_print(std::ostream&)
    {
        // do nothing
    }

    std::string_view name;
    Enum default_value;
    T on_change;
};

template <typename... T>
class Options
{
public:
    Options(T&&... opts)
        : options(std::move(opts)...)
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const Options<T...>& opt)
    {
        return std::apply([&](auto&... opts) -> decltype(auto) { return (os << ... << opts); }, opt.options);
    }

    auto build_handler()
    {
        return Consume { "name", std::apply([&](auto&... opts) { return OneOf { opts.handler()... }; }, options) };
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

}