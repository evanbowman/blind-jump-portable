#include "lisp.hpp"
#include "bytecode.hpp"
#include "number/endian.hpp"


namespace lisp {


Value* make_bytecode_function(Value* buffer);


int compile_impl(ScratchBuffer& buffer,
                 int write_pos,
                 Value* code)
{
    if (code->type_ == Value::Type::nil) {
        buffer.data_[write_pos++] = (u8)Opcode::push_nil;
    } else if (code->type_ == Value::Type::integer) {
        buffer.data_[write_pos++] = (u8)Opcode::push_integer;
        auto int_data = (HostInteger<s32>*)(buffer.data_ + write_pos);
        int_data->set(code->integer_.value_);
        write_pos += 4;
    } else if (code->type_ == Value::Type::cons) {
        u8 argc = 0;

        auto lat = code;

        auto fn = lat->cons_.car();
        lat = lat->cons_.cdr();

        while (lat not_eq get_nil()) {
            if (lat->type_ not_eq Value::Type::cons) {
                // ...
                break;
            }

            write_pos += compile_impl(buffer, write_pos, lat->cons_.car()) - write_pos;

            lat = lat->cons_.cdr();

            argc++;

            if (argc == 255) {
                // FIXME: raise error!
                while (true) ;
            }
        }

        write_pos += compile_impl(buffer, write_pos, fn) - write_pos;

        buffer.data_[write_pos++] = (u8)Opcode::funcall;
        buffer.data_[write_pos++] = argc;

    } else {
        buffer.data_[write_pos++] = (u8)Opcode::push_nil;
    }
    return write_pos;
}


void compile(Platform& pfrm, Value* code)
{
    // We will be rendering all of our compiled code into this buffer.
    push_op(make_databuffer(pfrm));

    if (get_op(0)->type_ not_eq Value::Type::data_buffer) {
        return;
    }

    auto fn = make_bytecode_function(get_op(0));
    if (fn->type_ not_eq Value::Type::function) {
        pop_op();
        auto err = fn;
        push_op(err);
        return;
    }

    auto buffer = get_op(0)->data_buffer_.value();
    pop_op();
    push_op(fn);

    __builtin_memset(buffer->data_, 0, sizeof buffer->data_);

    int write_pos = 0;

    auto lat = code;
    while (lat not_eq get_nil()) {
        if (lat->type_ not_eq Value::Type::cons) {
            // error...
            break;
        }

        if (write_pos not_eq 0) {
            buffer->data_[write_pos++] = (u8)Opcode::pop;
        }

        write_pos += compile_impl(*buffer, write_pos, lat->cons_.car()) - write_pos;

        lat = lat->cons_.cdr();
    }

    buffer->data_[write_pos++] = (u8)Opcode::ret;
}


}
