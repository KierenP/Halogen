#pragma once

#include <array>
#include <cstdint>
#include <iterator>

// Stack allocated fixed capacity vector. Should be a drop-in replacement for std::vector, with some lesser
// functionality
template <typename T, std::size_t N>
class StaticVector
{
private:
    std::array<T, N> elems_;
    std::size_t size_ = 0;

public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using iterator = typename decltype(elems_)::iterator;
    using const_iterator = typename decltype(elems_)::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // element access

    reference at(size_t pos)
    {
        return elems_[pos];
    }

    const_reference at(size_t pos) const
    {
        return elems_[pos];
    }

    reference operator[](size_t pos)
    {
        return elems_[pos];
    }

    const_reference operator[](size_t pos) const
    {
        return elems_[pos];
    }

    reference front()
    {
        return elems_[0];
    }

    const_reference front() const
    {
        return elems_[0];
    }

    reference back()
    {
        return elems_[size_ - 1];
    }

    const_reference back() const
    {
        return elems_[size_ - 1];
    }

    // iterators

    iterator begin()
    {
        return &elems_[0];
    };

    const_iterator begin() const
    {
        return &elems_[0];
    };

    const_iterator cbegin() const
    {
        return &elems_[0];
    };

    iterator end()
    {
        return &elems_[size_];
    }

    const_iterator end() const
    {
        return &elems_[size_];
    }

    const_iterator cend() const
    {
        return &elems_[size_];
    }

    reverse_iterator rbegin()
    {
        return end();
    }

    const_reverse_iterator rbegin() const
    {
        return end();
    }

    const_reverse_iterator crbegin() const
    {
        return end();
    }

    reverse_iterator rend()
    {
        return begin();
    }

    const_reverse_iterator rend() const
    {
        return begin();
    }

    const_reverse_iterator crend() const
    {
        return begin();
    }

    // capacity

    bool empty() const
    {
        return size_ == 0;
    }

    size_t size() const
    {
        return size_;
    }

    size_t max_size() const
    {
        return N;
    }

    void reserve(std::size_t)
    {
        // no-op
    }

    std::size_t capacity() const
    {
        return N;
    }

    void shrink_to_fit()
    {
        // no-op
    }

    // modifiers

    void clear()
    {
        size_ = 0;
    }

    iterator insert(const_iterator pos, const T& value)
    {
        std::move_backward(pos, end(), std::next(end()));
        *pos = value;
        size_++;
        return pos;
    }

    iterator insert(const_iterator pos, T&& value)
    {
        std::move_backward(pos, end(), std::next(end()));
        *pos = std::move(value);
        size_++;
        return pos;
    }

    template <class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), end() + (last - first));
        std::copy(first, last, mutable_it);
        size_ += last - first;
        return mutable_it;
    }

    template <class... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), std::next(end()));
        *mutable_it = T { std::forward<Args>(args)... };
        size_++;
        return mutable_it;
    }

    iterator erase(const_iterator pos)
    {
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move(std::next(mutable_it), end(), mutable_it);
        size_--;
        return mutable_it;
    }

    void push_back(const T& value)
    {
        elems_[size_++] = value;
    }

    void push_back(T&& value)
    {
        elems_[size_++] = std::move(value);
    }

    template <class... Args>
    reference emplace_back(Args&&... args)
    {
        elems_[size_++] = T { std::forward<Args>(args)... };
        return back();
    }

    void pop_back()
    {
        size_--;
    }
};