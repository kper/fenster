//
// Created by Johannes Zottele on 05.12.25.
//

#ifndef MAIN_UTILS_H
#define MAIN_UTILS_H
#include "move.h"
#include "new.h"

namespace rnt {
    template<typename T>
    class Optional {
        bool has_;
        alignas(T) unsigned char storage_[sizeof(T)];

        T* ptr()       { return reinterpret_cast<T*>(storage_); }
        const T* ptr() const { return reinterpret_cast<const T*>(storage_); }

    public:
        Optional() : has_(false) {}

        Optional(const T& value) : has_(true) {
            new (storage_) T(value);
        }

        Optional(const Optional& other) : has_(other.has_) {
            if (other.has_) {
                new (storage_) T(*other.ptr());   // copy-propagated
            }
        }

        Optional(Optional&& other) : has_(other.has_) {
            if (other.has_) {
                new (storage_) T(rnt::move(*other.ptr()));  // move-propagated
                other.reset();
            }
        }

        Optional& operator=(const Optional& other) {
            if (this != &other) {
                reset();
                if (other.has_) {
                    new (storage_) T(*other.ptr());
                    has_ = true;
                }
            }
            return *this;
        }

        Optional& operator=(Optional&& other) {
            if (this != &other) {
                reset();
                if (other.has_) {
                    new (storage_) T(rnt::move(*other.ptr()));
                    has_ = true;
                    other.reset();
                }
            }
            return *this;
        }

        ~Optional() { reset(); }

        bool has_value() const { return has_; }
        bool is_empty() const { return !has_value(); }

        T&       value()       { return *ptr(); }
        const T& value() const { return *ptr(); }

        void reset() {
            if (has_) {
                ptr()->~T();
                has_ = false;
            }
        }
    };
}
#endif //MAIN_UTILS_H