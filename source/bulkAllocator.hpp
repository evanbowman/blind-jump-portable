#pragma once

#include "platform/platform.hpp"
#include <memory>
#include <new>


// borrowed from standard library
inline void*
align(size_t __align, size_t __size, void*& __ptr, size_t& __space) noexcept
{
    const auto __intptr = reinterpret_cast<uintptr_t>(__ptr);
    const auto __aligned = (__intptr - 1u + __align) & -__align;
    const auto __diff = __aligned - __intptr;
    if ((__size + __diff) > __space)
        return nullptr;
    else {
        __space -= __diff;
        return __ptr = reinterpret_cast<void*>(__aligned);
    }
}


// An abstraction for a single large allocation. Must fit within 1K of
// memory. If your data is nowhere near 1k, and the data is long lived, might be
// better to use a bulk allocator, and share the underlying scratch buffer with
// other stuff.
template <typename T> struct DynamicMemory {
    static_assert(sizeof(T) + alignof(T) <=
                  sizeof Platform::ScratchBuffer::data_);
    Platform::ScratchBufferPtr memory_;
    std::unique_ptr<T, void (*)(T*)> obj_;

    T& operator*() const
    {
        return *obj_.get();
    }

    T* operator->() const
    {
        return obj_.get();
    }

    operator bool() const
    {
        return obj_ not_eq nullptr;
    }
};


template <typename T, typename... Args>
DynamicMemory<T> allocate_dynamic(Platform& pfrm, Args&&... args)
{
    auto sc_buf = pfrm.make_scratch_buffer();

    auto deleter = [](T* val) {
        if (val) {
            if constexpr (not std::is_trivial<T>()) {
                val->~T();
            }
            // No need to actually deallocate anything, we
            // just need to make sure that we're calling the
            // destructor.
        }
    };

    void* alloc_ptr = sc_buf->data_;
    std::size_t size = sizeof sc_buf->data_;

    if (align(alignof(T), sizeof(T), alloc_ptr, size)) {
        T* result = reinterpret_cast<T*>(alloc_ptr);
        new (result) T(std::forward<Args>(args)...);
        alloc_ptr = (char*)alloc_ptr + sizeof(T);
        size -= sizeof(T);

        return {sc_buf, {result, deleter}};
    }
    return {sc_buf, {nullptr, deleter}};
}


// Does not provide any mechanism for deallocation. Everything allocated from
// the memory region will be de-allocated at once when the allocator goes out of
// scope and lets go of its buffer.
struct ScratchBufferBulkAllocator {

    ScratchBufferBulkAllocator(Platform& pfrm)
        : buffer_(pfrm.make_scratch_buffer()), alloc_ptr_(buffer_->data_),
          size_(sizeof buffer_->data_)
    {
    }

    template <typename T> using Ptr = std::unique_ptr<T, void (*)(T*)>;

    template <typename T> static Ptr<T> null()
    {
        auto deleter = [](T* val) {
            if (val) {
                val->~T();
                // No need to actually deallocate anything, we
                // just ean to make sure that we're calling the
                // destructor.
            }
        };
        return {nullptr, deleter};
    }

    template <typename T, typename... Args> Ptr<T> alloc(Args&&... args)
    {
        auto deleter = [](T* val) {
            if (val) {
                val->~T();
                // No need to actually deallocate anything, we
                // just ean to make sure that we're calling the
                // destructor.
            }
        };

        if (align(alignof(T), sizeof(T), alloc_ptr_, size_)) {
            T* result = reinterpret_cast<T*>(alloc_ptr_);
            new (result) T(std::forward<Args>(args)...);
            alloc_ptr_ = (char*)alloc_ptr_ + sizeof(T);
            size_ -= sizeof(T);
            return {result, deleter};
        }

        return {nullptr, deleter};
    }

private:
    Platform::ScratchBufferPtr buffer_;
    void* alloc_ptr_;
    std::size_t size_;
};


template <u8 pages> struct BulkAllocator {
    BulkAllocator(Platform& pfrm)
    {
        if (pfrm.scratch_buffers_remaining() < pages) {
            warning(pfrm, "available scratch buffer count may be too low!");
        }
        buffers_.emplace_back(pfrm);
    }

    template <typename T, typename... Args>
    auto alloc(Platform& pfrm, Args&&... args)
    {
        auto try_alloc = [&] {
            return buffers_.back().template alloc<T>(
                std::forward<Args>(args)...);
        };

        if (auto mem = try_alloc()) {
            return mem;
        } else {
            if (buffers_.full()) {
                return ScratchBufferBulkAllocator::null<T>();
            } else {
                buffers_.emplace_back(pfrm);
                if (auto mem = try_alloc()) {
                    return mem;
                } else {
                    return ScratchBufferBulkAllocator::null<T>();
                }
            }
        }
    }

private:
    Buffer<ScratchBufferBulkAllocator, pages> buffers_;
};
