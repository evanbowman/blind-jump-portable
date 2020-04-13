#pragma once

#define COLD [[gnu::cold]]
#define HOT [[gnu::hot]]

#ifdef __GNUC__
#define UNLIKELY(COND) __builtin_expect((COND), false)
#else
#define UNLIKELY(COND) (COND)
#endif

#include <iterator>


#ifdef __GBA__
#define READ_ONLY_DATA __attribute__((section(".rodata")))
#else
#define READ_ONLY_DATA
#endif


namespace _detail {

template <typename T> struct reversion_wrapper {
    T& iterable;
};


template <typename T> auto begin(reversion_wrapper<T> w)
{
    return std::rbegin(w.iterable);
}


template <typename T> auto end(reversion_wrapper<T> w)
{
    return std::rend(w.iterable);
}


} // namespace _detail


template <typename T> _detail::reversion_wrapper<T> reversed(T&& iterable)
{
    return {iterable};
}
