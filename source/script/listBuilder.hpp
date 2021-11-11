#pragma once


#include "lisp.hpp"


namespace lisp {


class ListBuilder {
public:
    ListBuilder() : head_(get_nil())
    {
    }

    void push_front(Value* val)
    {
        Protected protected_val(val);

        auto next = make_cons(val, head_);

        if (tail_ == nullptr) {
            tail_ = next;
        }

        head_.set(next);
    }

    void push_back(Value* val)
    {
        if (tail_ == nullptr) {
            push_front(val);
        } else {
            Protected protected_val(val);
            if (tail_->type() == Value::Type::cons) {
                auto new_tail = make_cons(val, L_NIL);
                tail_->cons().set_cdr(new_tail);
                tail_ = new_tail;
            }
        }
    }

    Value* result()
    {
        return head_;
    }

private:
    Protected head_;
    Value* tail_ = nullptr;
};


} // namespace lisp
