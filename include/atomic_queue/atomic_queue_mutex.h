/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
#ifndef ATOMIC_QUEUE_ATOMIC_QUEUE_SPIN_LOCK_H_INCLUDED
#define ATOMIC_QUEUE_ATOMIC_QUEUE_SPIN_LOCK_H_INCLUDED

// Copyright (c) 2019 Maxim Egorushkin. MIT License. See the full licence in file LICENSE.

#include "atomic_queue.h"
#include "spinlock.h"

#include <mutex>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace atomic_queue {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class M>
struct ScopedLockType {
    using type = typename M::scoped_lock;
};

template<>
struct ScopedLockType<std::mutex> {
    using type = std::unique_lock<std::mutex>;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Mutex, unsigned SIZE, bool MINIMIZE_CONTENTION>
class AtomicQueueMutexT {
public:
    using value_type = T;

    template<class U>
    bool try_push(U&& element) noexcept {
        return false;
    }

    bool try_pop(T& element) noexcept {
        return false;
    }

    bool was_empty() const noexcept {
        return true;
    }

    bool was_full() const noexcept {
        return false;
    }
};

template<class T, unsigned SIZE, class Mutex, bool MINIMIZE_CONTENTION = true>
using AtomicQueueMutex = AtomicQueueMutexT<T, Mutex, SIZE, MINIMIZE_CONTENTION>;

template<class T, unsigned SIZE, bool MINIMIZE_CONTENTION = true>
using AtomicQueueSpinlock = AtomicQueueMutexT<T, Spinlock, SIZE, MINIMIZE_CONTENTION>;

// template<class T, unsigned SIZE, bool MINIMIZE_CONTENTION = true>
// using AtomicQueueSpinlockHle = AtomicQueueMutexT<T, SpinlockHle, SIZE, MINIMIZE_CONTENTION>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace atomic_queue

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // ATOMIC_QUEUE_ATOMIC_QUEUE_SPIN_LOCK_H_INCLUDED
