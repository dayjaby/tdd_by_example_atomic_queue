/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
#ifndef ATOMIC_QUEUE_ATOMIC_QUEUE_H_INCLUDED
#define ATOMIC_QUEUE_ATOMIC_QUEUE_H_INCLUDED

// Copyright (c) 2019 Maxim Egorushkin. MIT License. See the full licence in file LICENSE.

#include "defs.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace atomic_queue {

using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace details {

template<class T>
constexpr T nil() noexcept {
#if __cpp_lib_atomic_is_always_lock_free // Better compile-time error message requires C++17.
    static_assert(std::atomic<T>::is_always_lock_free, "Queue element type T is not atomic. Use AtomicQueue/AtomicQueueB2 for such element types.");
#endif
    return {};
}

template<class T>
constexpr T decrement(T x) noexcept {
    return x - 1;
}

template<class T>
constexpr T increment(T x) noexcept {
    return x + 1;
}

template<class T>
constexpr T or_equal(T x, unsigned u) noexcept {
    return x | x >> u;
}

template<class T, class... Args>
constexpr T or_equal(T x, unsigned u, Args... rest) noexcept {
    return or_equal(or_equal(x, u), rest...);
}

constexpr uint32_t round_up_to_power_of_2(uint32_t a) noexcept {
    return increment(or_equal(decrement(a), 1, 2, 4, 8, 16));
}

constexpr uint64_t round_up_to_power_of_2(uint64_t a) noexcept {
    return increment(or_equal(decrement(a), 1, 2, 4, 8, 16, 32));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
constexpr unsigned char EMPTY = 0;
constexpr unsigned char STORING = 1;
constexpr unsigned char STORED = 2;
constexpr unsigned char LOADING = 3;

template<class T, unsigned SIZE, class A = std::allocator<T>, T NIL = details::nil<T>()>
class AtomicQueue {
public:
    static constexpr unsigned size_ = SIZE;
    alignas(CACHE_LINE_SIZE) std::atomic<int> head{0}; // where new elements get pushed
    alignas(CACHE_LINE_SIZE) std::atomic<int> tail{0}; // from where elements are popped
    // alignas(CACHE_LINE_SIZE) std::atomic<T> data[SIZE] = {};
    alignas(CACHE_LINE_SIZE) T data[size_] = {};
    alignas(CACHE_LINE_SIZE) std::atomic<unsigned char> state[size_] = {};

    template <class U>
    bool try_push(U&& element) noexcept {
        // Step 1: get an empty index in the queue
        int n = head.load(std::memory_order_relaxed);
        if (n - tail.load(std::memory_order_relaxed) >= size_) {
            return false;
        }

        if (!head.compare_exchange_weak(n, n+1, std::memory_order_relaxed, std::memory_order_relaxed)) {
            return false;
        }
        n = (n - 1) % size_;

        // Step 2: store element at the index
        while(true) {
            unsigned char expected = EMPTY;
            if (state[n].compare_exchange_weak(expected, STORING, std::memory_order_acquire, std::memory_order_relaxed)) {
                data[n] = std::forward<U>(element);
                state[n].store(STORED, std::memory_order_release);
                return true;
            }
            while (state[n].load(std::memory_order_relaxed) != EMPTY) {
                spin_loop_pause();
            }
        }
    }

    bool try_pop(T& element) noexcept {
        // Step 1: get a stored index in the queue
        int n = tail.load(std::memory_order_relaxed);
        if (head.load(std::memory_order_relaxed) - n <= 0) {
            return false;
        }
        if (!tail.compare_exchange_weak(n, n+1, std::memory_order_relaxed, std::memory_order_relaxed)) {
            return false;
        }
        n = (n - 1) % size_;

        // Step 2: read element at the index
        while(true) {
            unsigned char expected = STORED;
            if (state[n].compare_exchange_weak(expected, LOADING, std::memory_order_acquire, std::memory_order_relaxed)) {
                element = std::move(data[n]);
                state[n].store(EMPTY, std::memory_order_release);
                return true;
            }
            while (state[n].load(std::memory_order_relaxed) != STORED) {
                spin_loop_pause();
            }
        }
    }

    template <class U>
    void push(U&& element) noexcept {
        while(!this->try_push(std::forward<U>(element)))
            spin_loop_pause();
    }

    auto pop() noexcept {
        T element;
        while(!this->try_pop(element))
            spin_loop_pause();
        return element;
    }

    bool was_empty() const noexcept {
        return (was_size() <= 0);
    }

    bool was_full() const noexcept {
        return (was_size() >= size_);
    }

    unsigned was_size() const noexcept {
        return std::max(head.load(std::memory_order_relaxed) - tail.load(std::memory_order_relaxed), 0);
    }

    unsigned capacity() const noexcept {
        return size_;
    }

public:
    using value_type = T;

    AtomicQueue() noexcept = default;
    AtomicQueue(AtomicQueue const&) = delete;
    AtomicQueue& operator=(AtomicQueue const&) = delete;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace atomic_queue

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // ATOMIC_QUEUE_ATOMIC_QUEUE_H_INCLUDED
