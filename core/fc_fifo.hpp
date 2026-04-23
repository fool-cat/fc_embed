/**
 * @file fc_fifo.hpp
 * @author fool_cat
 * @brief C++ 固定容量泛型 FIFO, 接口命名尽量贴近 STL
 * 注意代码膨胀,泛型不同类型或者同类型不同大小 等效于两套代码!
 * @version 1.0
 * @date 2026-04-08
 */

#ifndef __FC_FIFO_HPP__
#define __FC_FIFO_HPP__

#include "fc_config.h"

#ifdef __cplusplus

    #include <cassert>
    #include <cstddef>
    #include <initializer_list>
    #include <new>
    #include <type_traits>
    #include <utility>

    #ifndef fc_assert
        #define fc_assert(expr) ((void)0)
    #endif

    #ifndef fc_fifo_cpp_assert
        #define fc_fifo_cpp_assert(expr) fc_assert(expr)
    #endif

namespace fc_embed
{
    template <typename T, std::size_t log2_size>
    class fifo_t
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;

        static constexpr size_type static_capacity = (static_cast<size_type>(1) << log2_size);

        static_assert(log2_size >= 1, "fc_embed::fifo_t log2_size must be at least 1");

        fifo_t() noexcept = default;

        fifo_t(std::initializer_list<value_type> init)
        {
            fc_fifo_cpp_assert(init.size() <= capacity());
            for (const auto &value : init)
            {
                (void)push(value);
            }
        }

        fifo_t(const fifo_t &other)
        {
            copy_from(other);
        }

        fifo_t(fifo_t &&other) noexcept(std::is_nothrow_move_constructible<value_type>::value)
        {
            move_from(std::move(other));
        }

        fifo_t &operator=(const fifo_t &other)
        {
            if (this != &other)
            {
                clear();
                copy_from(other);
            }

            return *this;
        }

        fifo_t &operator=(fifo_t &&other) noexcept(std::is_nothrow_move_constructible<value_type>::value && std::is_nothrow_destructible<value_type>::value)
        {
            if (this != &other)
            {
                clear();
                move_from(std::move(other));
            }

            return *this;
        }

        ~fifo_t()
        {
            clear();
        }

        constexpr size_type capacity() const noexcept
        {
            return static_capacity;
        }

        constexpr size_type max_size() const noexcept
        {
            return static_capacity;
        }

        size_type size() const noexcept
        {
            return (in_ - out_);
        }

        size_type free_size() const noexcept
        {
            return (capacity() - size());
        }

        bool empty() const noexcept
        {
            return (in_ == out_);
        }

        bool full() const noexcept
        {
            return (size() == capacity());
        }

        void clear() noexcept(std::is_nothrow_destructible<value_type>::value)
        {
            while (pop())
            {
            }
        }

        void reset() noexcept(std::is_nothrow_destructible<value_type>::value)
        {
            clear();
        }

        void reset_read() noexcept(std::is_nothrow_destructible<value_type>::value)
        {
            clear();
        }

        void reset_write() noexcept(std::is_nothrow_destructible<value_type>::value)
        {
            clear();
        }

        template <typename... Args>
        bool emplace(Args &&...args)
        {
            if (full())
            {
                return false;
            }

            ::new (static_cast<void *>(slot(in_))) value_type(std::forward<Args>(args)...);
            ++in_;
            return true;
        }

        bool push(const value_type &value)
        {
            return emplace(value);
        }

        bool push(value_type &&value)
        {
            return emplace(std::move(value));
        }

        template <typename... Args>
        bool emplace_overwrite(Args &&...args)
        {
            if (full())
            {
                (void)pop();
            }

            return emplace(std::forward<Args>(args)...);
        }

        bool push_overwrite(const value_type &value)
        {
            return emplace_overwrite(value);
        }

        bool push_overwrite(value_type &&value)
        {
            return emplace_overwrite(std::move(value));
        }

        bool pop()
        {
            if (empty())
            {
                return false;
            }

            destroy(out_);
            ++out_;
            return true;
        }

        bool pop(value_type &value)
        {
            if (empty())
            {
                return false;
            }

            value = std::move(front());
            return pop();
        }

        bool try_pop(value_type &value)
        {
            return pop(value);
        }

        reference front()
        {
            fc_fifo_cpp_assert(!empty());
            return *slot(out_);
        }

