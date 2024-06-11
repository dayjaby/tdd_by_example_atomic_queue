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
    static_assert(std::atomic<T>::is_always_lock_free, "Queue element type T is not atomic. Use AtomicQueue2/AtomicQueueB2 for such element types.");
#endif
    return {};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, unsigned SIZE, class A = std::allocator<T>, T NIL = details::nil<T>()>
class AtomicQueue2 {
public:
    template <class U>
    bool try_push(U&& element) noexcept {
        return false;
    }

    bool try_pop(T& element) noexcept {
        return false;
    }

    template <class U>
    void push(U&& element) noexcept {
    }

    auto pop() noexcept {
    }

    bool was_empty() const noexcept {
        return true;
    }

    bool was_full() const noexcept {
        return false;
    }

    unsigned was_size() const noexcept {
        return 0;
    }

    unsigned capacity() const noexcept {
        return 0;
    }

    T do_pop(unsigned tail) noexcept {
    }

    template <class U>
    void do_push(U&& element, unsigned head) noexcept {
    }

public:
    using value_type = T;

    AtomicQueue2() noexcept = default;
    AtomicQueue2(AtomicQueue2 const&) = delete;
    AtomicQueue2& operator=(AtomicQueue2 const&) = delete;
};

template<class Queue>
struct RetryDecorator : Queue {
    using T = typename Queue::value_type;

    using Queue::Queue;

    void push(T element) noexcept {
        while(!this->try_push(element))
            spin_loop_pause();
    }

    T pop() noexcept {
        T element;
        while(!this->try_pop(element))
            spin_loop_pause();
        return element;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace atomic_queue

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // ATOMIC_QUEUE_ATOMIC_QUEUE_H_INCLUDED
