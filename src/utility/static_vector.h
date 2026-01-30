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

    reference at(size_t pos) noexcept
    {
        assert(pos < size_);
        return elems_[pos];
    }

    const_reference at(size_t pos) const noexcept
    {
        assert(pos < size_);
        return elems_[pos];
    }

    reference operator[](size_t pos) noexcept
    {
        assert(pos < size_);
        return elems_[pos];
    }

    const_reference operator[](size_t pos) const noexcept
    {
        assert(pos < size_);
        return elems_[pos];
    }

    reference front() noexcept
    {
        assert(size_ > 0);
        return elems_[0];
    }

    const_reference front() const noexcept
    {
        assert(size_ > 0);
        return elems_[0];
    }

    reference back() noexcept
    {
        assert(size_ > 0);
        return elems_[size_ - 1];
    }

    const_reference back() const noexcept
    {
        assert(size_ > 0);
        return elems_[size_ - 1];
    }

    // iterators

    iterator begin() noexcept
    {
        return &elems_[0];
    };

    const_iterator begin() const noexcept
    {
        return &elems_[0];
    };

    const_iterator cbegin() const noexcept
    {
        return &elems_[0];
    };

    iterator end() noexcept
    {
        return &elems_[size_];
    }

    const_iterator end() const noexcept
    {
        return &elems_[size_];
    }

    const_iterator cend() const noexcept
    {
        return &elems_[size_];
    }

    reverse_iterator rbegin() noexcept
    {
        return end();
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return end();
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return end();
    }

    reverse_iterator rend() noexcept
    {
        return begin();
    }

    const_reverse_iterator rend() const noexcept
    {
        return begin();
    }

    const_reverse_iterator crend() const noexcept
    {
        return begin();
    }

    // capacity

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    size_t size() const noexcept
    {
        return size_;
    }

    size_t max_size() const noexcept
    {
        return N;
    }

    void reserve(std::size_t) noexcept
    {
        // no-op
    }

    std::size_t capacity() const noexcept
    {
        return N;
    }

    void shrink_to_fit() noexcept
    {
        // no-op
    }

    // modifiers

    void clear() noexcept
    {
        size_ = 0;
    }

    iterator insert(const_iterator pos, const T& value) noexcept
    {
        assert(size_ < N);
        std::move_backward(pos, end(), std::next(end()));
        *pos = value;
        size_++;
        return pos;
    }

    iterator insert(const_iterator pos, T&& value) noexcept
    {
        assert(size_ < N);
        std::move_backward(pos, end(), std::next(end()));
        *pos = std::move(value);
        size_++;
        return pos;
    }

    template <class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) noexcept
    {
        assert(size_ + (last - first) <= N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), end() + (last - first));
        std::copy(first, last, mutable_it);
        size_ += last - first;
        return mutable_it;
    }

    template <class... Args>
    iterator emplace(const_iterator pos, Args&&... args) noexcept
    {
        assert(size_ < N);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move_backward(mutable_it, end(), std::next(end()));
        *mutable_it = T { std::forward<Args>(args)... };
        size_++;
        return mutable_it;
    }

    iterator erase(const_iterator pos) noexcept
    {
        assert(size_ > 0);
        auto mutable_it = begin() + std::distance(cbegin(), pos);
        std::move(std::next(mutable_it), end(), mutable_it);
        size_--;
        return mutable_it;
    }

    iterator erase(const_iterator first, const_iterator last) noexcept
    {
        assert((int)size_ > (last - first));
        auto mutable_first = begin() + std::distance(cbegin(), first);
        std::move(last, cend(), mutable_first);
        size_ -= last - first;
        return mutable_first;
    }

    void push_back(const T& value) noexcept
    {
        assert(size_ < N);
        elems_[size_++] = value;
    }

    void push_back(T&& value) noexcept
    {
        assert(size_ < N);
        elems_[size_++] = std::move(value);
    }

    template <class... Args>
    reference emplace_back(Args&&... args) noexcept
    {
        assert(size_ < N);
        elems_[size_++] = T { std::forward<Args>(args)... };
        return back();
    }

    void pop_back() noexcept
    {
        assert(size_ > 0);
        size_--;
    }

    bool operator<=>(const StaticVector& other) const noexcept
    {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }

    bool operator==(const StaticVector& other) const noexcept
    {
        return size_ == other.size_ && std::equal(begin(), end(), other.begin());
    }

    // extensions
    void unsafe_resize(std::size_t new_size) noexcept
    {
        assert(new_size <= N);
        size_ = new_size;
    }
};