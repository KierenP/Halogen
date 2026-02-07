#pragma once

#include <array>
#include <cassert>
#include <iterator>

// TODO: bring this more in line with C++26 std::inplace_vector
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

    constexpr reference at(size_t pos)
    {
        assert(pos < size_);
        return elems_[pos];
    }

    constexpr const_reference at(size_t pos) const
    {
        assert(pos < size_);
        return elems_[pos];
    }

    constexpr reference operator[](size_t pos)
    {
        assert(pos < size_);
        return elems_[pos];
    }

    constexpr const_reference operator[](size_t pos) const
    {
        assert(pos < size_);
        return elems_[pos];
    }

    constexpr reference front()
    {
        assert(size_ > 0);
        return elems_[0];
    }

    constexpr const_reference front() const
    {
        assert(size_ > 0);
        return elems_[0];
    }

    constexpr reference back()
    {
        assert(size_ > 0);
        return elems_[size_ - 1];
    }

    constexpr const_reference back() const
    {
        assert(size_ > 0);
        return elems_[size_ - 1];
    }

    // iterators

    constexpr iterator begin()
    {
        return &elems_[0];
    };

    constexpr const_iterator begin() const
    {
        return &elems_[0];
    };

    constexpr const_iterator cbegin() const
    {
        return &elems_[0];
    };

    constexpr iterator end()
    {
        return &elems_[size_];
    }

    constexpr const_iterator end() const
    {
        return &elems_[size_];
    }

    constexpr const_iterator cend() const
    {
        return &elems_[size_];
    }

    constexpr reverse_iterator rbegin()
    {
        return end();
    }

    constexpr const_reverse_iterator rbegin() const
    {
        return end();
    }

    constexpr const_reverse_iterator crbegin() const
    {
        return end();
    }

    constexpr reverse_iterator rend()
    {
        return begin();
    }

    constexpr const_reverse_iterator rend() const
    {
        return begin();
    }

    constexpr const_reverse_iterator crend() const
    {
        return begin();
    }

    // capacity

    constexpr bool empty() const
    {
        return size_ == 0;
    }

    constexpr size_t size() const
    {
        return size_;
    }

    constexpr size_t max_size() const
    {
        return N;
    }

    constexpr void reserve(std::size_t)
    {
        // no-op
    }

    constexpr std::size_t capacity() const
    {
        return N;
    }

    constexpr void shrink_to_fit()
    {
        // no-op
    }

    // modifiers

    constexpr void clear()
    {
        size_ = 0;
    }

    constexpr iterator insert(const_iterator pos, const T& value)
    {
        assert(size_ < N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), std::next(end()));
        *mutable_it = value;
        size_++;
        return mutable_it;
    }

    constexpr iterator insert(const_iterator pos, T&& value)
    {
        assert(size_ < N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), std::next(end()));
        *mutable_it = std::move(value);
        size_++;
        return mutable_it;
    }

    template <class InputIt>
    constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        assert(size_ + (last - first) <= N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), end() + (last - first));
        std::copy(first, last, mutable_it);
        size_ += last - first;
        return mutable_it;
    }

    template <class... Args>
    constexpr iterator emplace(const_iterator pos, Args&&... args)
    {
        assert(size_ < N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), std::next(end()));
        *mutable_it = T { std::forward<Args>(args)... };
        size_++;
        return mutable_it;
    }

    constexpr iterator erase(const_iterator pos)
    {
        assert(size_ > 0);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move(std::next(mutable_it), end(), mutable_it);
        size_--;
        return mutable_it;
    }

    constexpr iterator erase(const_iterator first, const_iterator last)
    {
        assert((int)size_ >= (last - first));
        auto mutable_first = begin() + std::distance(cbegin(), first);
        std::move(last, cend(), mutable_first);
        size_ -= last - first;
        return mutable_first;
    }

    constexpr void push_back(const T& value)
    {
        assert(size_ < N);
        elems_[size_++] = value;
    }

    constexpr void push_back(T&& value)
    {
        assert(size_ < N);
        elems_[size_++] = std::move(value);
    }

    template <class... Args>
    constexpr reference emplace_back(Args&&... args)
    {
        assert(size_ < N);
        elems_[size_++] = T { std::forward<Args>(args)... };
        return back();
    }

    constexpr void pop_back()
    {
        assert(size_ > 0);
        size_--;
    }

    constexpr auto operator<=>(const StaticVector& other) const
    {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }

    constexpr bool operator==(const StaticVector& other) const
    {
        return size_ == other.size_ && std::equal(begin(), end(), other.begin());
    }

    // extensions
    constexpr void unsafe_resize(std::size_t new_size)
    {
        assert(new_size <= N);
        size_ = new_size;
    }
};