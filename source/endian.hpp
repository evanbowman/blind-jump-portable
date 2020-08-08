#pragma once

#include "number/numeric.hpp"

// Because most processors are little endian, I am using little endian byte
// order for binary encoded data.


#define __bswap_constant_16(x) ((u16)((((x) >> 8) & 0xff) | (((x)&0xff) << 8)))

#define __bswap_constant_32(x)                                                 \
    ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) |                      \
     (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))


#ifdef __GBA__
inline bool is_little_endian()
{
    return true;
}
#else
inline bool is_little_endian()
{
    static const u16 i = 0x000f;
    return ((u8*)&i)[0] == 0x0f;
}
#endif

extern "C" {
using size_t = decltype(sizeof(int)); // If only I wasn't too lazy to look up
                                      // which header it's defined in
void* memcpy(void* destination, const void* source, size_t num);
}


template <typename T> T to_host_order(T value);


template <> inline u8 to_host_order(u8 val)
{
    return val;
}

template <> inline u16 to_host_order(u16 val)
{
    if (is_little_endian()) {
        return val;
    } else {
        return __bswap_constant_16(val);
    }
}

template <> inline s16 to_host_order(s16 val)
{
    if (is_little_endian()) {
        return val;
    } else {
        return __bswap_constant_16(val);
    }
}

template <> inline u32 to_host_order(u32 val)
{
    if (is_little_endian()) {
        return val;
    } else {
        return __bswap_constant_32(val);
    }
}

template <> inline s32 to_host_order(s32 val)
{
    if (is_little_endian()) {
        return val;
    } else {
        return __bswap_constant_32(val);
    }
}


template <typename T> class HostInteger {
public:
    // NOTE: I could overload the cast operator and assignment operators to make
    // this class behave similarly to a plain integer. But then I get
    // class-memaccess errors, which I don't want to disable throughout the
    // project... calling get()/set() isn't so bad.

    void set(T value)
    {
        value = to_host_order(value);
        memcpy(data_, &value, sizeof(T));
    }

    T get() const
    {
        T value;
        memcpy(&value, data_, sizeof(T));
        return to_host_order(value);
    }

private:
    char data_[sizeof(T)];
};


using host_u16 = HostInteger<u16>;
using host_u32 = HostInteger<u32>;
using host_s16 = HostInteger<s16>;
using host_s32 = HostInteger<s32>;
