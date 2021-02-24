#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


Value* make_bytecode_function(Value* buffer);


u16 symbol_offset(const char* symbol);


int compile_impl(ScratchBuffer& buffer, int write_pos, Value* code)
{
    if (code->type_ == Value::Type::nil) {
        buffer.data_[write_pos++] = (u8)Opcode::push_nil;
    } else if (code->type_ == Value::Type::integer) {
        if (code->integer_.value_ == 0) {
            buffer.data_[write_pos++] = (u8)Opcode::push_0;
        } else if (code->integer_.value_ == 1) {
            buffer.data_[write_pos++] = (u8)Opcode::push_1;
        } else if (code->integer_.value_ == 2) {
            buffer.data_[write_pos++] = (u8)Opcode::push_2;
        } else if (code->integer_.value_ < 127 and
                   code->integer_.value_ > -127) {
            buffer.data_[write_pos++] = (u8)Opcode::push_small_integer;
            buffer.data_[write_pos++] = code->integer_.value_;
        } else {
            buffer.data_[write_pos++] = (u8)Opcode::push_integer;
            auto int_data = (HostInteger<s32>*)(buffer.data_ + write_pos);
            int_data->set(code->integer_.value_);
            write_pos += 4;
        }
    } else if (code->type_ == Value::Type::symbol) {
        buffer.data_[write_pos++] = (u8)Opcode::load_var;
        auto offset_data = (HostInteger<u16>*)(buffer.data_ + write_pos);
        offset_data->set(symbol_offset(code->symbol_.name_));
        write_pos += 2;
    } else if (code->type_ == Value::Type::cons) {

        auto lat = code;

        auto fn = lat->cons_.car();

        if (fn->type_ == Value::Type::symbol and
            str_cmp(fn->symbol_.name_, "if") == 0) {

            lat = lat->cons_.cdr();
            if (lat->type_ not_eq Value::Type::cons) {
                while (true)
                    ; // TODO: raise error!
            }

            write_pos +=
                compile_impl(buffer, write_pos, lat->cons_.car()) - write_pos;

            buffer.data_[write_pos++] = (u8)Opcode::jump_if_false;
            auto jne_pos = (HostInteger<u16>*)(buffer.data_ + write_pos);
            write_pos += 2;

            auto true_branch = get_nil();
            auto false_branch = get_nil();

            if (lat->cons_.cdr()->type_ == Value::Type::cons) {
                true_branch = lat->cons_.cdr()->cons_.car();

                if (lat->cons_.cdr()->cons_.cdr()->type_ == Value::Type::cons) {
                    false_branch = lat->cons_.cdr()->cons_.cdr()->cons_.car();
                }
            }

            write_pos +=
                compile_impl(buffer, write_pos, true_branch) - write_pos;
            buffer.data_[write_pos++] = (u8)Opcode::jump;
            auto j_pos = (HostInteger<u16>*)(buffer.data_ + write_pos);
            write_pos += 2;

            jne_pos->set(write_pos);

            write_pos +=
                compile_impl(buffer, write_pos, false_branch) - write_pos;

            j_pos->set(write_pos);

        } else if (fn->type_ == Value::Type::symbol and
                   str_cmp(fn->symbol_.name_, "lambda") == 0) {
            while (true) {
                // ...
            }
        } else {
            u8 argc = 0;

            lat = lat->cons_.cdr();

            while (lat not_eq get_nil()) {
                if (lat->type_ not_eq Value::Type::cons) {
                    // ...
                    break;
                }

                write_pos += compile_impl(buffer, write_pos, lat->cons_.car()) -
                             write_pos;

                lat = lat->cons_.cdr();

                argc++;

                if (argc == 255) {
                    // FIXME: raise error!
                    while (true)
                        ;
                }
            }

            write_pos += compile_impl(buffer, write_pos, fn) - write_pos;

            switch (argc) {
            case 1:
                buffer.data_[write_pos++] = (u8)Opcode::funcall_1;
                break;

            case 2:
                buffer.data_[write_pos++] = (u8)Opcode::funcall_2;
                break;

            case 3:
                buffer.data_[write_pos++] = (u8)Opcode::funcall_3;
                break;

            default:
                buffer.data_[write_pos++] = (u8)Opcode::funcall;
                buffer.data_[write_pos++] = argc;
                break;
            }
        }

    } else {
        buffer.data_[write_pos++] = (u8)Opcode::push_nil;
    }
    return write_pos;
}


void live_values(::Function<24, void(Value&)> callback);


class PeepholeOptimizer {
public:
    u32 run(ScratchBuffer& code_buffer, u32 code_size)
    {
        // TODO...
        // Be careful about jump offsets when implementing these optimizations.
        return code_size;
    }
};


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

    fn->function_.bytecode_impl_.bc_offset_ = 0;

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

        write_pos +=
            compile_impl(*buffer, write_pos, lat->cons_.car()) - write_pos;

        lat = lat->cons_.cdr();
    }

    buffer->data_[write_pos++] = (u8)Opcode::ret;


    write_pos = PeepholeOptimizer().run(
        *dcompr(fn->function_.bytecode_impl_.data_buffer_)
             ->data_buffer_.value(),
        write_pos);

    // std::cout << "compilation finished, bytes used: " << write_pos << std::endl;

    // OK, so now, we've successfully compiled our function into the scratch
    // buffer. But, what about all the extra space in the buffer!? So we're
    // going to scan over all of the interpreter's allocated memory, and
    // collapse our own bytecode into a previously allocated slab of bytecode,
    // if possible.

    const int bytes_used = write_pos;
    bool done = false;

    live_values([fn, &done, bytes_used](Value& val) {
        if (done) {
            return;
        }

        if (fn not_eq &val and val.type_ == Value::Type::function and
            val.mode_bits_ == Function::ModeBits::lisp_bytecode_function) {

            auto buf = dcompr(val.function_.bytecode_impl_.data_buffer_);
            int used = SCRATCH_BUFFER_SIZE - 1;
            for (; used > 0; --used) {
                if ((Opcode)buf->data_buffer_.value()->data_[used] not_eq
                    Opcode::fatal) {
                    ++used;
                    break;
                }
            }

            const int remaining = SCRATCH_BUFFER_SIZE - used;
            if (remaining >= bytes_used) {
                done = true;

                // std::cout << "found another buffer with remaining space: "
                //           << remaining
                //           << ", copying " << bytes_used
                //           << " bytes to dest buffer offset "
                //           << used;

                auto src_buffer =
                    dcompr(fn->function_.bytecode_impl_.data_buffer_);
                for (int i = 0; i < bytes_used; ++i) {
                    buf->data_buffer_.value()->data_[used + i] =
                        src_buffer->data_buffer_.value()->data_[i];
                }

                fn->function_.bytecode_impl_.bc_offset_ = used;
                fn->function_.bytecode_impl_.data_buffer_ = compr(buf);
            }
        }
    });
}


} // namespace lisp
