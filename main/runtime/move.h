//
// Created by Johannes Zottele on 05.12.25.
//

#ifndef MAIN_MOVE_H
#define MAIN_MOVE_H

// Minimal type traits
namespace rnt {

    template<typename T>
    struct remove_reference {
        using type = T;
    };

    template<typename T>
    struct remove_reference<T&> {
        using type = T;
    };

    template<typename T>
    struct remove_reference<T&&> {
        using type = T;
    };

    template<typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    // move: turn anything into an rvalue
    template<typename T>
    constexpr remove_reference_t<T>&& move(T&& t) noexcept {
        return static_cast<remove_reference_t<T>&&>(t);
    }

}

#endif //MAIN_MOVE_H