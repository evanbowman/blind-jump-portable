#pragma once

#include "int.h"
#include <ciso646> // For MSVC. What an inept excuse for a compiler.


#ifdef __GBA__
template <typename T> using Atomic = T;
#else
#include <atomic>
template <typename T> using Atomic = std::atomic<T>;
#endif


enum class byte : u8 {};


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
    return (val > 0) ? val : val * -1;
}


// When you don't need an exact value, this works as a fast distance
// approximation.
template <typename T> T manhattan_length(const Vec2<T>& a, const Vec2<T>& b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}


using Degree = u16;
using Angle = Degree;


// Note: In case I need to swap in a fixed-point class in the future.
using Float = float;


inline Float sqrt_approx(const Float x)
{
    constexpr Float magic = 0x5f3759df;
    const Float xhalf = 0.5f * x;

    union {
        float x;
        int i;
    } u;

    u.x = x;
    u.i = magic - (u.i >> 1);
    return x * u.x * (1.5f - xhalf * u.x * u.x);
}

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

template <typename T> bool operator==(const Vec2<T>& rhs, const Vec2<T>& lhs)
{
    return lhs.x == rhs.x and lhs.y == rhs.y;
}

template <typename T>
bool operator not_eq(const Vec2<T>& rhs, const Vec2<T>& lhs)
{
    return lhs.x not_eq rhs.x or lhs.y not_eq rhs.y;
}

template <typename T> T clamp(T x, T floor, T ceil)
{
    if (x < floor) {
        return floor;
    } else if (x > ceil) {
        return ceil;
    } else {
        return x;
    }
}

inline Float smoothstep(Float edge0, Float edge1, Float x)
{
    x = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3 - 2 * x);
}

template <typename T> T interpolate(const T& a, const T& b, Float t)
{
    return a * t + (1 - t) * b;
}


inline Float ease_out(Float time, Float b, Float c, Float duration)
{
    time = time / duration - 1;
    return c * (time * time * time + 1) + b;
}

inline Float ease_in(Float time, Float b, Float c, Float duration)
{
    time /= duration;
    return c * time * time * time + b;
}


inline u8 fast_interpolate(u8 a, u8 b, u8 t)
{
    return b + (u8)(((u16)(a - b) * t) >> 8);
}


using Microseconds = s32; // Therefore, a maximum of ~2147.5 seconds will fit in
                          // this data type.


constexpr Microseconds seconds(u32 count)
{
    return count * 1000000;
}


constexpr Microseconds minutes(u8 count)
{
    return count * seconds(60);
}


constexpr Microseconds milliseconds(u32 count)
{
    return count * 1000;
}


// These functions are imprecise versions of sin/cos for embedded
// systems. Fortunately, we aren't doing heavy scientific calculations, so a
// slightly imprecise angle is just fine. If you want an angle in terms of 360,
// cast the result to a float, divide by numeric_limits<s16>::max(), and
// multiply by 360.
s16 sine(s16 angle);


// This cosine function is implemented in terms of sine, and therefore
// slightly more expensive to call than sine.
s16 cosine(s16 angle);


using UnitVec = Vec2<Float>;


inline UnitVec direction(const Vec2<Float>& origin, const Vec2<Float>& target)
{
    const auto vec = target - origin;

    const auto magnitude = sqrt_approx(vec.x * vec.x + vec.y * vec.y);

    return vec / magnitude;
}


inline Vec2<Float> rotate(const Vec2<Float>& input, Float angle)
{
    const s16 converted_angle = INT16_MAX * (angle / 360.f);
    const Float cos_theta = Float(cosine(converted_angle)) / INT16_MAX;
    const Float sin_theta = Float(sine(converted_angle)) / INT16_MAX;

    return {input.x * cos_theta - input.y * sin_theta,
            input.x * sin_theta + input.y * cos_theta};
}


// Given an angle in degrees, return the corresponding unit vector.
inline Vec2<Float> cartesian_angle(Float degree_angle)
{
    return rotate(Vec2<Float>{1, 0}, degree_angle);
}


inline Float distance(const Vec2<Float>& from, const Vec2<Float>& to)
{
    const Vec2<float> vec = {abs(from.x - to.x), abs(from.y - to.y)};
    return sqrt_approx(vec.x * vec.x + vec.y * vec.y);
}


enum class Cardinal : u8 { north, south, west, east };


inline Float fast_atan_approx(Float x)
{
    return 57.2f * // degrees per radian
               (3.14f / 4) * x -
           x * (abs(x) - 1) * (0.2447f + 0.0663f * abs(x));
}


#include <algorithm>

// This is really inefficient. Ideally, I'd convert all these numbers to
// degrees, instead of multipying by 57.2f at the end. I think there's room for
// improvement here, but math theory has never been one of my strong suits.
inline Float fast_atan2_approx(Float x, Float y)
{
    auto a = std::min(abs(x), abs(y)) / std::max(abs(x), abs(y));
    auto s = a * a;
    auto r = ((-0.0464f * s + 0.1593f) * s - 0.3276f) * s * a + a;
    if (abs(y) > abs(x)) {
        r = 1.5707f - r;
    }
    if (x < 0) {
        r = 3.1415f - r;
    }
    if (y < 0) {
        r = -r;
    }

    auto result = 57.2f * r;
    if (result < 0) {
        result = 360 + result;
    }
    return result;
}