        const_reference front() const
        {
            fc_fifo_cpp_assert(!empty());
            return *slot(out_);
        }

        reference back()
        {
            fc_fifo_cpp_assert(!empty());
            return *slot(in_ - 1);
        }

        const_reference back() const
        {
            fc_fifo_cpp_assert(!empty());
            return *slot(in_ - 1);
        }

        reference operator[](size_type index)
        {
            fc_fifo_cpp_assert(index < size());
            return *slot(out_ + index);
        }

        const_reference operator[](size_type index) const
        {
            fc_fifo_cpp_assert(index < size());
            return *slot(out_ + index);
        }

        reference at(size_type index)
        {
            fc_fifo_cpp_assert(index < size());
            return (*this)[index];
        }

        const_reference at(size_type index) const
        {
            fc_fifo_cpp_assert(index < size());
            return (*this)[index];
        }

        bool peek(value_type &value) const
        {
            if (empty())
            {
                return false;
            }

            value = front();
            return true;
        }

        bool seek(value_type &value, size_type offset) const
        {
            if (offset >= size())
            {
                return false;
            }

            value = (*this)[offset];
            return true;
        }

        size_type write(const value_type *data, size_type count)
        {
            size_type written = 0;

            fc_fifo_cpp_assert((data != nullptr) || (count == 0));

            while ((written < count) && push(data[written]))
            {
                ++written;
            }

            return written;
        }

        size_type overwrite(const value_type *data, size_type count)
        {
            size_type offset = 0;

            fc_fifo_cpp_assert((data != nullptr) || (count == 0));

            if (count > capacity())
            {
                offset = count - capacity();
                count = capacity();
            }

            for (size_type i = 0; i < count; ++i)
            {
                (void)push_overwrite(data[offset + i]);
            }

            return count;
        }

        size_type peek(value_type *data, size_type count) const
        {
            return seek(data, count, 0);
        }

        size_type read(value_type *data, size_type count)
        {
            const size_type read_count = peek(data, count);
            (void)drop(read_count);
            return read_count;
        }

        size_type seek(value_type *data, size_type count, size_type offset) const
        {
            const size_type used = size();
            size_type       actual = 0;

            fc_fifo_cpp_assert((data != nullptr) || (count == 0));

            if (offset >= used)
            {
                return 0;
            }

            actual = used - offset;
            if (count < actual)
            {
                actual = count;
            }

            for (size_type i = 0; i < actual; ++i)
            {
                data[i] = (*this)[offset + i];
            }

            return actual;
        }

        size_type drop(size_type count)
        {
            const size_type actual = (count < size()) ? count : size();

            for (size_type i = 0; i < actual; ++i)
            {
                (void)pop();
            }

            return actual;
        }

        bool pop_back()
        {
            if (empty())
            {
                return false;
            }

            --in_;
            destroy(in_);
            return true;
        }

        size_type drop_back(size_type count)
        {
            const size_type actual = (count < size()) ? count : size();

            for (size_type i = 0; i < actual; ++i)
            {
                (void)pop_back();
            }

            return actual;
        }

        bool drop_front()
        {
            return pop();
        }

    private:
        using storage_type = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;

        static constexpr size_type mask_value = (static_capacity - 1);

        pointer slot(size_type index) noexcept
        {
            return reinterpret_cast<pointer>(&storage_[index & mask_value]);
        }

        const_pointer slot(size_type index) const noexcept
        {
            return reinterpret_cast<const_pointer>(&storage_[index & mask_value]);
        }

        void destroy(size_type index) noexcept(std::is_nothrow_destructible<value_type>::value)
        {
            slot(index)->~value_type();
        }

        void copy_from(const fifo_t &other)
        {
            for (size_type i = 0; i < other.size(); ++i)
            {
                (void)emplace(other[i]);
            }
        }

        void move_from(fifo_t &&other) noexcept(std::is_nothrow_move_constructible<value_type>::value && std::is_nothrow_destructible<value_type>::value)
        {
            while (!other.empty())
            {
                (void)emplace(std::move(other.front()));
                (void)other.pop();
            }
        }

    private:
        storage_type storage_[static_capacity];
        size_type    in_ = 0;
        size_type    out_ = 0;
    };

}  // namespace fc_embed

#endif  // __cplusplus

#endif  // __FC_FIFO_HPP__
