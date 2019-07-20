#pragma once

#include <stdint.h>


using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;


template <typename T> struct Vec2 {
    T x = 0;
    T y = 0;

    template <typename U> Vec2<U> cast() const
    {
        // Note: We could have used a uniform initializer here to
        // prevent narrowing, but there are cases of float->int cast
        // where one might not worry too much about a narrowing
        // conversion.
        Vec2<U> result;
        result.x = x;
        result.y = y;
        return result;
    }
};


template <typename T, typename U = T> struct Rect {
    T x_off = 0;
    T y_off = 0;
    U w = 0;
    U h = 0;
};


template <typename T> T abs(const T& val)
{
    return (val > 0) ? val : -val;
}


// When you don't need an exact value, this works as a fast distance
// approximation.
template <typename T> T manhattan_length(const Vec2<T>& a, const Vec2<T>& b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}


// Note: In case I need to swap in a fixed-point class in the future.
using Float = float;


template <typename T> Vec2<T> operator+(const Vec2<T>& lhs, const Vec2<T>& rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <typename T> Vec2<T> operator-(const Vec2<T>& lhs, const Vec2<T>& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

template <typename T> Vec2<T> operator*(const Vec2<T>& lhs, const Vec2<T>& rhs)
{
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}

template <typename T> Vec2<T> operator/(const Vec2<T>& lhs, const Vec2<T>& rhs)
{
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

template <typename T> Vec2<T> operator+(const Vec2<T>& lhs, const T& rhs)
{
    return {lhs.x + rhs, lhs.y + rhs};
}

template <typename T> Vec2<T> operator-(const Vec2<T>& lhs, const T& rhs)
{
    return {lhs.x - rhs, lhs.y - rhs};
}

template <typename T> Vec2<T> operator*(const Vec2<T>& lhs, const T& rhs)
{
    return {lhs.x * rhs, lhs.y * rhs};
}

template <typename T> Vec2<T> operator*(const T& rhs, const Vec2<T>& lhs)
{
    return {lhs.x * rhs, lhs.y * rhs};
}

template <typename T> Vec2<T> operator/(const Vec2<T>& lhs, const T& rhs)
{
    return {lhs.x / rhs, lhs.y / rhs};
}

template <typename T> Vec2<T> operator/(const T& rhs, const Vec2<T>& lhs)
{
    return {lhs.x / rhs, lhs.y / rhs};
}

template <typename T>
T clamp(T x, T floor, T ceil) {
    if (x < floor) {
        return floor;
    } else if (x > ceil) {
        return ceil;
    } else {
        return x;
    }
}

inline Float smoothstep(Float edge0, Float edge1, Float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3 - 2 * x);
}


template <typename T> T interpolate(const T& a, const T& b, Float t)
{
    return a * t + (1 - t) * b;
}


using Microseconds = s32;


s16 sine(s16 angle);


s16 cosine(s16 angle);
