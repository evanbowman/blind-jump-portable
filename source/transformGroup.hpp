#pragma once

#include <tuple>
#include <utility>
#include "buffer.hpp"


namespace detail {
    template<std::size_t I = 0, typename FuncT, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    for_each(std::tuple<Tp...> &, FuncT)
    {
    }

    template<std::size_t I = 0, typename FuncT, typename... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    for_each(std::tuple<Tp...>& t, FuncT f)
    {
        f(std::get<I>(t));
        for_each<I + 1, FuncT, Tp...>(t, f);
    }
}


template <typename ...Members>
class TransformGroup {
public:
    template <typename F>
    void transform(F&& method)
    {
        detail::for_each(members_, method);
    }

    template <u32 index>
    auto& get()
    {
        return std::get<index>(members_);
    }

private:
    std::tuple<Members...> members_;
};
