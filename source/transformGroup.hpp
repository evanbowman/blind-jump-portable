#pragma once

#include "memory/buffer.hpp"
#include <tuple>
#include <utility>


namespace detail {
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...>&, FuncT)
{
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type for_each(std::tuple<Tp...>& t, FuncT f)
{
    f(std::get<I>(t));
    for_each<I + 1, FuncT, Tp...>(t, f);
}

template <size_t I, typename T, typename Tuple_t>
constexpr size_t index_in_tuple()
{
    static_assert(I < std::tuple_size<Tuple_t>::value);

    typedef typename std::tuple_element<I, Tuple_t>::type el;
    if constexpr (std::is_same<T, el>::value) {
        return I;
    } else {
        return index_in_tuple<I + 1, T, Tuple_t>();
    }
}

} // namespace detail


template <typename... Members> class TransformGroup {
public:
    TransformGroup()
    {
    }

    template <typename T> TransformGroup(T& init) : members_(Members{init}...)
    {
    }

    template <typename F> void transform(F&& method)
    {
        detail::for_each(members_, method);
    }

    template <u32 index> auto& get()
    {
        return std::get<index>(members_);
    }

    template <typename T> auto& get()
    {
        return std::get<T>(members_);
    }

    template <typename T> static constexpr int index_of()
    {
        return detail::index_in_tuple<0, T, decltype(members_)>();
    }

private:
    std::tuple<Members...> members_;
};
