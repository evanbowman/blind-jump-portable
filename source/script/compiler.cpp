#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


Value* make_bytecode_function(Value* buffer);


u16 symbol_offset(const char* symbol);


int compile_impl(ScratchBuffer& buffer,
                 int write_pos,
                 Value* code,
                 int jump_offset);


template <typename Instruction>
static Instruction* append(ScratchBuffer& buffer, int& write_pos)
{
    if (write_pos + sizeof(Instruction) >= SCRATCH_BUFFER_SIZE) {
        // FIXME: propagate error! We should return nullptr, but the caller does
        // not check the return value yet. Now, a lambda that takes up 2kb of
        // bytecode seems highly unlikely in the first place, but you never know
        // I guess...
        while (true) ;
    }

    auto result = (Instruction*)(buffer.data_ + write_pos);
    result->header_.op_ = Instruction::op();
    write_pos += sizeof(Instruction);
    return result;
}


int compile_quoted(ScratchBuffer& buffer,
                   int write_pos,
                   Value* code)
{
    if (code->type_ == Value::Type::integer) {
        write_pos = compile_impl(buffer, write_pos, code, 0);
    } else if (code->type_ == Value::Type::symbol) {
        auto inst = append<instruction::PushSymbol>(buffer, write_pos);
        inst->name_offset_.set(symbol_offset(code->symbol_.name_));
    } else if (code->type_ == Value::Type::cons) {

        int list_len = 0;

        while (code not_eq get_nil()) {
            if (code->type_ not_eq Value::Type::cons) {
                // ...
                break;
            }
            write_pos = compile_quoted(buffer, write_pos, code->cons_.car());

            code = code->cons_.cdr();

            list_len++;

            if (list_len == 255) {
                // FIXME: raise error!
                while (true)
                    ;
            }
        }

        auto inst = append<instruction::PushList>(buffer, write_pos);
        inst->element_count_ = list_len;
    }

    return write_pos;
}


int compile_impl(ScratchBuffer& buffer,
                 int write_pos,
                 Value* code,
                 int jump_offset)
{
    if (code->type_ == Value::Type::nil) {

        append<instruction::PushNil>(buffer, write_pos);

    } else if (code->type_ == Value::Type::integer) {

        if (code->integer_.value_ == 0) {
            append<instruction::Push0>(buffer, write_pos);
        } else if (code->integer_.value_ == 1) {
            append<instruction::Push1>(buffer, write_pos);
        } else if (code->integer_.value_ == 2) {
            append<instruction::Push2>(buffer, write_pos);
        } else if (code->integer_.value_ < 127 and
                   code->integer_.value_ > -127) {

            append<instruction::PushSmallInteger>(buffer, write_pos)
                ->value_ = code->integer_.value_;

        } else {
            append<instruction::PushInteger>(buffer, write_pos)
                ->value_.set(code->integer_.value_);
        }
    } else if (code->type_ == Value::Type::symbol) {

        append<instruction::LoadVar>(buffer, write_pos)
            ->name_offset_.set(symbol_offset(code->symbol_.name_));

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

            write_pos =
                compile_impl(buffer, write_pos, lat->cons_.car(), jump_offset);

            auto jne = append<instruction::JumpIfFalse>(buffer, write_pos);

            auto true_branch = get_nil();
            auto false_branch = get_nil();

            if (lat->cons_.cdr()->type_ == Value::Type::cons) {
                true_branch = lat->cons_.cdr()->cons_.car();

                if (lat->cons_.cdr()->cons_.cdr()->type_ == Value::Type::cons) {
                    false_branch = lat->cons_.cdr()->cons_.cdr()->cons_.car();
                }
            }

            write_pos =
                compile_impl(buffer, write_pos, true_branch, jump_offset);

            auto jmp = append<instruction::Jump>(buffer, write_pos);

            jne->offset_.set(write_pos - jump_offset);

            write_pos =
                compile_impl(buffer, write_pos, false_branch, jump_offset);

            jmp->offset_.set(write_pos - jump_offset);

        } else if (fn->type_ == Value::Type::symbol and
                   str_cmp(fn->symbol_.name_, "lambda") == 0) {

            lat = lat->cons_.cdr();

            if (lat->type_ not_eq Value::Type::cons) {
                while (true)
                    ; // TODO: raise error!
            }

            auto lambda = append<instruction::PushLambda>(buffer, write_pos);

            write_pos =
                compile_impl(buffer,
                             write_pos,
                             lat->cons_.car(),
                             jump_offset + write_pos);

            append<instruction::Ret>(buffer, write_pos);

            lambda->lambda_end_.set(write_pos - jump_offset);

        } else if (fn->type_ == Value::Type::symbol and
                   str_cmp(fn->symbol_.name_, "'") == 0) {

            write_pos = compile_quoted(buffer,
                                       write_pos,
                                       lat->cons_.cdr());
        } else {
            u8 argc = 0;

            lat = lat->cons_.cdr();

            while (lat not_eq get_nil()) {
                if (lat->type_ not_eq Value::Type::cons) {
                    // ...
                    break;
                }

                write_pos = compile_impl(buffer, write_pos, lat->cons_.car(),
                                         jump_offset);

                lat = lat->cons_.cdr();

                argc++;

                if (argc == 255) {
                    // FIXME: raise error!
                    while (true)
                        ;
                }
            }

            write_pos = compile_impl(buffer, write_pos, fn, jump_offset);

            switch (argc) {
            case 1:
                append<instruction::Funcall1>(buffer, write_pos);
                break;

            case 2:
                append<instruction::Funcall2>(buffer, write_pos);
                break;

            case 3:
                append<instruction::Funcall3>(buffer, write_pos);
                break;

            default:
                append<instruction::Funcall>(buffer, write_pos)->argc_ = argc;
                break;
            }
        }

    } else {
        append<instruction::PushNil>(buffer, write_pos);
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


    // // We have just inserted/removed an instruction, and now need to scan
    // // through the bytecode, and adjust the offsets of local jumps within the
    // // lambda definition.
    // void fixup_jumps(ScratchBuffer& code_buffer,
    //                  int inflection_point,
    //                  int size_diff)
    // {
    //     int pc = 0;

    //     while (true) {
    //         switch ((Opcode)code_buffer.data_[pc]) {

    //         }
    //     }
    // }
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
            append<instruction::Pop>(*buffer, write_pos);
        }

        write_pos =
            compile_impl(*buffer, write_pos, lat->cons_.car(), 0);

        lat = lat->cons_.cdr();
    }

    append<instruction::Ret>(*buffer, write_pos);

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
                    instruction::Fatal::op()) {
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
