#pragma once


#include "memory/buffer.hpp"
#include "number/numeric.hpp"


// For embedded systems, we cannot pull in all of libc, and we do not want to
// use the standard implementations of the libc functions, because we do not
// know how large or complex the implementations of those functions will be, in
// terms of code footprint. Necessary libc string functions will be provided in

// this header, along with other non-standard string functions. The game itself
// does not need complex logic for manipulating strings--instead, the overlay
// code has plenty of classes for displaying formatted text.


inline u32 str_len(const char* str)
{
    const char* s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}


inline void str_reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;

    while (start < end) {
        std::swap(*(str + start), *(str + end));
        start++;
        end--;
    }
}


inline int str_cmp(const char* p1, const char* p2)
{
    const unsigned char* s1 = (const unsigned char*)p1;
    const unsigned char* s2 = (const unsigned char*)p2;

    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;

        if (c1 == '\0') {
            return c1 - c2;
        }

    } while (c1 == c2);

    return c1 - c2;
}


// A not great, but satisfactory implementation of a string class.
template <u32 Capacity> class StringBuffer {
public:
    using Buffer = ::Buffer<char, Capacity + 1>;

    StringBuffer(const char* init)
    {
        mem_.push_back('\0');
        (*this) += init;
    }

    StringBuffer()
    {
        mem_.push_back('\0');
    }

    StringBuffer(const StringBuffer& other)
    {
        clear();

        for (auto it = other.begin(); it not_eq other.end(); ++it) {
            push_back(*it);
        }
    }

    template <u32 OtherCapacity>
    StringBuffer(const StringBuffer<OtherCapacity>& other)
    {
        static_assert(OtherCapacity <= Capacity);

        clear();

        for (auto it = other.begin(); it not_eq other.end(); ++it) {
            push_back(*it);
        }
    }

    const StringBuffer& operator=(const StringBuffer& other)
    {
        clear();

        for (auto it = other.begin(); it not_eq other.end(); ++it) {
            push_back(*it);
        }
        return *this;
    }

    template <u32 OtherCapacity>
    const StringBuffer& operator=(const StringBuffer<OtherCapacity>& other)
    {
        clear();

        for (auto it = other.begin(); it not_eq other.end(); ++it) {
            push_back(*it);
        }
        return *this;
    }

    const StringBuffer& operator=(StringBuffer&&) = delete;

    char& operator[](int pos)
    {
        return mem_[pos];
    }

    void push_back(char c)
    {
        if (not mem_.full()) {
            mem_[mem_.size() - 1] = c;
            mem_.push_back('\0');
        }
    }

    void pop_back()
    {
        mem_.pop_back();
        mem_.pop_back();
        mem_.push_back('\0');
    }

    typename Buffer::Iterator begin() const
    {
        return mem_.begin();
    }

    typename Buffer::Iterator end() const
    {
        return mem_.end() - 1;
    }

    StringBuffer& operator+=(const char* str)
    {
        while (*str not_eq '\0') {
            push_back(*(str++));
        }
        return *this;
    }

    template <u32 OtherCapacity>
    StringBuffer& operator+=(const StringBuffer<OtherCapacity>& other)
    {
        (*this) += other.c_str();
        return *this;
    }

    StringBuffer& operator=(const char* str)
    {
        this->clear();

        *this += str;

        return *this;
    }

    bool operator==(const char* str)
    {
        return str_cmp(str, this->c_str()) == 0;
    }

    bool full() const
    {
        return mem_.full();
    }

    u32 length() const
    {
        return mem_.size() - 1;
    }

    bool empty() const
    {
        return mem_.size() == 1;
    }

    void clear()
    {
        mem_.clear();
        mem_.push_back('\0');
    }

    const char* c_str() const
    {
        return mem_.data();
    }

private:
    Buffer mem_;
};


template <u32 Capacity>
bool operator==(StringBuffer<Capacity> buf, const char* str)
{
    return str_cmp(str, buf.c_str()) == 0;
}


// I was trying to track down certain bugs, where invalid strings were being
// passed to str_len.
inline bool validate_str(const char* str)
{
    for (int i = 0; i < 100000; ++i) {
        if (str[i] == '\0') {
            return true;
        }
    }
    return false;
}
