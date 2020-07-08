#pragma once

#include <new>

#include "memory/pool.hpp"


template <typename T> struct BiNode {
    BiNode* right_;
    BiNode* left_;
    T data_;
};


template <typename T, typename Pool> class List {
public:
    using Node = BiNode<T>;
    using ValueType = T;

    List(Pool& pool) : begin_(nullptr), pool_(&pool)
    {
        static_assert(sizeof(Node) == Pool::element_size() and
                          alignof(Node) == Pool::alignment(),
                      "Pool incompatible");
    }

    List(List&& other)
    {
        begin_ = other.begin_;
        other.begin_ = nullptr;
        pool_ = other.pool_;
    }

    List(const List&) = delete;

    ~List()
    {
        clear();
    }

    void push(const T& elem)
    {
        if (auto mem = pool_->get()) {
            new (mem) Node{begin_, nullptr, elem};
            if (begin_) {
                begin_->left_ = reinterpret_cast<Node*>(mem);
            }
            begin_ = reinterpret_cast<Node*>(mem);
        }
    }

    void push(T&& elem)
    {
        if (auto mem = pool_->get()) {
            new (mem) Node{begin_, nullptr, std::forward<T>(elem)};
            if (begin_) {
                begin_->left_ = reinterpret_cast<Node*>(mem);
            }
            begin_ = reinterpret_cast<Node*>(mem);
        }
    }

    void pop()
    {
        if (begin_) {
            auto popped = begin_;

            popped->~Node();
            pool_->post(reinterpret_cast<byte*>(popped));

            begin_ = begin_->right_;
            if (begin_) {
                begin_->left_ = nullptr;
            }
        }
    }

    void clear()
    {
        while (begin_)
            pop();
    }

    bool empty()
    {
        return begin_ == nullptr;
    }

    class Iterator {
    public:
        Iterator(Node* ptr) : node_(ptr)
        {
        }

        const Iterator& operator++()
        {
            node_ = node_->right_;
            return *this;
        }

        T* operator->()
        {
            return &node_->data_;
        }

        T& operator*()
        {
            return node_->data_;
        }

        // Don't implement operator-- yet! list::end() is implemented in sort of
        // a hacky way.

        bool operator==(const Iterator& other) const
        {
            return other.node_ == node_;
        }

        bool operator not_eq(const Iterator& other) const
        {
            return other.node_ not_eq node_;
        }

        Node* node_;
    };

    Iterator erase(Iterator it)
    {
        if (it.node_->left_) {
            it.node_->left_->right_ = it.node_->right_;
        }

        if (it.node_->right_) {
            it.node_->right_->left_ = it.node_->left_;
        }

        if (it.node_ == begin_) {
            begin_ = begin_->right_;
        }

        auto next = it.node_->right_;

        it.node_->~Node();
        pool_->post(reinterpret_cast<byte*>(it.node_));

        return Iterator(next);
    }

    Iterator begin() const
    {
        return Iterator(begin_);
    }

    Iterator end() const
    {
        // NOTE: This technically works, but prevents us from implementing the
        // decrement operator on the Iterator. FIXME!
        return Iterator(nullptr);
    }

private:
    Node* begin_;
    Pool* pool_;
};


template <typename T, typename Pool> u32 length(const List<T, Pool>& lat)
{
    u32 len = 0;

    for (auto it = lat.begin(); it not_eq lat.end(); ++it) {
        ++len;
    }

    return len;
}


template <typename T, typename Pool> T* list_ref(List<T, Pool>& lat, int i)
{
    for (auto& elem : lat) {
        if (i == 0) {
            return &elem;
        }
        --i;
    }
    return nullptr;
}
