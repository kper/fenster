//
// Created by Johannes Zottele on 06.12.25.
//

#ifndef MAIN_ATOMIC_H
#define MAIN_ATOMIC_H
#include <stdint.h>

class AtomicU64 {
    volatile uint64_t value;
public:
    explicit AtomicU64(uint64_t v = 0) : value(v) {}

    uint64_t fetch_add(uint64_t inc) {
        return __atomic_fetch_add(&value, inc, __ATOMIC_SEQ_CST);
    }


    uint64_t compare_and_swap(uint64_t old, uint64_t newval) {
        uint64_t expected = old;

        // weak memory ordering (relaxed); change to __ATOMIC_ACQ_REL if you need stronger
        __atomic_compare_exchange_n(
            &value,
            &expected,
            newval,
            false,                 // strong CAS (no spurious failure)
            __ATOMIC_RELAXED,      // success memory order
            __ATOMIC_RELAXED       // failure memory order
        );

        // returns the value that was actually in value before the CAS
        // -> CAS succeeded iff expected == old
        return expected;
    }

    uint64_t load() const {
        return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
    }
};

#endif //MAIN_ATOMIC_H